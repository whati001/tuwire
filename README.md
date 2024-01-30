# Tuwire

## General
This repository consolidates all the relevant documents and code from my master thesis with the title "Analysis of low-level iOS lightning protocols" passed at the [Technical University of Graz](https://www.tugraz.at/home/).

## Topic
The thesis is structured into three parts:

### Analysis
At first, we investigated into Apple's proprietary low level iOS lightning protocols and how we can abuse these to attack an iPhone during the charging process. These class of attacks are called "Juice Jacking Attacks" by the academic research. 

### Attack
After identifying potential weaknesses in these proprietary protocols, we assembled a malicious charging cable called `TuWire`. This cable allows an attack to control the device’s user interface, extract the access credentials for the WiFi network the iPhone is currently connected to, enroll an external Mobile Device Management (MDM) profile, and stream the device screen content to a storage device. 

### Mitigation
To mitigate Juice Jacking attacks such as `TuWire` the thesis also provides a Lightning-to-Lightning adapter called `Lightning Condom`, which forces any malicious charging cable to act as a charging-only cable. Therefore, it mitigates attacks such as `TuWire` and delivers a secure charging interface to users.


## Thesis publication
Please find the final thesis publication within the universities repository; [direct link](https://repository.tugraz.at/publications/znszh-d4t88)

## Demonstration
Please find here some demonstration videos uploaded to YouTube.

### TuWire
[YouTubeLink](https://youtu.be/imAbaG3W6TI?feature=shared)

### Lightning Condom
[YouTubeLink](https://youtu.be/olzWqD0d64o?feature=shared)

## Code
Please be patient, I am still waiting for the official OK from the university to be allowed to share my code.

But what can you expect:
* IDBUS implementation for the Raspberry Pi Pico (POC for Zephyr RTOS)
* iAP implementation for the Raspberry Pi Pico (POC for Zephyr RTOS)
* MFI chip communication for the Raspberry Pi Pico (POC for Zephyr RTOS)
* Nero (USB Protocol) implementation for Zephyr RTOS (stm32) 