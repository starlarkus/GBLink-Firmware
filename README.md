# GBLink Firmware — RP2040 Game Boy Link Adapter

This firmware turns an RP2040 into a USB-to-Game Boy Link Cable adapter supporting both **Game Boy (Gen 1/2)** and **Game Boy Advance** communication over WebUSB.

This is forked from Celio Firmware from the CelioLink project and adds the functionality of the GBLink Reconfigurable to allow for Tetris, Pokemon gen1-3 trading. Sending multiboot files to GBA, GB Printer emulator as well as the Celio Gen3 modes for direct gen3 GBA link.

## Quick Start

1. Download `gblink.uf2` from the latest **[Release](../../releases)**.
2. Hold the **BOOTSEL** button on the RP2040.
3. Connect the device to your computer.
4. Drag and drop the `.uf2` file onto the mounted drive.

---

## Supported Modes

| Mode | Command | Description |
|:---|:---|:---|
| **GBA Trade Emu** | `0x00` | Emulate a GBA link partner for Pokémon Gen 3 trades |
| **GBA Link** | `0x01` | Bridge two GBA systems over the internet |
| **GB Link** | `0x02` | SPI passthrough for Game Boy |
| **GB Printer** | `0x31` | Game Boy Printer emulation (bit-bang SPI slave) |

---

## Hardware Required

* **USB Adapter Board:** Raspberry Pi Pico or Waveshare RP2040-Zero, Solderless board in development. Pre built DIY adapter or kits are avalible on Etsy. https://www.etsy.com/listing/1517956485/gb-link-usb-to-gameboy-link-adapter-for 

### Wiring Guide

The firmware uses the RP2040 PIO to communicate with the Game Boy. Connect the Link Cable wires as follows:

| Game Boy Signal | RP2040 Pin |
|:---|:---|
| **SCK** (Clock) | **GP0** |
| **SIN** (Data In) | **GP1** |
| **SOUT** (Data Out) | **GP2** |
| **SD** (Chip Select) | **GP3** |
| **GND** | **Ground** |

Additional hardware pins:

| **Voltage 3.3V** | **GP11** | Pull low for 3.3V 
| **Voltage 5V** | **GP12** | Pull low for 5V
| **WS2812 LED** | **GP16** | NeoPixel status indicator

## To avoid damaging the board do not pull both GP11 and GP12 low at the same time

---

## LED Status Indicators

The onboard WS2812 RGB LED (GP16) indicates the current status:

| Color | Status |
|:---|:---|
| **Red** | Disconnected — Device powered, USB not enumerated |
| **Green** | Mounted — USB connected to host |
| **Blue** | Active — WebUSB session active |
| **Purple** | Printer Mode — GB Printer emulation active |

LED color can also be set via USB command (`0x42`).

---

## Link Cable Requirements

**GBA link cable connectors are not identical: Cheap 3rd party GBA cables missing the hub typicall only have 4 conductors are are missing a groud which result in a poor connection**

- **Slim** connector = **Master**
- **Wide** connector = **Slave**

The device **must** be connected to the **master connector** for GBA modes.

---

## USB Command Protocol

Commands are sent over the WebUSB command endpoint:

| Range | Module | Commands |
|:---|:---|:---|
| `0x00–0x0F` | Control | `0x00` SetMode, `0x01` Cancel |
| `0x10–0x1F` | GBA Link | SetModeMaster, SetModeSlave, StartHandshake, ConnectLink |
| `0x20–0x2F` | GBA Emu | *(internal section commands)* |
| `0x30–0x3F` | GB Link | `0x30` SetTimingConfig, `0x31` EnterPrinter, `0x32` ExitPrinter |
| `0x40–0x4F` | Hardware | `0x40` Voltage3V3, `0x41` Voltage5V, `0x42` SetLEDColor |

---

## Troubleshooting

- When refreshing the web page, the USB device may need to be reset (unplug or press reset button).
- On Linux, you may need to edit udev rules: https://stackoverflow.com/questions/30983221
- In rare cases, the firmware may not respond to a mode-switch command — reconnect the device.
