# SolarChargeLogger
Logs battery voltage and charge current from a solar cell into a lead acid battery to an SD card and optionally the serial output.

To measure voltage and current a voltage divider is placed across the voltage as defined by R1 and R2 defines.
Current is measured with a current shunt of resistance specefied in the defines.
Voltage is measured in millivolts to increase precision without floating point math.
Current is in Ma for the same reason. a smaller unit may be worthwhile.
