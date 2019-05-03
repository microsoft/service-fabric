#!/usr/bin/env bash
ulimit -c unlimited
check_errs()
{
  # Function. Parameter 1 is the return code
  if [ "${1}" -ne "0" ]; then
    # make our script exit with the right error code.
    exit ${1}
  fi
}

DIR=`dirname $0`
echo 0x3f > /proc/self/coredump_filter
dotnet $DIR/MS.Test.ImageStoreService.dll $@ >> out.out
check_errs $?
