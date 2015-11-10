#!/bin/bash

# run on email/

__id_list=(`find . -name "en_US.po" | xargs egrep -o "\"IDS_[^\"]+\"" | cut -f2 -d ":" | sort | uniq`)
__all_file_list=(`find . -name "*.c" -o -name "*.h" -o -name "*.cpp"`)

for __id in ${__id_list[@]}; do
    __result=""
    for __file in ${__all_file_list[@]}; do
        __result=`egrep -rn $__id $__file`
        if [ -n "$__result" ]; then
            break
        fi
    done
    if [ ! -n "$__result" ]; then
        echo "UNUSED STRING >> $__id"
    fi
done
