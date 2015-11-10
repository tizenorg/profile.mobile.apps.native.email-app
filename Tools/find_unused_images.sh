#!/bin/bash

# This script should be executed on /email/tools

function usage()
{
	echo
	echo " $0 --with-rm=[yes/no]"
	echo
	exit
}

if [ $# = 1 -a "${1%%=*}" = "--with-rm" ]
then
	__with_rm=${1##*=}
	if ! [ "${__with_rm}" = "yes" -o "${__with_rm}" = "no" ]
	then
		usage
	fi
else
	usage
fi

__root_dir=".."
__dest_dir="${__root_dir}/images"
__git_dir=".git"

find ${__dest_dir} -type f -exec basename {} \; | while read file
do
	_count=`grep "${file}" ${__root_dir} -rn --exclude-dir=${__git_dir} | wc -l`
	if [ ${_count} = 0 ];
	then
		_path=`find ${__dest_dir} -name "${file}"`
		echo "UNUSED FILE >> ${_path}"
		if [ "$__with_rm" = "yes" ]
		then
			rm ${_path}
		fi
	fi
done

