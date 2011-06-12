#!/bin/bash

function c() {
    local bin=`which c`
    local dir=`$bin $@`
    cd "$dir"
}
complete -A directory c
