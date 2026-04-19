# CAI Collision Awareness Indicator (ESP-IDF Prototype)

This repository contains the embedded prototype for the CAI (Collision Awareness Indicator) concept: a pole-mounted traffic safety system that detects near-collision risk and triggers a visual warning.
THis project is build on blink example of ESP-IDF, which is a simple template for ESP32 projects. The code is organized in a single main file for simplicity, but can be refactored into multiple files and modules as needed.


## What this firmware does (2026-04-19)

Current firmware in `main/blink_example_main.c` implements a real-time proximity and collision-risk warning loop using a TF-Mini Plus LiDAR over UART.

It:

- reads TF-Mini frames at 115200 baud on UART2
- computes approach speed and time-to-collision (TTC)
- triggers alarm output when danger conditions are met
- keeps alarm active with a cooldown timer to avoid rapid flicker
- drives two GPIO outputs (one solid warning output and one blinking warning output)

The logic includes two trigger modes:

1. Immediate near-proximity mode ("jump scare") when object is <= 50 cm (for up to 3 s in-zone).
2. TTC mode when estimated time to collision drops below 2.0 s.

## Hardware and pin mapping

Defined in `main/blink_example_main.c`:

- TF-Mini UART TX pin: GPIO12 (`TXD_PIN`)
- TF-Mini UART RX pin: GPIO13 (`RXD_PIN`)
- UART port: `UART_NUM_2`
- Blinking warning output: GPIO5 (`BLINK_PIN`)
- Solid warning output: GPIO6 (`SOLID_PIN`)

Thresholds and timing:

- max tracked distance: 600 cm
- valid sensor range accepted: 1 to 1199 cm
- TTC danger threshold: < 2.0 s
- cooldown time: 4.0 s
- proximity window: <= 50 cm
- proximity timeout: 3.0 s

## Build and flash (ESP-IDF)
You can build and flash the firmware using ESP-IDF tools either visual Studio Code extension or command line.

From the repository root:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

Example port on Windows:

```bash
idf.py -p COM4 flash monitor
```

## Serial output

The firmware prints periodic distance logs and danger events such as TTC warnings.

Typical messages include:

- `Distance: <n> cm`
- `TTC DANGER! Dist: <n> cm, Speed: <n> cm/s`
- `Ignoring unrealistic approach speed: <n> cm/s`


```
Distance: 39 cm
Distance: 166 cm
Ignoring unrealistic approach speed: 1129 cm/s
Distance: 53 cm
Distance: 55 cm
Distance: 55 cm
Distance: 56 cm
Distance: 49 cm
TTC DANGER! Dist: 49 cm, Speed: 39 cm/s
Distance: 146 cm
TTC DANGER! Dist: 130 cm, Speed: 156 cm/s
TTC DANGER! Dist: 107 cm, Speed: 224 cm/s
Distance: 105 cm
Distance: 154 cm
Distance: 156 cm
Distance: 155 cm
Distance: 155 cm
Distance: 154 cm
Distance: 152 cm
```

## Optional plotting utility

`main/serial_plotter.py` provides a live serial distance plot using `pyqtgraph` and `pyserial`.

Update the serial port in the script if needed:

- `COM_PORT = 'COM4'`

Install dependencies:

```bash
pip install pyqtgraph pyserial pyqt5
```

Run:

```bash
python main/serial_plotter.py
```


# Hardware prototype
The current hardware prototype consists of:
- ESP32-S3-DevKitC-1 development board for the main controller and processing unit
- TF-Mini Plus LiDAR for distance measurement and collision risk estimation
- Breadboard and jumper wires for GPIO connections
- 4s 21700 lion battery pack for portable power
- usb c step-down power module to convert battery voltage to 5V for the ESP32, relays, 5v LED strip and TF-Mini Plus
- switch for tuning the system on and off
- 100w cop led with adjustable step-up driver for the visual warning output
- two 5v relay modules for controlling the LED warning output
- 5v Led strip for blinking warning output,(audio output can be added in parallel in the future)
## Wiring diagram
protoptype wiring diagram for the TF-Mini Plus LiDAR and GPIO outputs:
![wiring diagram](Notes_260419_174151_2.jpg)
picture 1, wiring diagram

## Functional prototype photos
![wiring](20260419_163823.jpg)
picture 2, wiring of the prototype
Everything is placed inside cardboard box for demo purposes. Carboard is easy to work with and provides a good platform to realize layout of electronics for more robust implementations. Box used is old filament box.
![esp32](20260419_165656.jpg)
picture 3, ESP32-S3-DevKitC-1 development board
Heart of the prototype is the ESP32-S3-DevKitC-1 development board, which provides the processing power and GPIO interfaces for the system. Wiring is build on a breadboard for easy prototyping and testing. Connections are glued with hot glue to secure them in place for field testing. And for easy removal when need to update hardware.
![cobcutout](20260419_163902.jpg)
picture 4, COB LED cutout cob led, heasink, coling fan and step-up are inside cardboard box. Cutout is made for the COB LED.

![lidar](20260419_163845.jpg)
picture 5, TF-Mini Plus LiDAR
TF-Mini Plus LiDAR is mounted on the top of the box to provide a clear line of sight for distance measurements. It is connected to the ESP32 via UART for real-time data acquisition. cutout was already in filament box.

![switch](Screenshot_20260419_235752_Gallery.jpg)
picture 6, switch for tuning the system on and off, maximun continous current of the switch is 6A, which is 14.4v*6A=86.4W, which is enough for the prototype, since the COB LED is 100W but we will run it at around 60% brightness to stay within the limits of the switch and thermals.

## Project vision and next steps

The broader CAI concept targets right-hook conflict prevention at intersections by combining:

- pole-mounted sensing (RGB + IR in concept)
- on-device collision prediction
- immediate crossing illumination warning

This repository currently hosts the embedded warning prototype and ESP-IDF project scaffolding for iterative field testing.