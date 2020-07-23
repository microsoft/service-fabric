#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

check_errs()
{
  # Function. Parameter 1 is the return code
  if [ "${1}" -ne "0" ]; then
    echo Operation ${2} failed with exit code ${1} 
    exit ${1}
  fi
}

export LD_LIBRARY_PATH=/opt/microsoft/servicefabric/bin/Fabric/Fabric.Code
export FabricDataRoot=$(cat /etc/servicefabric/FabricDataRoot)
export FabricLogRoot=$(cat /etc/servicefabric/FabricLogRoot)
export FabricBinRoot=$(cat /etc/servicefabric/FabricBinRoot)
export FabricCodePath=$(cat /etc/servicefabric/FabricCodePath)

# increase limits
ulimit -Sc unlimited
ulimit -Hc unlimited
ulimit -n 99999

sed -i '/^fs\.inotify\.max_user_instances/d' /etc/sysctl.conf
echo fs.inotify.max_user_instances=2048 | sudo tee -a /etc/sysctl.conf && sudo sysctl -p

# rssh setting
if [ ! -f /etc/rssh.conf ]; then
   echo "Error: rssh not installed."
   exit -1
fi
if ! grep -q "^allowscp" /etc/rssh.conf ; then
   echo "allowscp" >> /etc/rssh.conf
fi
if ! grep -q "^allowsftp" /etc/rssh.conf ; then
   echo "allowsftp" >> /etc/rssh.conf
fi

#echo "/var/crash/core.%e.%p.%t" > /proc/sys/kernel/core_pattern

# Start tracing
# TODO: Move to TraceSessionManager after figuring out startring lttng as sfuser or root 
# Ex: lttng_handler.sh $serviceFabricVersion $traceDiskBufferSizeInMB $keepExistingSFSessions
echo "Starting tracing session"
traceDiskBufferSizeInMB=5120
keepExistingSFSessions=0
if command -v lttng >/dev/null 2>&1; then
    ./Fabric/DCA.Code/lttng_handler.sh $(cat ./Fabric/DCA.Code/ClusterVersion) $traceDiskBufferSizeInMB $keepExistingSFSessions
else
    echo "LTTng is not installed." >&2;
fi

echo Configuring block devices for system services
for i in `lsblk -l | cut -d' ' -f1 |grep -v NAME`
do 
    setfacl -m "u:sfuser:rw-" /dev/${i}
done

# Setting sfuser as owner of /home/sfuser for writing selfsigned certs
echo "Setting sfuser as owner of /home/sfuser"
if [ -d "/home/sfuser" ]; then
   chown sfuser:sfuser /home/sfuser
else
   mkdir /home/sfuser
   chown sfuser:sfuser /home/sfuser
fi

/opt/microsoft/servicefabric/bin/Fabric/Fabric.Code/FabricDeployer.sh
check_errs $? FabricDeployer

#Load Blockstore driver.
${FabricCodePath}/BlockStore/driver/blockstore.sh -u -l -s ${FabricCodePath}/BlockStore/driver/
if [ $? != 0 ]; then
    echo "Couldn't load blockstore driver."
fi

echo Starting FabricHost
/opt/microsoft/servicefabric/bin/FabricHost -c -skipfabricsetup
