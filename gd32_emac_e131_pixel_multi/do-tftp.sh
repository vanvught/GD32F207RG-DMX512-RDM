#!/bin/bash
export PATH=./:../../scripts:$PATH

if [ -f "gd32f20x.bin" ]; then
cp gd32f20x.bin gd32.bin
else
exit
fi

echo '!tftp#1' | udp_send $1 
echo '?tftp#' | udp_send $1 

tftp $1 << -EOF
binary
put gd32.bin
quit
-EOF

rm gd32.bin

echo '!tftp#0' |udp_send $1 
echo '?tftp#' | udp_send $1 
sleep 1
echo -e "Rebooting..."
echo '?reboot##' | udp_send $1 

ON_LINE=$(echo '?list#' | udp_send $1 ) || true

while  [ "$ON_LINE" == "" ]
 do
  ON_LINE=$(echo '?list#' | udp_send $1 )  || true
done

echo -e "[$ON_LINE]"
echo -e "$(echo '?version#' | udp_send $1 )"
