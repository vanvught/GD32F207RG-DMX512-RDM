#!/bin/bash

if [ $# -lt 2 ]; then
  echo "Usage: $0 <size_file> <linker_script>"
  exit 1
fi

size_file="$1"
linker_script="$2"

used_data=$(grep '.data' "$size_file" | tail -n 1 | awk '{print $2}')
used_bss=$(grep ".bss" "$size_file" | awk '{print $2}')

total_ram=$(grep "RAM (xrw)" "$linker_script" | awk '{print $NF}' | sed 's/K$//' | awk '{printf "%d", $0 * 1024}')
unused_ram=$(( $(echo $total_ram) - $(echo $used_data) - $(echo $used_bss) ))

used_stack=$(grep ".stack" "$size_file" | awk '{print $2}')
used_heap=$(grep ".heap" "$size_file" | awk '{print $2}')

total_ram1=$(grep "RAM1 (xrw)" "$linker_script" | awk '{print $NF}' | sed 's/K$//' | awk '{printf "%d", $0 * 1024}')
unused_ram1=$(( $(echo $total_ram1) - $(echo $used_stack)- $(echo $used_heap) ))

used_ram2=$(grep ".sram2" "$size_file" | awk '{print $2}')

total_ram2=$(grep "RAM2 (xrw)" "$linker_script" | awk '{print $NF}' | sed 's/K$//' | awk '{printf "%d", $0 * 1024}')
unused_ram2=$(( $(echo $total_ram2) - $(echo $used_ram2) ))

cat $1
echo "SRAM0 $total_ram bytes, Unused: $unused_ram bytes"
echo "SRAM1 $total_ram1 bytes, Unused: $unused_ram1 bytes"
echo "SRAM2 $total_ram2 bytes, Unused: $unused_ram2 bytes"
echo 