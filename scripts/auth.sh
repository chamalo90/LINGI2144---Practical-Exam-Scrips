#!/bin/bash
echo "Welcome !"
while true; do
	read -p "What's your key (hex 16-bytes) ? " k

	if [ ${#k} -eq 0 ]; then
		k="49 45 4d 4b 41 45 52 42 21 4e 41 43 55 4f 59 46" #BREAKMEIFYOUCAN!
	else
		limit=$((${#k}/2))
		k=$(echo -n $(echo "${k:0:$limit}" | rev)$(echo "${k:$limit:${#k}}" | rev) | xxd -p -u)
	fi
	echo "Using $k as key"
	key=$(echo "$k" | sed 's/[[:space:]]//g')
	if [ ${#key} -ne 32 ]; then
		echo "Error, key must be 16-byte long"
	else
		break
	fi
done

while true; do
	read -p "What challenge do you want to use ? (8-bytes) " c
if [ ${#c} -eq 0 ]; then
	c="1122334455667788"
	echo "Using $c as challenge"
fi
	challenge2=$(echo "$c" | sed 's/[[:space:]]//g')
	if [ ${#challenge2} -ne 16 ]; then
		echo "Error, challenge must be 8-byte long"
	else
		break
	fi
done

while true; do
{
	echo 'FF 00 00 00 04 D4 4A 01 00' | scriptor #>&1,2 /dev/null
	echo 'FF 00 00 00 04 D4 42 1A 00' | scriptor #>&1,2 /dev/null
	c=$(echo "FF C0 00 00 0E" | scriptor | grep '< D5' | sed 's/< D5 43 00 AF //; s/ 90 00 : Normal processing.//')
} &> /dev/null
	if [ ${#c} -eq 0 ]; then
		read -p "Launch the auth process. What's the challenge given by the card ? " c
	fi
	challenge1=$(echo "$c" | sed 's/[[:space:]]//g')
	if [ ${#challenge1} -ne 16 ]; then
		echo "Error, challenge must be 8-byte long"
	else
		break
	fi
done

l1=${#challenge1}
l2=${#challenge2}

shift=2
challenge1bis=${challenge1:$shift:$l1-$shift}${challenge1:0:$shift}
challenge2bis=${challenge2:$shift:$l2-$shift}${challenge2:0:$shift}

dchallenge1=$(echo $challenge1 | xxd -p -r | openssl enc -des-ede-cbc -d -K $key -iv 0000000000000000 -nopad | xxd -p)
dchallenge1bis=${dchallenge1:$shift:$l1-$shift}${dchallenge1:0:$shift}

echallenge2=$(echo $challenge2$dchallenge1bis | xxd -p -r | openssl enc -des-ede-cbc -e -K $key -iv $challenge1 -nopad | xxd -p)

echo "Answer to the card: $echallenge2"

while true; do
{
	# Need to remove the space for scriptor (doesn't like the 2-way format)
	echo "ff00000013d442af$echallenge2" | scriptor
	c=$(echo "FF C0 00 00 0E" | scriptor | grep '< D5' | sed 's/< D5 43 00 00 //; s/ 90 00 : Normal processing.//')
	echo $c
} &> /dev/null
	if [ ${#c} -eq 0 ]; then
		read -p "What's the answer of the card ? " c
	fi
	cout=$(echo "$c" | sed 's/[[:space:]]//g')
	if [ ${#cout} -ne 16 ]; then
		echo "Error, challenge must be 8-byte long"
	else
		break
	fi
done

l=${#echallenge2}
iv=${echallenge2:l-16:l}
cout=$(echo $cout | xxd -p -r | openssl enc -des-ede-cbc -d -K $key -iv $iv -nopad | xxd -p)
if [ $cout == $challenge2bis ]; then
	echo "You're successfully authenticated ! =D *happy face*"
	
	case $1 in
		'scriptor')
			echo "==================================================================================="
			echo "Running scriptor shell"
			echo "==================================================================================="
			scriptor;;
		'')
			exit;;
		*)
			echo "==================================================================================="
			echo "Running scriptor with $1"
			echo "==================================================================================="	
			scriptor $1;;
	esac
else
	echo "You failed ! Noob!"
fi
