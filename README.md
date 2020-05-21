# STM32_RFSwitch
12V RF switch/DC relay controller (7 channels) and open-drain button press emulator (6 channels) with switchable USB2.0/USB3.0 ports with independent voltage/current monitoring. Includes 2 thermistor channels, and 2 spare ADC, 2 spare GPIO, and 1 spare GPI channel.

Zip file contains Gerbers ready for fab (2-up panel), and xlsx files contain BOM/component placement list also needed for fab by JLCPCB SMT service. Estimated cost is $61.77 for PCBA including DHL shipping (10 boards total, 5 panels), plus $18.12 for additional components from LCSC (through hole headers, inductor, USB connectors), and $16.86 for the STM32 "blue pill" microcontroller boards, for a total of $96.75 ($9.68 per board).

Usage:

RF Switch: r[1-2]

DPDT Relay: l[1-2]

Open Drain Output: o[1-2] l[level: 0 = open, 1 = low, default 0] d[delay, x0.1s, deafult 10 (1s)]

Cal R/W Mode: t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]

Version: v

Status: s

Soft Reset: z

Help (Prints this menu): ?

# Errata
The +/- input are swapped for the MCP6002 op-amp 2 (USB3.0 current measurement). The traces need to be cut and jumpered per the image below:

![HW Fix for v1.0](/v1p0_errata_fix.png)
