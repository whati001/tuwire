# ACCCTL
accctl is a small utility application, shipped with the iPhone test os called [SwitchBoard](https://www.theiphonewiki.com/wiki/SwitchBoard.app). The utility allows you to retrieve detail information about the connected Lightning accessory. 

## Setup
As mentioned above, the utility is not part of the official iOS version. However, we can download it from the SwitchBoard firmware and run it on jailbroken devices very easily. Luckily, SwitchBoard firmwares are already leaked and available to download for iPhones up to the iPhone6.

### Download binary
To setup accctl on the iPhone running please first download the March-O binary via the following links: 
[iPhone6-iOS 12.5.6-accctl](https://archive.org/download/iosswitchboardfirmwares/8.0%20OkemoAni%2012A93650o%20%28iPhone%206%29.zip/12A93650o%2FN56%3AN61%2Fusr%2Flocal%2Fbin%2Faccctl)
[iPhone5-iOS](https://archive.org/download/iosswitchboardfirmwares/6.0%20Sundance%2010A316%20%28iPhone%205%29.tar/usr%2Flocal%2Fbin%2Faccctl)

### Jailbreak device
#### iPhone5
Please follow this guide [here](https://onejailbreak.com/blog/h3lix-jailbreak/)
You need to donwload the h3lix ipa file, the patch script and the sideloadly application. After downloading and installing all these things, please follow these steps:
* patch the ipa file with ./path.sh <input> <output>
* start sideloadly
* connect your iphone and trust the pc
* select the patched ipa file (h3lix)
* enter your apple id (create some dummy is recommended, never trust hackers :p )
* wait until completion
* if you receive some error, retry the process
* you will see a h3lix application
* to run it, you need to trust the developer, trust it by going into settings -> general -> device management -> <user> -> trust
* now, you can run the app and hit jailbreak
* wait for it, your device will restart
* done, cydia should be installed

The jailbreak is gone after the restart, by you can reapply it via the app. In addition, the openssh server has only worked after a restart + rejailbreak.
Seems like the daemon is not started without a restart + rejailbreak.

#### iPhone6
Next, you need to jailbreak the device, we have used the bootable [checkra1n](https://github.com/asineth0/checkn1x) image from asineth0.
Simply download the latest release, which was [checkn1x-1.1.7.iso](https://github.com/asineth0/checkn1x/releases/download/1.1.7/checkn1x-1.1.7.iso) these days and flash it to an usb stick. We have used [Ventoy](https://www.ventoy.net/en/index.html) for booting the image. This method (except booting via Ventoy) is shown in this youtube video [Jailbreak iOS 12.5.6 Checkra1n |iOS 12.5.6 Jailbreak |iPhone 6/5S 12.5.6 Jailbreak| Jailbreak 12.5.6](https://www.youtube.com/watch?v=_IbBXbx79TM).

### Install openssh
Next, we need to install openSSH via Cydia. Do to so, simply open Cydia and search for the package `OpenSSH`. At this time, the latest version was 8.4-2. After installing it, please connect via ssh to the iPhone device
```bash
# connect via ssh, the ip of the iphone is 192.168.0.17 in this example
# please use the password `alpine` for the root user
$ ssh root@192.168.0.17
```

### Transfer accctl binary
Next, we can transfer the accctl binary to the jailbroken device. It's important to store the binary into the following directory `/usr/local/bin/` otherwise you will receive an `killed: 9` error.
```bash
# on iphone, create the directory first
$ mkdir -p /usr/local/bin/

# on host, copy over the binary
$ scp accctl root@192.168.0.17:/usr/local/bin/
```

### Execute accctl binary
Next, we can execute the command on the jailbroken device.
```bash
# on iphone
$ cd /usr/local/bin/
$ chmod +x accctl

# with official lightning to usb cable connected
$ ./accctl list

iPhone:/usr/local/bin root# ./accctl list
IOAccessoryManager primary port 1
        Accessory ID: 91 - Digital ID
        Digital ID: 10 0c 00 00 00 00
        Interface device info: vid=2 pid=1 rev=2 flags=0x80
        Interface device serial number: 0x600126ddaa6f
        Interface module serial number: DYG7285UP9VFJYHAY
        Accessory serial number: FC973152926G0NHAS
        USB connect state: none (active)
        USB current limit: detached (base 0 offset none max none)
        Battery pack: no
        Power Mode:   on (supported: off<0mA> on<100mA> high-current<500mA>)
        Sleep Power:  disabled
IOAccessoryPort device port 1 (manager primary port 1, transport type 0)
IOAccessoryPort device port 2 (manager primary port 1, transport type 2)
iPhone:/usr/local/bin root# 
```

## Output
Some accctl outputs:

### iPhone6 + LightningUSB + noPC
```bash
iPhone:/usr/local/bin root# ./accctl list
IOAccessoryManager primary port 1
        Accessory ID: 91 - Digital ID
        Digital ID: 10 0c 00 00 00 00
        Interface device info: vid=2 pid=1 rev=2 flags=0x80
        Interface device serial number: 0x600126ddaa6f
        Interface module serial number: DYG7285UP9VFJYHAY
        Accessory serial number: FC973152926G0NHAS
        USB connect state: none (active)
        USB current limit: detached (base 0 offset none max none)
        Battery pack: no
        Power Mode:   on (supported: off<0mA> on<100mA> high-current<500mA>)
        Sleep Power:  disabled
IOAccessoryPort device port 1 (manager primary port 1, transport type 0)
IOAccessoryPort device port 2 (manager primary port 1, transport type 2)
```

### iPhone6 + LightningUSB + PC
```bash
iPhone:/usr/local/bin root# ./accctl list
IOAccessoryManager primary port 1
        Accessory ID: 91 - Digital ID
        Digital ID: 10 0c 00 00 00 00
        Interface device info: vid=2 pid=1 rev=2 flags=0x80
        Interface device serial number: 0x600126ddaa6f
        Interface module serial number: DYG7285UP9VFJYHAY
        Accessory serial number: FC973152926G0NHAS
        USB connect state: host (active)
        USB current limit: 1500 (base 1500 offset none max none)
        Battery pack: no
        Power Mode:   on (supported: off<0mA> on<100mA> high-current<500mA>)
        Sleep Power:  disabled
IOAccessoryPort device port 1 (manager primary port 1, transport type 0)
IOAccessoryPort device port 2 (manager primary port 1, transport type 2)
```

### iPhone6 + USB-OTG + noDevice
```bash
IOAccessoryManager primary port 1
        Accessory ID: 91 - Digital ID
        Digital ID: 11 f0 00 00 00 00
        Interface device info: vid=1 pid=37 rev=1 flags=0x80
        Interface device serial number: 0xa7236724c611
        Interface module serial number: DWH23871E07F5L4AC
        Accessory serial number: C0824260115DY7QA7
        USB connect state: device (active)
        USB current limit: detached (base 1500 offset none max none)
        Battery pack: no
        Power Mode:   on (supported: off<0mA> on<100mA> high-current<500mA>)
        Sleep Power:  disabled
IOAccessoryPort device port 1 (manager primary port 1, transport type 0)
IOAccessoryPort device port 2 (manager primary port 1, transport type 2)
```

### iPhone6 + CharigingOnly
```bash
iPhone:~ root# accctl list
IOAccessoryManager primary port 1
        Accessory ID: 91 - Digital ID
        Digital ID: 10 0c 00 00 00 00
        Interface device info: vid=1 pid=37 rev=1 flags=0x80
        Interface device serial number: 0x8864c124ef45
        Interface module serial number: F1123327FUTF5V93Z
        Accessory serial number: F0Q23651Y0JF7C0AW
        USB connect state: none (active)
        USB current limit: detached (base 0 offset none max none)
        Battery pack: no
        Power Mode:   on (supported: off<0mA> on<100mA> high-current<500mA>)
        Sleep Power:  disabled
IOAccessoryPort device port 1 (manager primary port 1, transport type 0)
IOAccessoryPort device port 2 (manager primary port 1, transport type 2)
```
