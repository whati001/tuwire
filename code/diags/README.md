# Diags
Diags is a diagnostic image that is installed on devices, and is used for testing in factories. It is commonly located in /AppleInternal/Diags/bin/diag.img4 for 64 bit devices, or /AppleInternal/Diags/bin/diag.img3 for 32 bit devices.

* [TheIphoneWiki](https://www.theiphonewiki.com/wiki/Diags)

## Boot Diags
To boot the diags image, we need either to compile and a load a DEVELOPMENT build of the iBoot image, or binary patch the original iBoot to load diags instead of the kernel. There is some leaked source code available for the iBoot image, however, as far as I know and can trust Satan, it's not complete and does not compile properly.

* [iBoot & SecureBoot src](https://yalujailbreak.net/bootrom-iboot-source-code-leaked/)
* [NyanSatan-Build-iBoot](https://nyansatan.github.io/building-iboot/)
* [iOS Diagnostics mode](https://haiyuidesu.github.io/blog/diagnostics.html)

For batching the official iBoot image from the ipws files, please keep reading.

### Retrieve Keys
To retrieve the keys for decrypting the iBSS, iBEC and iBoot, you need to get access to the AES engine for the iPhone. The easies way is to use `ipwndfu` and pwn the bootrom. After doing so, you can use the `--decrypt-gid=KEYBAG` do decrypt the IV+KEY from the KEYBAG from the img4 and receive the raw key.

tools:
* [ipwndfu](https://github.com/axi0mX/ipwndfu)
* [img4lib](https://github.com/xerub/img4lib)
* [autodecrypt](https://github.com/matteyeux/autodecrypt)

```bash
# first download the ipsw
$ curl https://updates.cdn-apple.com/2021WinterFCS/fullrestores/012-51870/2A494DC9-8DE4-43F8-B084-87E91D5AF9BA/iPhone_4.7_12.5.6_16H71_Restore.ipsw -p iPhone_4.7_12.5.6_16H71_Restore.ipsw
# the ipsw is a simple zip, so let's extract it
$ mv iPhone_4.7_12.5.6_16H71_Restore.ipsw iPhone_4.7_12.5.6_16H71_Restore.zip
$ unzip iPhone_4.7_12.5.6_16H71_Restore.zip

# find the iBSS and iBEC within the 
$ cd Firmware/dfu/

# show the keybag of the file (https://github.com/xerub/img4lib), we are interested within the first IV + KEY string
$ img4 -i iBEC.n61.RELEASE.im4p -b
#1B3C581CFFA255FEA7C291369E82E5960937929413BD30A6DEFE1B34AA7E4F302A492C9D27209F84D8E5E1B75D870B77
#1517089FC6FA40E6BD8FF340CF771A6365897C577999EF0BF461AC49CA4173B2436EFBCEBBA42EF8A7EEC31EDCAD9B9C

# now we need to pwn the device to get access to the AES engine
# this is done on a old mac machine, we have some troubles to get ipwndfu running on linux
# on linux uses a wrong libusb and does not find the langids from the usb device
# run this multiple times, it will not work always, also try to use sudo ...
$ python ipwndfu -p

# after successful pwn, we can decrypt the keybag with the GID (group aes key)
$ python ipwndfu --decrypt-gid=1B3C581CFFA255FEA7C291369E82E5960937929413BD30A6DEFE1B34AA7E4F302A492C9D27209F84D8E5E1B75D870B77
# Decrypting with t7000si GID key.
# a2b36e0dddd8f4b06f43dd2acd69f04bfb9e1b44a1f1f078f17e040948f7006e564535a883cf2db9ab9304e4aad87a84

# now we can decrypt the image, we will use autodecrypt, activate the virtual env first (source)
$ python autodecrypt/main.py -f iBEC.n61.RELEASE.im4p -i 12.5.6 -d iPhone7,2 --ivkey a2b36e0dddd8f4b06f43dd2acd69f04bfb9e1b44a1f1f078f17e040948f7006e564535a883cf2db9ab9304e4aad87a84
# finally we have the decrypted iBEC/iBSS
```

The final keys are:
#### iBSS.n61.RELEASE.im4p
```b593c362965e57ed27be521e78dd958abc6558cce87081decb4a312583d2c52597f877432fb6c7608b71cba827d2ab4b```
 
#### iBEC.n61.RELEASE.im4p
```a2b36e0dddd8f4b06f43dd2acd69f04bfb9e1b44a1f1f078f17e040948f7006e564535a883cf2db9ab9304e4aad87a84```

#### iBoot.n61.RELEASE.im4p
```224fa3ce3935841878155d498692db684e863029e89040ab81d753b0a3c0b43e671585339249b59a642e1527b016835c```

## iPhone SKDs
[sdks](https://github.com/okanon/iPhoneOS.sdk)