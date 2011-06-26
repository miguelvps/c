#!/bin/bash

function c() {
    cd "`command c $@`"
}
if [[ `bind -v` == *"set completion-ignore-case on"* ]]
then
    complete -o filenames -C "command c -ci" c
else
    complete -o filenames -C "command c -c" c
fi
