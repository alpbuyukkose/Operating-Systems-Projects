#!/bin/bash
# With help of " ", program can even take spaced name txt files. Ex: "my story1.txt"
input_file="$1"

if [ -f "$input_file" ]; then
	echo -n "$input_file exists. Do you want it to be modified? (y/n): "
	read -n 1 input
	echo

	if [ -z "$input" ]; then
		echo "Input can not be NULL! Exiting program..."
		exit 1
	fi

	if [ $input = y ]; then
		:
	elif [ $input = n ]; then
		echo "You enter n. Exiting program..."
		exit 1
	else
		echo "Input should be 'y' or 'n'. Exiting program..."
		exit 1
	fi
fi

random1=$(shuf -e 1 3 5 -n 1)
random2=$(shuf -e 1 3 5 -n 1)
random3=$(shuf -e 1 3 5 -n 1)

#echo $random1
#echo $random2
#echo $random3

satir1=$(sed -n "${random1}p" giris.txt)
satir2=$(sed -n "${random2}p" gelisme.txt)
satir3=$(sed -n "${random3}p" sonuc.txt)

echo $satir1 > "$input_file"
echo >> "$input_file"
echo $satir2 >> "$input_file"
echo >> "$input_file"
echo $satir3 >> "$input_file"

echo "A random story is created and stored in ${input_file}."
