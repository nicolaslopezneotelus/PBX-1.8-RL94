#!/bin/bash

# Define SOURCEPATH
SOURCEPATH="/usr/src/PBX-1.8-RL94"

# Define the JSON file path
JSON_FILE="$SOURCEPATH/scripts/directoryStructure.json"

# Function to recursively process the JSON structure
process_json() {
  local json=$1
  local base_path=$2
  local total_keys=$(echo $json | jq -r 'keys | length')
  local index=0

  for key in $(echo $json | jq -r 'keys[]'); do
    index=$((index + 1))
    local item_path="$base_path/$key"
    local item=$(echo $json | jq -r --arg key "$key" '.[$key]')

    if [ "$item" = "file" ]; then
      # It's a file, copy it and set permissions
      mkdir -p "$(dirname "$item_path")"
      cp -f "$SOURCEPATH/builded/$item_path" "$item_path"
      chmod 755 "$item_path"
      echo "[$index/$total_keys] Copiado: $SOURCEPATH/builded/$item_path a $item_path y permisos asignados."
    else
      # It's a directory, recurse into it
      process_json "$item" "$item_path"
    fi
  done
}

# Read the JSON structure and start processing from the root
json_structure=$(cat "$JSON_FILE" | jq '.')
process_json "$json_structure" ""

echo "Archivos copiados y permisos asignados."
