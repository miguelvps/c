#!/bin/bash

function c() {
    cd "`command c $_C_OPTS "$@"`"
}
if [[ `bind -v` == *"set completion-ignore-case on"* ]]
then
    complete -o filenames -C 'command c $_C_OPTS -ci' c
else
    complete -o filenames -C 'command c $_C_OPTS -c' c
fi
