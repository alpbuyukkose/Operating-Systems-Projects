#!/bin/bash

alphabet=({a..z})
numbers=({0..9})
first_input="$1"
second_input="$2"

length_first=${#first_input}
length_second=${#second_input}

is_consistof_letter=false
is_consistof_number=false
is_length_correct=false

#checks whether inputs are null
if [ -z "$first_input" ] && [ -z "$second_input" ]; then
	echo "Both of the inputs is null!"
	exit 1
elif [ -z "$first_input" ]; then
	echo "First input is null!"
	exit 1
elif [ -z "$second_input" ]; then
	echo "Second input is null!"
        exit 1
fi


#checks whether first input consist of letters
index_letter=0
for (( i=0; i<$length_first; i++ )); do
        letter=${first_input:i:1}
        for j in ${alphabet[@]}; do
                if [[ $letter = $j ]]; then
                        ((index_letter++))
                fi
        done
done

if [ $length_first -eq $index_letter ]; then
        is_consistof_letter=true
fi

#checks whether second input consist of numbers
index_number=0
for (( i=0; i<$length_second; i++ )); do
        num=${second_input:i:1}
        for j in ${numbers[@]}; do
                if [[ $num == $j ]]; then
                        ((index_number++))
                fi
        done
done

if [ $length_second -eq $index_number ]; then
        is_consistof_number=true
fi

#checks second input is correct length
if [ $length_second -eq $length_first ]; then
	is_length_correct=true
elif [ $length_second -eq 1 ]; then
	is_length_correct=true
fi

#handles errors
if [ $is_consistof_letter = false ] && [ $is_consistof_number ]; then
	echo "Both input values are doesn't consist of their correct type."
	exit 1
fi

if [ $is_consistof_letter = false ]; then
	echo "The first input value doesn't consist of letters."
	exit 1
elif [ $is_consistof_number = false ]; then
	echo "The second input value doesn't consist of numbers."
	exit 1
elif [ $is_length_correct = false ]; then
	echo "The second input isn't in the correct size."
	exit 1
fi

#handles apple 12345 part
if [ $length_first -eq $length_second ]; then
	for (( i=0; i<$length_first; i++ )); do
        	current_digit=${second_input:i:1}
        	current_letter=${first_input:i:1}
        	index=0
        	for j in ${alphabet[@]}; do
                	if [ $j = $current_letter ]; then
                        	break
                	fi
                	((index++))
        	done
        	echo -n ${alphabet[(( (index+current_digit)%26 ))]}
	done
#handles zoo 8 part
elif [ $length_second -eq 1 ];then
	for (( i=0; i<$length_first; i++ )); do
		current_letter=${first_input:i:1}
        	index=0
        	for j in ${alphabet[@]}; do
                	if [ $j = $current_letter ]; then
                        	break
                	fi
                	((index++))
        	done
        	echo -n ${alphabet[(( (index+second_input) % 26 ))]}
	done
fi
echo
