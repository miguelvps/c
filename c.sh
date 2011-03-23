#!/bin/bash

function c() {
    local bin=`which c`
    local dir=`$bin $@`
    echo "$dir"
    cd "$dir"
}
complete -A directory c
