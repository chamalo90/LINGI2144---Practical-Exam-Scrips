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

Msg="00000000"
if [ ${#3} -ne 0 ]; then
	Msg=$( echo $3 | sed -e :a -e 's/^.\{1,7\}$/&0/;ta')
fi

Start="04"
if [ $(echo "ibase=16; ${2^^}" | bc) -gt 4 ]; then
	if [ ${#2} -eq 1 ]; then
		Start=$(echo $"0${2^^}")
	elif [ ${#2} -eq 2 ]; then
		Start=$(echo ${2^^})
	fi
fi

echo "Writing FF 00 00 00 15 D4 40 01 A0 $Start $Msg 00*{12}"
{
	echo "FF 00 00 00 15 D4 40 01 A0 $Start $Msg 00 00 00 00 00 00 00 00 00 00 00 00" | sed 's/[[:space:]]//g' | scriptor
	out=$(echo "FF C0 00 00 05" | scriptor | sed '/< D5/ s/.$/$$/g' | sed ':a;N;$!ba;s/$$\n/ /g' | grep '< D5' | sed 's/< D5 41 //; s/ 90 00 : Normal processing.//')
} &> /dev/null
echo $out
