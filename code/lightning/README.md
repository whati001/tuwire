# Apple Lightning applications
This directory contains the various developed applications used throughout this thesis.

## Folder structure
* apps: contains the high level Lightning applications
  * usb_otg.c: USB-OTG moke, used by TuWire to attack the Apple device
  * lightning_condom.c: Lightning condom code
* iap: lib_iap library
  * assets:
    * idbus_*.sal: LogicAnalyzers trace, use Saleae Logic app to open it
    * IPA*: extracted certificate from unauthorized USB-OTG adapter
    * wifi_*.png: screenshot of WiFi sharing request
  * iap_core: source code of lib
* idbus:
  * assets:
    * idbus_*.sal: LogicAnalyzers trace, use Saleae Logic app to open it
    * *.jpg|png: images showing experimental setups
  * apps: IDBUS apps like hifive and tristar emulation
  * idbus_core: source code of lib

## Building
The project is build with the Raspberry Pi Pico, so please install it's c/c++ SDK as explained [here](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html). After installation, please build the project as usual:
```bash
# export path to your pico sdk
export PICO_SDK_PATH=~/pico/pico-sdk

# create build directory and build
mkdir build && cd build && cmake .. && make
```
