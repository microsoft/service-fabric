#!/bin/bash

# Set default values and then override with user-set ones.
CFG_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd `pwd` > /dev/null
cd $CFG_DIR
eval `cat ../settings.default.conf`

[ -f ../settings.conf ] && eval `cat ../settings.conf`
popd > /dev/null