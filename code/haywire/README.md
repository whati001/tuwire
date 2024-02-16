# Haywire
Haywire is the official Lightning to HDMI (Digitial AV) adapter from Apple.
Based on the research done by [Satan](https://twitter.com/nyan_satan/status/1155149135600934912) and [LampdaConcepts](http://blog.lambdaconcept.com/post/2019-09/graywire/), the Haywire is a normal USB Device.

## USB-Analyzer iPhone (Jailbreak)

We are capable of capturing the USB traffic between the iPhone and the device via `tcpdump` and `ifconfig` on a jailbroken device. We have performed this experiment on an iPhone 6 jailbroken via [checkn1x-1.1.7.iso](https://github.com/asineth0/checkn1x/releases/download/1.1.7/checkn1x-1.1.7.iso) and with the ifconfig from the [Switchboard-8.0 Okemo 12A93311h (iPhone 6).zip](https://archive.org/download/iosswitchboardfirmwares/8.0%20Okemo%2012A93311h%20%28iPhone%206%29.zip) as well as tcpdump from the cydia repositories.

### Preparation
First jailbreak the device and install openssh via Cydia. After the installation, you should be able to login as `root` with the password `alpine`. After the login, please install tcpdump as shown below:
```bash
# on host machine; use alpine as pwd
$ ssh root@<iphoneIP>

# on iPhone
$ apt update && apt install tcpdump
$ apt update && apt install network-cmds
```

Next you need to download the ipconfig from the [Switchboard-8.0 Okemo 12A93311h (iPhone 6).zip](https://archive.org/download/iosswitchboardfirmwares/8.0%20Okemo%2012A93311h%20%28iPhone%206%29.zip) image and copy it to the device into `/sbin/`.
```bash
# on host machine; use alpine as pwd
$ scp ./ifconfig root@<iphoneIP>:/sbin
```

### Capture traffic
After the preparation phase we can connect the Haywire adapter and run the following command as soon as possible on the device:
```bash
$ ifconfig EHC1 up && tcpdump -i EHC1 -w eth1.cap
```
This command will enable the USB interface EHC1 (please do not ask me why this one works) and starts to write the captured packages into the file eth1.cap.

## Build Nero driver
To build the nero driver, please install the current Zephyr SDK as shown [here](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
In the case that your driver does not receive any USB packet, please double check if the STM32 Zephyr USB driver still faces the FIFO size issue descript in [ISSUE-54162](https://github.com/zephyrproject-rtos/zephyr/issues/54162). If so, you need to fix the Zephyr USB driver an properly allocate more memory to the RX-FIFO. However, even if the ISSUE is not fixed yet, it should work because we amount of endpoints if fixed to <4> in the device tree file. But be advised, that this implies that the current dts file does not support any further USB interfaces.
After the SDK is present, building and flashing the board is done by leveraging the `west` utility.
```bash
# source/activate Zephyr SDK env
$ source $HOME/zephyrproject/.venv/bin/activate

# build project without storing frames
$ west build -p -b stm32f723e_disco $THESIS_HOME/code/haywire/tuwire_usb_device
# build project with flash storage -> enable qspi node in dt file
$ west build -b stm32f723e_disco /home/akarner/tulocal/master_thesis_andreas_karner/code/haywire/tuwire_usb_device -DOVERLAY_CONFIG=overlay-flash_storage.conf
# build project with sdcard storage -> enable spi node in dt file
$ west build -b stm32f723e_disco /home/akarner/tulocal/master_thesis_andreas_karner/code/haywire/tuwire_usb_device -DOVERLAY_CONFIG=overlay-sdcard_storage.conf

# flash board 
$ west flash
```

## USB-Interface:
### Connect to PC in DFU mode
As stated by [Satan](https://twitter.com/nyan_satan/status/1155149135600934912), simply connect the USB lines properly.
Please consider the rotation of the Haywire adapter, if the HDMI + Lightning symbol is facing top use this pinout
* USB -> Lightning
* 5V -> ACC2
* GND -> GND
* DP -> DP1
* DN -> DN1
otherwise, if the Module number and designed by Apple is on the top please use this pinout:
* USB -> Lightning
* 5V -> ACC1
* GND -> GND
* DP -> DP2
* DN -> DN2
* 
The following device appearers on the host machine:

**dmesg**
```
[ 6620.989141] usb 1-2: new high-speed USB device number 21 using xhci_hcd
[ 6621.135677] usb 1-2: New USB device found, idVendor=05ac, idProduct=1227, bcdDevice= 0.00
[ 6621.135691] usb 1-2: New USB device strings: Mfr=2, Product=3, SerialNumber=4
[ 6621.135698] usb 1-2: Product: Apple Mobile Device (DFU Mode)
[ 6621.135703] usb 1-2: Manufacturer: Apple Inc.
[ 6621.135707] usb 1-2: SerialNumber: CPID:8747 CPRV:10 CPFM:03 SCEP:10 BDID:00 ECID:000003D5E73519AF IBFL:00 SRTG:[iBoot-1413.8]
[ 6636.029912] apple-mfi-fastcharge 1-2: USB disconnect, device number 21
```

**lsusb**
```
Bus 001 Device 021: ID 05ac:1227 Apple, Inc. Mobile Device (DFU Mode)
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               2.00
  bDeviceClass            0 
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0        64
  idVendor           0x05ac Apple, Inc.
  idProduct          0x1227 Mobile Device (DFU Mode)
  bcdDevice            0.00
  iManufacturer           2 Apple Inc.
  iProduct                3 Apple Mobile Device (DFU Mode)
  iSerial                 4 CPID:8747 CPRV:10 CPFM:03 SCEP:10 BDID:00 ECID:000003D5E73519AF IBFL:00 SRTG:[iBoot-1413.8]
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x0019
    bNumInterfaces          1
    bConfigurationValue     1
    iConfiguration          5 
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              100mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           0
      bInterfaceClass       254 Application Specific Interface
      bInterfaceSubClass      1 Device Firmware Update
      bInterfaceProtocol      0 
      iInterface              0 
      Device Firmware Upgrade Interface Descriptor:
        bLength                             7
        bDescriptorType                    33
        bmAttributes                        1
          Will Not Detach
          Manifestation Intolerant
          Upload Unsupported
          Download Supported
        wDetachTimeout                     10 milliseconds
        wTransferSize                    2048 bytes
```

### Connect to PC in Nero mode
As stated by Satan, the Haywire adapter is a simple USB devices, which receives the firmware from the Apple mobile device.
This is the reason, why we see in the above experiment a iBoot device in DFU mode, because it is waiting for the firmware.
However, what if we connect the Haywire to an Apple Device load the firmware and then connect it to a PC via USB.
We have performed this experiment and stored the syslogs as well as the lsusb output below:

**dmesg**
```
[14456.277969] usb 1-3: New USB device found, idVendor=05ac, idProduct=12ad, bcdDevice= 2.11
[14456.277984] usb 1-3: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[14456.277991] usb 1-3: Product: iAccy1
[14456.277997] usb 1-3: Manufacturer: Apple Inc.
[14456.278002] usb 1-3: SerialNumber: nomac?123456

```

**lsusb**
```
Bus 001 Device 012: ID 05ac:12ad Apple, Inc. iAccy1
Couldn't open device, some information will be missing
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               2.00
  bDeviceClass            0 
  bDeviceSubClass         0 
  bDeviceProtocol         0 
  bMaxPacketSize0        64
  idVendor           0x05ac Apple, Inc.
  idProduct          0x12ad 
  bcdDevice            2.11
  iManufacturer           1 Apple Inc.
  iProduct                2 iAccy1
  iSerial                 3 nomac?123456
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x003e
    bNumInterfaces          2
    bConfigurationValue     1
    iConfiguration          5 
    bmAttributes         0xc0
      Self Powered
    MaxPower              500mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           3
      bInterfaceClass       255 Vendor Specific Class
      bInterfaceSubClass    240 
      bInterfaceProtocol      0 
      iInterface              6 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x02  EP 2 OUT
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0200  1x 512 bytes
        bInterval              10
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0200  1x 512 bytes
        bInterval              10
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x83  EP 3 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0040  1x 64 bytes
        bInterval              10
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       0
      bNumEndpoints           2
      bInterfaceClass       255 Vendor Specific Class
      bInterfaceSubClass     42 
      bInterfaceProtocol    255 
      iInterface              7 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x85  EP 5 IN
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0200  1x 512 bytes
        bInterval              10
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x04  EP 4 OUT
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0200  1x 512 bytes
        bInterval              10
```

As we can see, the interface looks quite odd. There is nothing about Nero mentioned. 