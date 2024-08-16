#!/bin/bash

echo IRQ"|"Device

awk -v OFS='|' '/eth/ || /eno/ || /enp/|| /ens/ || /wc/ || /wanpipe/ || /ide/ || /ata/ || /sas/ {sub(/\:$/,"",$1);print $1,$NF}' /proc/interrupts
