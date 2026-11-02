# Embedded integration design: three distinct roles

## Selected devices and scope

NUCLEO-F401RE (STM32F401RE MCU), Digilent Basys 3 (Artix-7 FPGA), and a SEGGER
J-Link probe with a standard ARM 20-pin interface. These are **design targets**;
all discovery, status and telemetry in the application are simulated. The probe
is a JTAG-capable debug adapter, used in SWD mode for the MCU in this design.
It translates host debug commands; it is not interchangeable with a compute
board. The FPGA uses its onboard USB programming path, not an assumed J-Link
configuration path. Real FPGA vendor tool support has not been implemented.

## Labelled system connections

See Hardware_Block_Diagram.png for the block diagram. The host owns three
independent logical USB adapters. MCU USB provides power and virtual serial
telemetry; FPGA USB provides board power, UART and its onboard programming path.
J-Link USB connects the debug probe to the host; its target side connects only
to the MCU debug port after isolating the onboard debug driver. There is no
required MCU-to-FPGA GPIO cable. This intentionally avoids incompatible pinouts.

| Interface | Power/domain | Signals / connector | Owner and assumptions |
|---|---|---|---|
| Host to NUCLEO-F401RE | USB 5 V input; board regulator supplies 3.3 V MCU domain | Board USB connector and onboard ST-LINK virtual COM | Use board revision's documented USB-power jumper configuration; no second power source |
| Host to Basys 3 | USB power input; board regulates its own internal rails; expansion IO assumed 3.3 V only after revision check | Board USB programming/UART port | Onboard bridge owns FPGA JTAG; real counter telemetry would need a user bitstream |
| Host to J-Link | USB powers probe; target reference sensed, not supplied by VTref | Host USB; standard 20-pin ARM target cable assumed | Exact probe model/connector must be checked; some models use different connectors |
| J-Link to MCU, SWD | 3.3 V target reference | Pin 1 VTref to target VDD; pin 7 SWDIO to PA13; pin 9 SWCLK to PA14; pin 15 reset to NRST; pin 4 GND to target GND | SWCLK driven by probe; SWDIO bidirectional; reset controlled per probe spec; isolate onboard ST-LINK first |
| Generic JTAG ownership (not connected to FPGA here) | Match target IO voltage | Probe TCK pin 9, TMS pin 7, TDI pin 5 -> target; target TDO -> probe pin 13 | One driver per signal. nTRST is target-dependent. These are ARM probe pins, NOT Basys header pins |
| Reference and power exclusions | Common ground required on each debug signal path | Probe pin 19 (optional target power output on some models) deliberately left disconnected | VTref is a sense/reference input, not a power supply. Do not join independent 5 V sources |
| Optional future GPIO between boards | Only after verifying both IO domains | Board-specific headers; no proposed universal connector | If levels differ, use direction-appropriate level translators and account for power sequencing; no direct 5 V to FPGA IO |

MCU target pads/header positions must be verified against the actual MB1136 board
revision and schematic; the design specifies signal names PA13/PA14/NRST rather
than guessing a cable-compatible header. Probe cable pin 1 orientation must be
verified. Basys 3 board-level pin numbers are deliberately not assumed to match
an ARM debug cable. No physical wiring should be inferred from mock discovery.

## Safe sequence

1. Record exact board/probe model and revision; read matching manuals and pinouts.
2. With power off, choose one power source per board and one debug owner. Isolate
   the onboard ST-LINK drive path before using the external probe.
3. Check common ground and continuity, connector orientation, target VDD and IO
   voltage. Never connect probe power output to a board already powered by USB.
4. Verify VTref within the selected probe's supported range. SWD is not JTAG;
   configure the correct protocol and a low initial clock rate.
5. Connect/read device ID without programming. Enable one telemetry channel,
   handle timeouts, and disconnect cleanly. Only then consider firmware work.
6. Keep FPGA programming through its documented onboard path. Do not attach two
   JTAG drivers, or assume a J-Link can configure every FPGA.

## Mock state and recovery

Discovery returns exactly three labelled mock records with capabilities.
Disconnected -> Connect -> Connected -> Telemetry -> Disconnect. Inject timeout
sets a one-shot failure flag; the next read reports a simulated 250 ms timeout
budget without actually waiting. The flag clears, the link remains connected,
and the next read succeeds with an incremented sequence number. This is a
failure simulation, not a measurement of USB latency. No physical device is read.

## Official references checked for design

- ST UM1724 Nucleo-64 MB1136 user manual:
  https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf
- ST STM32F401RE datasheet (PA13/PA14 debug functions):
  https://www.st.com/resource/en/datasheet/stm32f401re.pdf
- SEGGER interface description and 20-pin connector:
  https://www.segger.com/products/debug-probes/j-link/technology/interface-description/
  https://kb.segger.com/20-pin_J-Link_Connector
- Digilent official manual URL for revision verification:
  https://digilent.com/reference/programmable-logic/basys-3/reference-manual
  Direct automated retrieval returned 403 in this session; verify the revision
  locally before using this proposed physical design. No pin-number claims are
  made for the FPGA's external connector.
