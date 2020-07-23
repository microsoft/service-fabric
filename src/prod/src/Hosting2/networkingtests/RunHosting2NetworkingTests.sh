#!/bin/bash

ScriptDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
export TestRootDir=${ScriptDir}
pushd $ScriptDir
pushd ../
export _NTTREE=$(pwd)
pushd $ScriptDir
${ScriptDir}/Hosting2.Networking.Test.exe "$@"
popd

