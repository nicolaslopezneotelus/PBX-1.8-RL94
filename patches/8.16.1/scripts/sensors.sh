#!/bin/bash

#acpitz-virtual-0
#Adapter: Virtual device
#temp1:        +27.8°C  (crit = +105.0°C)
#temp2:        +29.8°C  (crit = +105.0°C)

#coretemp-isa-0000
#Adapter: ISA adapter
#Physical id 0:  +28.0°C  (high = +80.0°C, crit = +100.0°C)
#Core 0:         +24.0°C  (high = +80.0°C, crit = +100.0°C)
#Core 1:         +27.0°C  (high = +80.0°C, crit = +100.0°C)
#Core 2:         +27.0°C  (high = +80.0°C, crit = +100.0°C)
#Core 3:         +27.0°C  (high = +80.0°C, crit = +100.0°C)

echo ID"|"Name"|"Temperature

sensors | awk -F"[+.]" -v c="^Core" '$0~c{gsub(/[[:space:]]*/,"",$1);printf("|%s|%s\n",$1,$2)}'
