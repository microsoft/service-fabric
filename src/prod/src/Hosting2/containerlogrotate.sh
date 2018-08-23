#!/bin/bash

#------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#------------------------------------------------------------

#############################################################
#
# containerlogrotate.sh
#
# Name
#   containerlogrotate - manages log rotation for container logs
#
# Synopsis
#   containerlogrotate <command> [<args>]
#
# Description
#   containerlogrotate setes up cron job for running logrotate.
#
# Commands
#   Setup
#
#     containerlogrotate cleanup
#
#     Set up a cron job that calls log rotate once every minute.
#     The initial logrotate configuration file is empty but can,
#     be modified through the update command.
#     WARNING: the entire crontab for the user will be overwritten.
#
#   Cleanup
#
#     containerlogrotate cleanup
#
#     Cleans up the files created by the setup operation.
#     The cron job will be removed.
#     WARNING: the entire crontab for the user will be removed.
#
#   Update
#
#     containerlogrotate update <config-file>
#
#     Replaces the contents of the logrotate configuration file
#     with the content of the specified config file.
#
# Return Value
#   Returns 0 if the command completes successfully.
#
# Bugs
#   containerlogrotate replaces the user's crontab on setup and
#   removes the user's crontab on cleanup. This behaviour
#   results in users losing their specified cron jobs.
#   It is assumed that the user running this script does not have
#   any other cron job.
#   At this time, sfuser is not known to contain any cron jobs.
#
#############################################################

## Constants
### Commands
command_setup="setup"
command_cleanup="cleanup"
command_update="update"

### Paths
log_path_root="/mnt/logs/"

crontab_file_name="crontab"
crontab_file_path=${log_path_root}${crontab_file_name}

logrotateconfig_file_name="logrotate.config"
logrotateconfig_file_path=${log_path_root}${logrotateconfig_file_name}

logrotatestate_file_name="logrotate.state"
logrotatestate_file_path=${log_path_root}${logrotatestate_file_name}

## Setup
function crontab_setup() {
  touch ${crontab_file_path}
  echo "SHELL=/bin/bash" >> ${crontab_file_path}
  echo "* * * * * /usr/sbin/logrotate -s ${logrotatestate_file_path} ${logrotateconfig_file_path}" >> ${crontab_file_path}
  crontab ${crontab_file_path}
}

function logrotate_setup() {
  touch ${logrotatestate_file_path}
  touch ${logrotateconfig_file_path}
}

function containerlogrotate_setup() {
  mkdir -p ${log_path_root}
  logrotate_setup
  crontab_setup
}

## Cleanup
function crontab_cleanup() {
  crontab -r
  rm ${crontab_file_path}
}

function logrotate_cleanup() {
  rm ${logrotatestate_file_path}
  rm ${logrotateconfig_file_path}
}

function containerlogrotate_cleanup() {
  crontab_cleanup
  logrotate_cleanup
}

## Update
function containerlogrotate_update() {
  printf "${1}" > ${logrotateconfig_file_path}
}

## Main

user_command=${1}

case ${user_command} in
  ${command_setup})
    containerlogrotate_setup
    exit 0
  ;;
  ${command_cleanup})
    containerlogrotate_cleanup
    exit 0
  ;;
  ${command_update})
    command=${command_update}
    if [ ${#} -lt 2 ]
    then
      errorcode=1
      echo "${0} ${command} : missing argument <config-file> for exited with error code 1"
      exit ${errorcode}
    fi
    containerlogrotate_update "${2}"
    exit 0
  ;;
  *)
    echo "${0} ${user_command} : ${user_command} is not a recognized command."
    exit 1
  ;;
esac
