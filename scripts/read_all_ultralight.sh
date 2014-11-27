#!/bin/bash
type="UC"
if [ ${#1} -ne 0 ]; then
	type=$1
fi

case $type in
	'UCN')
		Max=44;; # 2C Last sector we want to read
	'UCA')
		Max=44 # 2C Last sector we want to read
		{
			echo "FF 00 00 00 04 D4 4A 01 00" | scriptor
		} &> /dev/null;;				
	'UC')
		Max=44 # 2C Last sector we want to read
		./auth.sh;;
	'U')
		Max=16 # 0F Last sector we want to read
		{
			echo "FF 00 00 00 04 D4 4A 01 00" | scriptor
		} &> /dev/null;;
esac

Count=0
if [ ${#3} -ne 0 ]; then
	Count=$3
fi
if [ ${#2} -ne 0 ]; then
	Max=$2
fi
while [ $Count -lt $Max ]; do
{
	echo "FF 00 00 00 05 D4 40 01 30 $(echo "$(($Count))" | awk '{printf "%2.2x", $0}')" | scriptor
	out=$(echo "FF C0 00 00 15" | scriptor | sed '/< D5/ s/.$/$$/g' | sed ':a;N;$!ba;s/$$\n/ /g' | grep '< D5' | sed 's/< D5 41 00 //; s/ 90 00 : Normal processing.//')
} &> /dev/null
	echo "0x$(echo "$(($Count))" | awk '{printf "%2.2x", $0}') : $out"
	Count=$(($Count+4))
done

