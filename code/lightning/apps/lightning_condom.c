/**********************************************************************
 *
 * Filename:    lightning_condom.c
 *
 * Description: Simple example, on how to transform a regular
 *              Lightning data-cable into a charging-only.
 *
 **********************************************************************/
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

#include "idbus.h"
#include "idbus_io.h"
#include "idbus_util.h"
#include <unwind.h>

#define IDBUS_PLUG_PIN 3
#define IDBUS_PORT_PIN 2

#define ASSERT                                                                 \
  while (1) {                                                                  \
    sleep_ms(100);                                                             \
  }

static uint8_t plug_cur_header, plug_cur_crc, running, has_response;
static int ret;

// allocate memory for storing the IDBUS messages
// 64 bytes should be sufficient to store all variations of IDBUS msgs
#define IDBUS_BUFFER_SIZE 64

static int idbus_plug_fd;
static uint8_t idbus_plug_data[IDBUS_BUFFER_SIZE];
static idbus_msg_t idbus_plug_msg = {.header = 0,
                                     .data_ptr = idbus_plug_data,
                                     .data_len = IDBUS_BUFFER_SIZE,
                                     .crc = 0};

static int idbus_port_fd;
static uint8_t idbus_port_data[IDBUS_BUFFER_SIZE];
static idbus_msg_t idbus_port_msg = {.header = 0,
                                     .data_ptr = idbus_port_data,
                                     .data_len = IDBUS_BUFFER_SIZE,
                                     .crc = 0};

#define IDBUS_SECURE_DIGITIAL_ID_LEN 6
static uint8_t secure_digitial_id[IDBUS_SECURE_DIGITIAL_ID_LEN] = {
    0x10, 0xc, 0x0, 0x0, 0x0, 0x0};

void secure_hifive_response(idbus_msg_t *msg) {
  switch (msg->header) {
  case 0x75:
    if (0 != memcmp(secure_digitial_id, msg->data_ptr,
                    IDBUS_SECURE_DIGITIAL_ID_LEN)) {
      printf("Found unsecure DigitialID, let's secure it\n");
      memcpy(msg->data_ptr, secure_digitial_id, IDBUS_SECURE_DIGITIAL_ID_LEN);
      idbus_compute_checksum(msg);
    } else {
      printf("Received DigitialID is already secure, only terminating the USB "
             "pins is needed\n");
    }
    break;

  default:
    printf("All good, nothing to secure\n");
    break;
  }
  return;
}

static _Unwind_Reason_Code trace_me(struct _Unwind_Context *a, void *var) {
  uint8_t *value = (uint8_t *)var;
  value++;
  printf("Some value: %d\n", *value);

  return _URC_NO_REASON;
}

[[gnu::noinline]] void funcB() {
  printf("some backtrace example begin\n");
  uint8_t idx = 5;
  _Unwind_Reason_Code ret = _Unwind_Backtrace(trace_me, &idx);
  printf("some backtrace example done and idx: %d and ret: %d\n", idx, ret);
}

[[gnu::noinline]] void funcA() { funcB(); }
int main() {
  // board initialization
  stdio_init_all();
  sleep_ms(5000);

  printf("Started IDBUS lightning condom example application\n");
  ret = idbus_init();

  funcA();

  // step 1: open new IDBUS instances
  running = 1;
  ret = idbus_init();
  ret |= idbus_open_hifive(&idbus_plug_fd, (void *)IDBUS_PLUG_PIN);
  ret |= idbus_open_tristar(&idbus_port_fd, (void *)IDBUS_PORT_PIN);
  if (IDBUS_OK != ret) {
    printf("Failed to initiate Lightning condom instance\n");
    ASSERT;
  }
  printf("Successfully initiated Lightning condom and opened idbus_instances "
         "for Plug and Port\n");

  // step 2: proxy IDBUS messages and secure them
  while (running) {
    // step 2.1: read original request from plug, send by Apple device
    printf("Start to read a IDBUS message from the Plug\n");
    ret = idbus_read(idbus_plug_fd, &idbus_plug_msg);
    if (IDBUS_OK != ret) {
      printf("Failed to read Tristar request from Apple device\n");
      ASSERT;
    }

    // if we do not have the answer yet, let's ask for it
    if (plug_cur_header != idbus_plug_msg.header ||
        plug_cur_crc != idbus_plug_msg.crc) {
      printf("New IDBUS message received, let's request the unsecure answer\n");
      plug_cur_header = idbus_plug_msg.header;
      plug_cur_crc = idbus_plug_msg.crc;
      has_response = 1;

      // forward to original Tristar request to the HIFIVE and read save the
      // response
      ret = idbus_write(idbus_port_fd, &idbus_plug_msg);
      if (IDBUS_OK != ret) {
        printf("Failed to send Tristar request to Lightning accessory\n");
        ASSERT;
      }
      ret = idbus_read(idbus_port_fd, &idbus_port_msg);
      if (IDBUS_ERR_TIMEOUT == ret) {
        printf("Received IDBUS message, which does not require any response\n");
        has_response = 0;
      } else if (IDBUS_OK != ret) {
        printf("Failed to read Tristar response from Lightning accessory\n");
        ASSERT;
      }

      // now, let's make the response secure
      secure_hifive_response(&idbus_port_msg);
    }

    // reply secured response from Lightning accessory to Apple device
    // #TODO: add a check if ~2.2ms passed, if so, there is no need to send
    // because Tristar will ignore it because it already run into a timeout
    if (has_response) {
      ret = idbus_write(idbus_plug_fd, &idbus_port_msg);
      if (IDBUS_OK != ret) {
        printf("Failed to write received message to Apple device\n");
        ASSERT;
      }
    }
  }

  // step 3: clear everything
  ASSERT;
}