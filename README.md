# STM32_RFSwitch
12V RF switch/DC relay controller (7 channels) and open-drain button press emulator (6 channels) with switchable USB2.0/USB3.0 ports with independent voltage/current monitoring. Includes 2 thermistor channels, and 2 spare ADC, 2 spare GPIO, and 1 spare GPI channel.

Zip file contains Gerbers ready for fab (2-up panel), and xlsx files contain BOM/component placement list also needed for fab by JLCPCB SMT service. Estimated cost is $61.77 for PCBA including DHL shipping (10 boards total, 5 panels), plus $18.12 for additional components from LCSC (through hole headers, inductor, USB connectors), and $16.86 for the STM32 "blue pill" microcontroller boards, for a total of $96.75 ($9.68 per board).

Usage:

RF Switch: r[1-7]

USB2.0 Power Control: l[1-2, 1=off, 2=on]

USB3.0 Power Control:  m[1-2, 1=off, 2=on]

Open Drain Output: o[1-6] l[level: 0 = open, 1 = low, default 0] d[delay, x0.1s, deafult 10 (1s)]

Cal R/W Mode: t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]

Version: v

Status: s

Soft Reset: z

Help (Prints this menu): ?

# Compatible RF Switches

| RF Switch P/N | RF Switch Mfg. | RF Switch Config | # IOs | CH/CL | Notes                                                             | RF Switch Datasheet                                                                        | Approx. Price         |
|---------------|----------------|------------------|-------|-------|-------------------------------------------------------------------|--------------------------------------------------------------------------------------------|-----------------------|
| 2SE1T11JB     | Ducommun       | SPDT             | 2     | CL    |                                                                   | https://www.ducommun.com/pdf/mwcat/2s.pdf                                                  | $18.75-20             |
| 431-420823    | Dow-Key        | SP3T             | 3     | CH    | Remove pull up resistor (R5/R15/R19/R25/R38/R47/R55) for IOs used | https://www.dowkey.com/wp-content/uploads/2015/06/_plk49_1_431461_Latching_CatalogPage.pdf | ~$36 (similar models) |

# Errata
The +/- input are swapped for the MCP6002 op-amp 2 (USB3.0 current measurement). The traces need to be cut and jumpered per the image below:

![HW Fix for v1.0](/v1p0_errata_fix.png)
