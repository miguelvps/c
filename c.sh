#!/bin/bash

function c() {
    local bin=`which c`
    local dir=`$bin $@`
    if [ "$1" != "$dir" ] ; then
        echo "$dir"
    fi
    cd "$dir"
}
complete -A directory c
