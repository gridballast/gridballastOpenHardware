# Grid Ballast User Guide
## Contents

  - [Setup](#setup)
    - [System Overview](#system-overview)
    - [Hardware](#hardware)
    - [Installation](#installation)
    - [Getting Started - Toolchain](#getting-started-toolchain)
    - [Runtime Network Configuration](#runtime-network-configuration)
    - [User Interface](#user-interface)
    - [OpenChirp Setup](#openchirp-setup)
  - [Technical](#technical)
    - [Overview](#overview)
    - [Specifications](#specifications)
    - [Firmware](#firmware)
    - [Cloud Architecture](#cloud-architecture)

## Setup
### System Overview
At a high-level, the Grid Ballast hardware can be used for supporting two classes of devices: 

**1. Thermostatically controlled loads (TCLs)** :TCLs like waterheaters, pool heaters, refrigerators, and HVAC systems, can useset-point manipulation to store energy or delay usage.
**OR**
**2.Miscellaneous plug loads**:  Plug loads like lighting and appliances (washer, dryer,dishwasher, etc.) typically only support binary control, but theiruse may be delayed and their usage patterns can be of benefit toutilities.


![Grid Ballast System Overview](https://drive.google.com/uc?id=1ldYqxVcLx1bcikpH_dUn3qW0ELpvCO4y)

_Grid Ballast System Overview_

### Hardware
The Grid Ballast hardware platform consists of the following hardware:
- _Main Controller Board_
- _CTA-2045 Board_: to control appliances with built-in CTA-2045 interface
- _Relay Board_: to control miscellaneous plug loads using hardware retrofit
- _Micro USB cable_: to flash firmware
- _Ethernet Cable_: to connect Controller board and CTA-2045 board using RJ45 interface

### Installation
**Assembly with CTA-2045 based Electronic Thermostat**
![APCOM EC-100 Wiring Diagram](https://drive.google.com/uc?id=1ak81TumP_04oUwI7fcUFu7eh4NQw7jtx)

<img src="https://drive.google.com/uc?id=1LmHvCMrnSSP7TucM5WkaKqPbrtBqIP54" width="300" >
_Grid Ballast Test System implemented on a residential Rheem Water Heater(cta 20145 enabled)_


**Assembly with mechanical retrofit**
![Mechanical Retrofit](https://drive.google.com/uc?id=1BnsPsl4xmlaQlow4bhHvJaPfrGIFJkTr)



### Getting Started - Toolchain

1. Install the `xtensa-esp32-elf` toolchain by following the instructions here: http://esp-idf.readthedocs.io/en/latest/get-started/#setup-toolchain.
   Make sure the `xtensa-esp32-elf/bin` is added to your `PATH`.
1. Clone the repository
    ```
    $ git clone --recursive https://github.com/WiseLabCMU/gridballast.git
    ```
1. Setup the `IDF_PATH` environment variable by adding the following to your `~/.profile`:
    ```
    export IDF_PATH="<path/to/gridballast>/Support/esp-idf"
    ```
1. Build the controller module
    ```
    $ cd Source/framework
    $ make
    ```
    In the framework configuration, navigate to "Serial flasher config" and select the default serial port. This can be determined by checking which device appears in `/dev` when the controller module is connected via USB. It should start with `/dev/ttyUSB`.
    Save and exit the configuration interface.
1. Flash the controller
    ```
    $ make flash
    ```
    
### Runtime Network Configuration
The WiFi SSID and password can be configured at runtime (without recompiling and reflashing the module). Other parameters could be added as necessary.
Configuration parameters are stored in flash using the [NVS Library](http://esp-idf.readthedocs.io/en/latest/api-reference/storage/nvs_flash.html).
On initialization of the wifi module, if stored wifi configuration parameters exist, they are loaded and used to connect to wifi.
If no parameters exist, the module launches configuration mode.

In configuration mode, the module broadcasts a wifi network named "gridballast". A user can connect to the gridballast network and open
http://[module-ip]/ (by default 192.168.1.4, can be found programatically with `tcpip_adapter_get_ip_info`) in a web browser. The module will present a webpage where the SSID and password can be configured.
To exit configuration mode, the module must be rebooted.

### User Interface

If the default code from the [GridBallast Github](https://github.com/WiseLabCMU/gridballast) is flashed into the board, the following user-selectable menus will be displayed on the LCD.

<img src="https://drive.google.com/uc?id=1cJ7Xsg-hxJUuCE0NnXOIjVgkVVjaiDUp" width="300" >
_Menu 1: Water Heater vitals_


<img src="https://drive.google.com/uc?id=1LqpXKeh5FiPumcsVbqxxMyJdve1ijnCR" width="300" >
_Menu 2: User selectable options_

<img src="https://drive.google.com/uc?id=1P8mGKNarrJHEOgEf8779BtQw5hYBVnhx" width="300" >
_Menu 3: Runtime Network Configuration_


### OpenChirp Setup

The OpenChirp platform can be used for control and visualization of the Controller parameters and measurements. Setup information can be found [here](https://github.com/OpenChirp/docs/wiki/time-series-database).

Here are snapshots of time series visualizations from OpenChirp for the Grid Ballast test system.

![Grid Frequency Measurement](https://drive.google.com/uc?id=1qCAgkg1X4H40S5nKz8r_ciVXNA5uIAXl)

![Top and Bottom Temperature measurements](https://drive.google.com/uc?id=1uNjZU_4bHxbKTq9LxONIebgns8BiGM72)

    
## Technical
### Overview
The open-source controller hardware is based around the Espressif Systems ESP32 microcontroller which has a connector for anexternal Raspberry Pi. The ESP32 is a single chip 2.4 GHz Wi-Fiand Bluetooth combination that provides a low-cost 240 MHz dualcore Tensilica LX6 processor with 520 KB of SRAM and 4 MByte offlash. The integrated wireless transceiver supports 802.11bgn witha stack that allows it to operate in both client, WiFi direct and APmodes to simplify installation. The chip comes with support forWEP, WPA/WPA2 PSK/Enterprise security with hardware accelerated encryption for AES / SHA2 / ECC and RSA-4096 standards.

![Hardware Architecture](https://drive.google.com/uc?id=1MIZUC6jlLwnzgi1W51_d52xUk7V-oMTr "Hardware Architecture")

_Hardware Architecture_

### Specifications
- Physical Dimensions:
- Power Supply: 100- 270 VAC at 50/60 Hz
- Frequency measurement: 45 - 65 Hz, Resolution of 4 s (user-definable)
- Power Consumption: average current 200mA @ 3.3 V
- Operating temperature: 
- Heating Coil Relay: 2 x 40 Amps at 240VAC
- Number of lifetime Switching Operations: 100,000 minimum at 1,800 operations/hr
- item



### Firmware
We use multiple tasks running on FreeRTOS to manage four mainsoftware tasks: water heater control, grid frequency estimation,user interaction, and communication. We developed a comprehen-sive library for different applications and multiple communication protocols (WiFi, LoRa and BLE) to interact with our Cloud-based management system. The firmware simplifies adding additionalfunctionality and interfaces while providing a GUI and connectivity options. Our software is available on [GridBallast Github](https://github.com/WiseLabCMU/gridballast/tree/softap-config-mode)  and compiled using a standard GCC toolchain.

![Firmware Architecture](https://drive.google.com/uc?id=1upI9KeSh2d6RMXwjEUGK_iklILIF1eGM "Firmware Architecture")
_Firmware Architecture_

### Cloud Architecture

In support of wide-area control of multiple water heaters, we leverage the [OpenChirp](https://openchirp.io/) framework for device management, data processing, and visualization. OpenChirp provides communication with end devices over multiple secure protocols, the facility to process data in real time, and the capabilities to manage groups of devices. In our design, the GridBallast controller communicates with OpenChirp using REST over WiFi and LoRaWANin a polling fashion. OpenChirp then stores this data in a time-series database and exposes it to other applications using REST andMQTT. The system also provides DeviceGroups that can be used to dispatch control commands across multiple water heaters.










