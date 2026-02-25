# Celio-Firmware

## TL;DR

This firmware is required to build a **Celio-Device**.

- The latest firmware revision can be found under **Releases**.
- Currently, only the **Raspberry Pi RP2040** is supported.

### Flashing the Firmware

To flash the RP2040:

1. Hold the **BOOTSEL** button.
2. Connect the device to your computer.
3. Drag and drop the firmware `.uf2` file onto the mounted device.

---

## Game Boy Advance Connectivity

The pinout is configured to work with:

- smashstacking's GBA to USB Adapter  
  https://www.youtube.com/watch?v=KtHu693wE9o  

You have several hardware options:

- Order a PCB from JLCPCB (or another PCB manufacturer):  
  https://github.com/agtbaskara/game-boy-pico-link-board

- Buy a prebuilt board from Etsy (not affiliated):  
  https://www.etsy.com/de/listing/1517956485/gb-link-usb-zu-gameboy-link-adapter-fur

- Compatible GBA link plug board:  
  https://github.com/weimanc/game-boy-zero-link-board

---

## Link Cable Requirements

This firmware has only been tested with **original Nintendo GBA Link Cables**.  
Reproduction cables should work as well.

⚠ Important: GBA link cable connectors are **not identical**.

- Slim connector = **Master**
- Wide connector = **Slave**

The Celio-Device **must** be connected to the **master connector**.

---

# Overview

This repository contains the firmware source code for the Celio-Device.

The firmware allows the device to connect to a Game Boy Advance as either:

- **Master** (Link Cable pinout default)
- **Slave** (by pulling SO up)

This makes it possible to implement the Pokémon Generation 3 Link Trading Protocol    
and perform trades or battles using real hardware.

For online connectivity, one system assumes the role of the master while the partner system acts as the slave.    
During synchronization, both sides keep the in-game trade session in an idle state until    
packets from the remote partner arrive. The firmware forwards and injects link data in real time,    
effectively allowing two physically separate Game Boy Advance systems to behave as if they were    
connected by a direct link cable.

In this setup, a Celio-Device act as a “dummy” Game Boy Advance. It injects packets into the ongoing trade while   
simultaneously capturing packets from its connected console and forwarding them to the remote partner.    
This packet-level bridging enables stable online trades and battles on original hardware.

---

# Design

The project is primarily written in **C++**.

While it avoids many advanced language features, it heavily relies on:

- **RAII patterns** for safe state management
- Scoped objects to ensure proper cleanup of link session components

This makes it easier to manage complex state transitions during link sessions
without leaving components in undefined states.

---

## Zephyr RTOS

This project uses **Zephyr RTOS**:

https://www.zephyrproject.org/

Originally, the target MCU was an STM32F07.  
Thanks to Zephyr, migrating to the RP2040 required only minimal to moderate effort.

Zephyr provides most low-level abstractions.  
The only MCU-specific implementation resides in: ```/src/layers/linkLayer_.c```   
This file implements the low-level bit-layer handling of the link protocol.

To port this project to another MCU:

1. Ensure the MCU is supported by Zephyr.
2. Implement a new `linkLayer`.
3. Provide a master clock implementation.

The remaining firmware is largely MCU-agnostic.

Currently built against:

- **Zephyr 3.7.99**

Newer versions should also be compatible.

---

# Known Issues

In rare cases, the firmware may not respond to a mode-switch command.

If this happens, reconnect the device.

The device should function normally afterward.
