#!/bin/bash

function c() {
    local bin=`which c`
    local dir=`$bin "$@"`
    cd "$dir"
}
complete -o filenames -C "`which c` -c" c
