#!/bin/bash

function c() {
    cd "`command c $@`"
}
# if readline completion-ignore-case is set to `on'
complete -o filenames -C "command c -ci" c
# else
# complete -o filenames -C "command c -c" c
