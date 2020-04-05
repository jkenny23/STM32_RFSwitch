# STM32_RFSwitch
RF switch and open-drain button press emulator controller

Usage:
RF Switch: 
r[1-2]
Open Drain Output:
o[1-2] l[level: 0 = open, 1 = low] d[delay, x0.1s]
         def.0,                      def.10 (1s)
Cal R/W Mode
t[r/w] a[address: 0-999] d[data, unsigned int (0-65535)]
Version
v
Status
s
Soft Reset
z
Help (Prints this menu)
?

Controls a Ducommun 2SE1T11JB RF SPDT relay (latching, active high) via 2N7002P (N-FET to pull P-FET gate low) + ME20P03 (P-FET to 12V source). Default switching pulse is 50ms (datasheet claims 20ms switching time).
Controls a device under test via 2 button press emulators consisting of 2N7002P (N-FET in open drain config).
Includes B6286 module for boosting 5V from USB to 12V for RF relay
