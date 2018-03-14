#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


CREATE_TABLE_FILE=false

function log {
  logger "SF-DCA $1"
}

function running_in_docker {
  count=$(awk '/\/docker\//' /proc/self/cgroup | wc -l)
  result=0
  if [ "$count" -gt "0" ]
  then
    result=1
  fi
  return $result
}

#create a new session with the given session name
function createSession {
  log "Creating session $1"
  fabricLogRoot=$(cat /etc/servicefabric/FabricLogRoot)
  logPath="${fabricLogRoot}/lttng-traces/${1}-$(date '+%Y%m%d-%H%M%S')"
  lttng create $1 --output=${logPath}
  running_in_docker
  if [ $? -eq "0" ]
  then
    lttng enable-channel --session $1 --userspace --tracefile-size 8388608 --subbuf-size 8388608 channel0
  else
    log "WARN: Inside Docker container, using smaller subbuf size to workaround a bug"
    lttng enable-channel --session $1 --userspace --tracefile-size 8388608 --subbuf-size 8192 channel0
  fi
  lttng enable-event --channel channel0 --userspace "service_fabric:*"
  lttng add-context -u -t vtid
  lttng add-context -u -t vpid
  lttng start
}

#total number of sessions at the moment
function sessionCount {
  sessionName=$1
  count="$(lttng list | grep -o "[0-9]*) ${sessionName}_[0-9] ([A-Za-z0-9\.\/ _-]*) \[[a-z]*\]" | wc -l)"
  echo $count
}


function getSessionName {
  session=${1%% (*}
  session=${session##*) }
  echo $session
}

function getFolderPath {
  folder=${1#* (}
  folder=${folder%%) *}
  echo $folder
}

function listSessions {
  sessionName=$1
  list="$(lttng list | grep -o "[0-9]*) ${sessionName}_[0-9] ([A-Za-z0-9\.\/ _-]*) \[[a-z]*\]")"
  echo "$list"
}

function createlogrotateConfigFile {
  echo "${1} {
  rotate 3
}" > ${2}
}

#convert the traces by session name and folder path
# $1: session name
# $2: trace dir
# $3: output common folder
#converted file will have .trace extension and will be changed to .dtr once conversion is done
function convertTraces {
  log "Stopping session $1"
  lttng stop $1
  lttng destroy $1
  SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  export LD_LIBRARY_PATH="${SCRIPT_DIR}"
  TRACE_DIR=$2
  DATE_NOW="$(date -u +%Y-%m-%d_%H:%M:%S)"
  SCRIPT_PATH="${SCRIPT_DIR}/sftrace"

  OFILE="${3}/${DATE_NOW}"
  OUTPUT_FILENAME="${OFILE}.trace"
  OUTPUT_FINISHED_FILENAME="${OFILE}.dtr"
  OUTPUT_TABLE_FILENAME="${OFILE}.table1"
  OUTPUT_TABLE_FINISHED_FILENAME="${OFILE}.table"
  OUTPUT_DTR_TO_APPEND_FOLDERNAME="${3}/../UserTraces/"
  OUTPUT_DTR_TO_APPEND_FILENAME="${OUTPUT_DTR_TO_APPEND_FOLDERNAME}traces.dtr"
  LOGROTATE_CONFIG_FILENAME="/tmp/sfusertraceslogrotate.conf"

  sftrace_arguments="--no-delta -f loglevel --clock-date --clock-gmt "${TRACE_DIR}" -w $OUTPUT_FILENAME"
  if [ "$CREATE_TABLE_FILE" == "true" ]
  then
    sftrace_arguments="${sftrace_arguments} -c TableEvents.config -t $OUTPUT_TABLE_FILENAME"
  fi

  log "Writing to $OUTPUT_FILENAME"

  ${SCRIPT_PATH} ${sftrace_arguments}

  log "Head $(head -1 $OUTPUT_FILENAME)"
  log "Tail $(tail -1 $OUTPUT_FILENAME)"

  # Creating dtr file for customer user traces
  if [ ! -d "$OUTPUT_DTR_TO_APPEND_FOLDERNAME" ]; then
    log "Creating customer user traces folder at: ${OUTPUT_DTR_TO_APPEND_FOLDERNAME}"
    mkdir $OUTPUT_DTR_TO_APPEND_FOLDERNAME
  fi

  if [ ! -d "$LOGROTATE_CONFIG_FILENAME" ]; then
    log "Creating logrotate configuration file at: $LOGROTATE_CONFIG_FILENAME"
    createlogrotateConfigFile $OUTPUT_DTR_TO_APPEND_FILENAME $LOGROTATE_CONFIG_FILENAME
  fi
  # rotating customer user traces .dtr file
  logrotate $LOGROTATE_CONFIG_FILENAME

  # writing new .dtr file for customer user traces
  cp $OUTPUT_FILENAME $OUTPUT_DTR_TO_APPEND_FILENAME

  log "Moving $OUTPUT_FILENAME to $OUTPUT_FINISHED_FILENAME"
  mv $OUTPUT_FILENAME $OUTPUT_FINISHED_FILENAME

  if [ "$CREATE_TABLE_FILE" == "true" ]
  then
    log "Moving $OUTPUT_TABLE_FILENAME to $OUTPUT_TABLE_FINISHED_FILENAME"
    mv $OUTPUT_TABLE_FILENAME $OUTPUT_TABLE_FINISHED_FILENAME
  fi
}

# Validate a session folder name
# $1: session folder name
# $2: session name
function validateSessionFolder {
  flag=`echo $1 | awk -v sessionName="$2" '{print match($1, sessionName)}'`;
  if [ $flag -gt 0 ]
  then
    echo 0
  else
    echo 1
  fi
}

log "LTTng start $(date)"
if [ $# -eq 0 ]
then
  count=$(sessionCount "WindowsFabric")
  if [ $count -eq 0 ]
  then
    count=$(sessionCount "ServiceFabric")
    if [ $count -eq 0 ]
    then
      createSession ServiceFabric_0
    fi
  fi

elif [ $# -gt 0 ]
then
  if [ "$2" == "-table" ]
  then
    CREATE_TABLE_FILE=true
  fi

  if [ "$1" == "-c" ]
  then
    sessionPrefix="ServiceFabric"
    count=$(sessionCount $sessionPrefix)

    if [ $count -eq 2 ]
    then
      sessions=$(listSessions $sessionPrefix)
      sessionName=$(getSessionName "$sessions")
      sessionFolder=$(getFolderPath "$sessions")

      convertTraces $sessionName $sessionFolder $1
      if [ 0 -eq $(validateSessionFolder $sessionFolder $sessionName) ]
      then
        rm -R $sessionFolder
      fi
    fi
    if [ $count -eq 1 ]
    then
      sessions=$(listSessions $sessionPrefix)
      sessionName=$(getSessionName "$sessions")
      sessionFolder=$(getFolderPath "$sessions")

      convertTraces $sessionName $sessionFolder "./ClusterData/Data/log/Traces/"
      parentDirectory=${sessionFolder%/*}
      rm -R $parentDirectory/$sessionName*
    fi

  else
    sessionPrefix="WindowsFabric"
    count=$(sessionCount $sessionPrefix)
    if [ $count -eq 0 ]
    then
      sessionPrefix="ServiceFabric"
      count=$(sessionCount $sessionPrefix)
    fi

    if [ $count -ge 2 ]
    then
      sessions=$(listSessions $sessionPrefix)
      sessionName=$(getSessionName "$sessions")
      sessionFolder=$(getFolderPath "$sessions")
      lttng stop $sessionName
      lttng destroy $sessionName
      if [ 0 -eq $(validateSessionFolder $sessionFolder $sessionName) ]
      then
        rm -R $sessionFolder
      fi

    elif [ $count -eq 1 ]
    then
      newSession="ServiceFabric_0"
      sessions=$(listSessions $sessionPrefix)
      sessionName=$(getSessionName "$sessions")
      sessionFolder=$(getFolderPath "$sessions")

      if [ "$sessionName" = "ServiceFabric_0" ]
      then
        newSession="ServiceFabric_1"
      fi

      createSession $newSession

      sleep 1s

      convertTraces $sessionName $sessionFolder $1

      if [ 0 -eq $(validateSessionFolder $sessionFolder $sessionName) ]
      then
        parentDirectory=${sessionFolder%/*}
        rm -R $parentDirectory/$sessionName*
      fi

    elif [ $count -eq 0 ]
    then
      createSession ServiceFabric_0
    fi
  fi
fi
log "LTTng end $(date)"