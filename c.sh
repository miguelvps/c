#!/bin/bash

function c() {
    local bin=`which c`
    local dir=`$bin "$@"`
    cd "$dir"
}
# if readline completion-ignore-case is set to `on'
complete -o filenames -C "`which c` -ci" c
# else
# complete -o filenames -C "`which c` -c" c
