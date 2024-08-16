#!/bin/bash

#FUNTIONS

get_processors_info_fun(){
	command="dmidecode -t 4"
	#echo $command
	result=$($command 2>/dev/null)
	#echo $result

	#WHILE FOR READING RESULT
	while read -r line; do
		#echo $line
		#Salto las primeras 3 lineas o si estan vacias
		if [[ $line = *dmidecode* ]] || [[ $line = *SMBIOS* ]] || [[ $line = "Processor Information" ]] || [[ $line = "" ]]; then
			continue
		fi
		
		#Ciclo dentro de cada "Process device"
		while read -r line; do
			#Verifico si es la primera linea que contiene el HANDLE del device
			#echo $line
			if [[ $line = *"DMI type 4"* ]]; then
				IFS=',' read -r -a array <<< "$line"
				device=${array[0]}
				command2="`dmidecode -t 4 | grep "$device" -A 15 | grep -c 'Unpopulated'`" #Check if there is module installed
				
				if [ $command2 -gt 0 ]; then
					continue
				else
					echo PROCESSOR"|"$device"||"
				fi
			#Caso contrario solamente verifico si son los atributos que interesan
			elif [[ $line = "Manufacturer: "* ]] || [[ $line = "Version: "* ]] || [[ $line = "Max Speed: "* ]] || [[ $line = "Socket Designation: "* ]] || [[ $line = "Signature: "* ]] && [[ $command2 -eq 0 ]]; then
				#Proceso info de cada slot
				IFS=':' read -r -a array <<< "$line"
				attribute=${array[0]}
				value=${array[1]}
				echo PROCESSOR"|"$device"|"$attribute"|"$value
			fi
		done <<< "$line"
	done <<< "$result" #END WHILE
}

get_memories_slots_not_used_fun(){
	command="dmidecode -t 17"
	#echo $command
	result=$($command 2>/dev/null)
	#exitcode=$?
	#echo $result
	
	value=0
	#WHILE FOR READING RESULT
	while read -r line; do
		if [[ $line = "Size: No Module Installed" ]]; then
			value=$((value+1))
		fi
	done <<< "$result" #END WHILE
	echo "$value"
}

get_memories_info_fun(){
	command="dmidecode -t 17"
	#echo $command
	result=$($command 2>/dev/null)
	#echo $result

	#WHILE FOR READING RESULT
	while read -r line; do
		#echo $line
		#Salto las primeras 3 lineas o si estan vacias
		if [[ $line = *dmidecode* ]] || [[ $line = *SMBIOS* ]] || [[ $line = "Memory Device" ]] || [[ $line = "" ]]; then
			continue
		fi
		
		#Ciclo dentro de cada "Memory device"
		while read -r line; do
			#Verifico si es la primera linea que contiene el HANDLE del device
			#echo $line
			if [[ $line = *"DMI type 17"* ]]; then
				IFS=',' read -r -a array <<< "$line"
				device=${array[0]}
				command2="`dmidecode -t 17 | grep "$device" -A 6 | grep -c 'No Module Installed'`" #Check if there is module installed
				
				if [ $command2 -gt 0 ]; then
					continue
				else
					echo MEMORY"|"$device"||"
				fi
			#Caso contrario solamente verifico si son los atributos que interesan
			elif [[ $line = "Locator: "* ]] || [[ $line = *"Bank Locator"* ]] || [[ $line = "Size: "* ]] || [[ $line = "Manufacturer: "* ]] || [[ $line = "Part Number: "* ]] || [[ $line = "Speed: "* ]] || [[ $line = "Type: "* ]] && [[ $command2 -eq 0 ]]; then
				#Proceso info de cada slot
				IFS=':' read -r -a array <<< "$line"
				attribute=${array[0]}
				value=${array[1]}
				echo MEMORY"|"$device"|"$attribute"|"$value
			fi
		done <<< "$line"
	done <<< "$result" #END WHILE
}

#Invoke funtion get_memories_slots_not_used_fun()
MOTHER_TOTAL_SLOTS_NOT_USED=$(get_memories_slots_not_used_fun) 

#CABECERAS
echo Family"|"Id"|"Attribute"|"Value

MOTHER_MANUFACTURER=`dmidecode -t 2 | awk -F':' '/Manu/{print $2}'`
MOTHER_MODEL=`dmidecode -t 2 | awk -F':' '/Product/{print $2}'`

echo BIOS"|MOTHER|"Manufacturer"|"$MOTHER_MANUFACTURER
echo BIOS"|MOTHER|"Model"|"$MOTHER_MODEL

get_processors_info_fun

MOTHER_TOTAL_SLOTS=`dmidecode -t 16 | awk -F':' '/Number Of Devices/{print $2}'`
MOTHER_MAX_CAPACITY=`dmidecode -t 16 | awk -F':' '/Maximum Capacity/{print $2}'`
MEMORYSLOTS=$(($MOTHER_TOTAL_SLOTS-$MOTHER_TOTAL_SLOTS_NOT_USED))

echo BIOS"|MEMORYSLOTS|"MaximumCapacity"|"$MOTHER_MAX_CAPACITY
echo BIOS"|MEMORYSLOTS|"TotalSlots"|"$MOTHER_TOTAL_SLOTS
echo BIOS"|MEMORYSLOTS|"UsedSlots"|" $MEMORYSLOTS

get_memories_info_fun
