# iap_core library
Small C-based library to communicate with iDevice via the iAP1/iAP2 interface. In addition, the library contains a binding to interact with the Apple Authentication Coprocessors (CP).

## Known issues
* The current lib is based on request-response pattern. However, the iDevice is allowed to send multiple consecutive message. If so, only the first message will be read and the other one will remain in the UART buffer. The idea was use interrupts to read spontaneously data, but where to store it than? I think it would be better to decouple the send and receive entirely and handle it within a own "thread". Functions can than push commands/messages to it, which will be send via the transport to the iDevice. In addition, it can subscribe for responses, which will be forwarded to it on arrival. In this case, we can also handle spontaneously data, because we simply broadcast it or so. Currently, we simply read until we receive the response we desire. 
* transaction handling is vary basic and shaky


## Emulated iAP accessory:
AccName(0x01): Ccc-chip
AccFirmware(0x04): 0x01,0x01,0x02
AccHwVersion(0x05): 0x02,0x03,0x01
AccManufacture(0x06): XlsTech
AccModelNumber(0x07): C02