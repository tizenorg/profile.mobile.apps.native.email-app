#!/bin/bash

function MakeColorXml() {
	ic=$2
	row=$3
	while read var; do
		#echo $var
		codename=`echo $var | cut -d',' -f5`
		inputcolor=`echo $var | cut -d',' -f$ic`
		h=`echo $var | cut -d',' -f$row | sed s,[^0-9+-],,g`
		s=`echo $var | cut -d',' -f$(expr $row + 1) | sed s,[^0-9+-],,g`
		b=`echo $var | cut -d',' -f$(expr $row + 2) | sed s,[^0-9+-],,g`
		a=`echo $var | cut -d',' -f$(expr $row + 3) | sed s,[^0-9+-],,g`

		if [[ $codename =~ ^[0-9A-Za-z]+$  && $inputcolor =~ .{1} && $h =~ .{1} && $s =~ .{1} && $b =~ .{1} && $a =~ .{1} ]];
		then
			fh=`echo $var | cut -d',' -f$(expr $row + 4)`
			fs=`echo $var | cut -d',' -f$(expr $row + 5)`
			fv=`echo $var | cut -d',' -f$(expr $row + 6)`
			minh=`echo $var | cut -d',' -f$(expr $row + 7)| sed s,[^0-9+-],,g`
			mins=`echo $var | cut -d',' -f$(expr $row + 8)| sed s,[^0-9+-],,g`
			minb=`echo $var | cut -d',' -f$(expr $row + 9)| sed s,[^0-9+-],,g`
			maxh=`echo $var | cut -d',' -f$(expr $row + 10) | sed s,[^0-9+-],,g`
			maxs=`echo $var | cut -d',' -f$(expr $row + 11) | sed s,[^0-9+-],,g`
			maxb=`echo $var | cut -d',' -f$(expr $row + 12) | sed s,[^0-9+-],,g`
			printf "\t\t\t<ChangeableColorInfo id=\"$codename\" inputColor=\"$inputcolor\" hue=\"$h\" saturation=\"$s\" value=\"$b\" alpha=\"$a\" " >> $1_color.xml

			if [[ $fh =~ ^[A-Z]+$ ]];
			then
				printf "fixedHue=\"true\" " >> $1_color.xml
			fi

			if [[ $fs =~ ^[A-Z]+$ ]];
			then
				printf "fixedSaturation=\"true\" " >> $1_color.xml
			fi

			if [[ $fv =~ ^[A-Z]+$ ]];
			then
				printf "fixedValue=\"true\" " >> $1_color.xml
			fi

			if [[ $minh =~ ^[0-9]+$ ]];
			then
				printf "minHue=\"$minh\" " >> $1_color.xml
			fi
			if [[ $mins =~ ^[0-9]+$ ]];
			then
				printf "minSaturation=\"$mins\" " >> $1_color.xml
			fi
			if [[ $minb =~ ^[0-9]+$ ]];
			then
				printf "minValue=\"$minb\" " >> $1_color.xml
			fi
				if [[ $maxh =~ ^[0-9]+$ ]];
			then
				printf "maxHue=\"$maxh\" " >> $1_color.xml
			fi
			if [[ $maxs =~ ^[0-9]+$ ]];
			then
				printf "maxSaturation=\"$maxs\" " >> $1_color.xml
			fi
			if [[ $maxb =~ ^[0-9]+$ ]];
			then
				printf "maxValue=\"$maxb\" " >> $1_color.xml
			fi
			printf "/>\n" >> $1_color.xml
		fi
		printf "."
	done < $1
}

function MakeTextXml() {
	row=$2
	while read var; do
		#echo $var
		codename=`echo $var | cut -d',' -f5`
		size=`echo $var | cut -d',' -f35 | sed s,[^0-9+-],,g`

		if [[ $codename =~ ^AT[0-9A-Za-z]+$  && $size ]];
		then
			typeface=`echo $var | cut -d',' -f$row`
			fstyle=`echo $var | cut -d',' -f$(expr $row + 1)`
			giant=`echo $var | cut -d',' -f$(expr $row + 4)`
			#typeface=`echo $var | cut -d',' -f$(expr $row + 3)`

			printf "\t<FontInfo id=\"$codename\" " >> $1_font.xml

			if [[ $typeface =~ ^[0-9A-Za-z]+$ ]];
			then
				printf "typeface=\"$typeface\" " >> $1_font.xml
			fi

			printf "style=\"$fstyle\" size=\"$size\" "  >> $1_font.xml

			if [[ $giant =~ ^[A-Z]+$ ]];
			then
				printf "giant=\"true\" " >> $1_font.xml
			fi
			printf "/>\n" >> $1_font.xml
		fi
		printf '.'
	done < $1
}

function ColorTable() {
	ic=$2
	row=$3
	row2=$4
	num=$5
	printf "<Color Table$5> Dark theme color parsing start\n "
	printf "\t<ChangeableColorTable num=\"$5\">\n\t\t<Theme style=\"Dark\">\n" >> $1_color.xml
	# Dark Theme
	MakeColorXml $1 $2 $3 $5
	printf '\t\t</Theme>\n'  >> $1_color.xml
	printf "\nDone\n\n"

	printf "<Color Table$5> Light theme color parsing start\n"
	printf '\t\t<Theme style="Light">\n' >> $1_color.xml
	# Light Theme
	MakeColorXml $1 $2 $4 $5
	printf '\t\t</Theme>\n\t</ChangeableColorTable>\n'  >> $1_color.xml
	printf "\nDone\n\n"
}

printf "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" > $1_color.xml
printf "<ChangeableColorTables>\n" >> $1_color.xml
# Color Table1
ColorTable $1 6 7 20 1

# Color Table2
ColorTable $1 42 43 56 2

# Color Table3
ColorTable $1 73 74 87 3
printf "</ChangeableColorTables>\n" >> $1_color.xml

# Font Table
printf 'Font data parsing\n'
printf '<?xml version="1.0" encoding="UTF-8"?>\n<FontInfoTable>\n' > $1_font.xml

MakeTextXml $1 33
printf '</FontInfoTable>\n'  >> $1_font.xml
printf "\nDone\n\n"
