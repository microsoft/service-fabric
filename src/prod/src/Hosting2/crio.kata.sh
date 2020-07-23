#!/bin/bash

#------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#------------------------------------------------------------

mydir="$( cd "$( dirname "$0" )" && pwd )"
toexit=0

logsub=" crio.sh: "

echo "${logsub}crio.sh starting" | logger
echo "${logsub}FabricBinRoot:  $FabricBinRoot"  | logger
echo "${logsub}FabricDataRoot: $FabricDataRoot" | logger
echo "${logsub}FabricCrioSock: $FabricCrioSock" | logger
echo "${logsub}FabricCrioConfig: $FabricCrioConfig" | logger

. /etc/os-release
LinuxDist=$ID

# special handling for onebox
if [ "$(basename $FabricDataRoot)" == "Data" ]; then
    FabricDataRoot=$(realpath "${FabricDataRoot}/../..")
fi

FabricCrioRoot=$(realpath "${FabricDataRoot}/crio-root")

echo $$ > /sys/fs/cgroup/systemd/tasks

cmd="$mydir/crio"

function normalize_version {
    echo "$@" | gawk -F. '{ printf("%02d%02d%02d\n", $1,$2,$3); }';
}

function setup_deppkgs() {
    pkg_retries=5
    while [[ "$pkg_retries" -ne 0 ]]; do
        inst_result=0
        if [ $LinuxDist = "rhel" ] || [ $LinuxDist = "centos" ]; then
            KataDist="RHEL"
            if [ LinuxDist = "centos" ]; then
                KataDist="CentOS"
            fi
            if ! yum list installed "kata-runtime" >/dev/null 2>&1 ; then
                yum-config-manager --add-repo "http://download.opensuse.org/repositories/home:/katacontainers:/release/${KataDist}_7/home:katacontainers:release.repo"
                yum -y install lvm2 socat kata-runtime kata-proxy kata-shim skopeo 2>&1 | logger
                inst_result=${PIPESTATUS[0]}
            fi
            yum -y update lvm2 socat kata-runtime kata-proxy kata-shim skopeo 2>&1 | logger
        if [ $inst_result -eq 0 ]; then
            return 0
        fi
        else
            add_repo=false
            if dpkg -l cc-runtime > /dev/null 2>&1 ; then
                apt-get purge -y cc-runtime\* cc-proxy\* cc-shim\* linux-container clear-containers-image qemu-lite cc-ksm-throttler
                rm /etc/apt/sources.list.d/clear-containers.list
            fi
            if ! dpkg -s kata-runtime > /dev/null 2>&1 ; then
                BRANCH=master
                ARCH=$(arch)
                sh -c "echo 'deb http://download.opensuse.org/repositories/home:/katacontainers:/releases:/${ARCH}:/${BRANCH}/xUbuntu_$(lsb_release -rs)/ /' > /etc/apt/sources.list.d/kata-containers.list"
                curl -sL  http://download.opensuse.org/repositories/home:/katacontainers:/releases:/${ARCH}:/${BRANCH}/xUbuntu_$(lsb_release -rs)/Release.key | sudo apt-key add -
                add_repo=true
            fi
            if ! dpkg -s skopeo-containers > /dev/null 2>&1 ; then
                add-apt-repository -y ppa:projectatomic/ppa > /dev/null 2>&1
                add_repo=true
            fi
            apt-get update > /dev/null 2>&1
            apt-get install -y lvm2 thin-provisioning-tools socat libgpgme11 kata-runtime kata-proxy kata-shim skopeo-containers   2>&1 | logger
            inst_result=${PIPESTATUS[0]}
            sed -i 's/default_vcpus = 1/default_vcpus = -1/' /usr/share/defaults/kata-containers/configuration.toml
            if [ $inst_result -eq 0 ] || [ "$add_repo" = false ]; then
                return 0
            fi
            
        fi

        pkg_retries=$((pkg_retries-1))
        sleep 5
    done
    return 1
}

function setup_cni() {
    if [ ! -d "/opt/cni/bin" ]; then
        mkdir -p /opt/cni/bin
    fi
    cp $mydir/loopback /opt/cni/bin
    cp $mydir/host-local /opt/cni/bin
    cp $mydir/bridge /opt/cni/bin
    cp $mydir/sf-cni /opt/cni/bin
    cp $mydir/azure-vnet /opt/cni/bin
    cp $mydir/azure-vnet-ipam /opt/cni/bin

    if [ ! -d "/etc/cni/net.d" ]; then
        mkdir -p /etc/cni/net.d
    fi
    cp $mydir/00-sf-cni.conf /etc/cni/net.d/
    cp $mydir/99-loopback.conf /etc/cni/net.d/
    cp $mydir/10-mynet.conf /etc/cni/net.d/

    if ! iptables -C FORWARD -i cni0 -j ACCEPT > /dev/null 2>&1 ; then
        iptables -A FORWARD -i cni0 -j ACCEPT
    fi

    if ! iptables -C FORWARD -o cni0 -j ACCEPT > /dev/null 2>&1 ; then
        iptables -A FORWARD -o cni0 -j ACCEPT
    fi

    if ! iptables -C FORWARD -i azure2 -j ACCEPT > /dev/null 2>&1 ; then
        iptables -A FORWARD -i azure2 -j ACCEPT
    fi

    if ! iptables -C FORWARD -o azure2 -j ACCEPT > /dev/null 2>&1 ; then
        iptables -A FORWARD -o azure2 -j ACCEPT
    fi
}

function cri_cleanup() {
    if pgrep -x "crio" > /dev/null
    then
        echo "${logsub}stopping and removing pods..." | logger
        pods=( $(${mydir}/crictl pods | cut -d' ' -f1))
        for pod in "${pods[@]:1}"
        do
            echo "${logsub}stopping pod: $pod" | logger
            nohup sh -c "${mydir}/crictl stopp $pod ; ${mydir}/crictl rmp $pod"  &
            disown
        done
    fi
}

function cri_wait() {
    cri_wait_timeout=300
    while pgrep -x "crictl" > /dev/null && [[ "$cri_wait_timeout" -ne 0 ]]; do
        sleep 1
        cri_wait_timeout=$((cri_wait_timeout-1))
    done
    if [[ "$cri_wait_timeout" -ne 0 ]]; then
        return 0
    fi
    return 1
}

function cc_cleanup() {
    containerlst=($(/usr/bin/kata-runtime list | tail -n +2 | cut -d\  -f1))
    for container in "${containerlst[@]}"
    do
        echo "${logsub}stopping kata container: $container" | logger
        nohup sh -c "/usr/bin/kata-runtime kill $container; /usr/bin/kata-runtime delete $container" > /dev/null &
        disown
    done
}

function cc_wait() {
    cc_wait_timeout=300
    while pgrep -x "kata-runtime" > /dev/null && [[ "$cc_wait_timeout" -ne 0 ]]; do
        sleep 1
        cc_wait_timeout=$((cc_wait_timeout-1))
    done
    if [[ "$cc_wait_timeout" -ne 0 ]]; then
        return 0
    fi
    return 1
}

function clean_containerlogrotate() {
    echo "${logsub}cleaning up container log rotation policy" | logger
    ${mydir}/containerlogrotate.sh cleanup
}

function clean_crio_and_cc() {
    cri_cleanup
    cri_wait
    if [ -n "$crio_pid" ]; then
        echo "${logsub}stopping crio daemon $crio_pid"  | logger
        kill -9 $crio_pid >/dev/null 2>&1
    fi
    cc_cleanup
    cc_wait
}

function clean_crioroot() {
    echo "${logsub}cleaning crio data root... ${FabricCrioRoot}" | logger
    umount $(grep ${FabricCrioRoot} /proc/mounts | cut -f2 -d' ' | sort -r)  2>&1 | logger
    rm ${FabricCrioRoot}/* -r 2>&1 | logger
}

function start_containerlogrotate() {
    echo "${logsub}setting up container log rotation policy" | logger
    ${mydir}/containerlogrotate.sh setup

    # TODO move this call into Hosting and have it manage and create
    # the content of the logrotate configuration file.
    ${mydir}/containerlogrotate.sh update "/mnt/logs/*_clearcontainer/*/*/*/*/*/application.log {\n  copytruncate\n  size 10M\n  rotate 4\n  missingok\n}\n"
}

function start_crio() {
    echo "${logsub}starting new crio daemon..."  | logger
    echo "runtime-endpoint: ${FabricCrioSock}" > /etc/crictl.yaml
    rm -f ${FabricCrioSock}
    mkdir -p "${FabricCrioRoot}"
    nohup $cmd --conmon "${mydir}/conmon" \
      --config "${mydir}/${FabricCrioConfig}" \
      --seccomp-profile "${mydir}/seccomp.json" \
      --listen "${FabricCrioSock}" \
      --storage-driver "devicemapper" \
      --root "${FabricCrioRoot}/lib/containers" \
      --runroot "${FabricCrioRoot}/run/containers" \
      --log "${FabricCrioRoot}/crio.log" \
      --log-level info &
    crio_pid=$!
    disown
    echo "${logsub}crio daemon started: $crio_pid" | logger
}

trap 'exitcode=$?; echo ${logsub}crio.sh exitcode is $exitcode | logger' EXIT HUP INT QUIT PIPE TERM

trap signal_handler 10

function signal_handler() {
    toexit=1
}

echo "${logsub}crio.sh is starting" | logger

echo "${logsub}checking /dev/kvm" | logger
if [ ! -e "/dev/kvm" ]; then
    echo "${logsub}nested virt is not supported on this host. exit." | logger
    sleep 30
    exit 99
fi

# install dependency packages
echo "${logsub}install the dependency packages" | logger
if ! setup_deppkgs ; then
    echo "${logsub}failed to install dependency packages... crio.sh has to exit" | logger
    exit 1
fi

# setup cni
echo "${logsub}setup the cni plugins" | logger
setup_cni

# clean up leftovers
echo "${logsub}clean up running pre-existing runtimes" | logger
crio_pid=$(pgrep -x "crio")
clean_crio_and_cc

# enable KSM (TBD)
# echo 1 > /sys/kernel/mm/ksm/run

# set up container log rotation policy
start_containerlogrotate

# start crio
retries=5
while [[ "$retries" -ne 0 ]]; do
    start_crio

    timeout=30
    while [ ! -S ${FabricCrioSock} ] && [[ "$timeout" -ne 0 ]]; do
        sleep 1
        timeout=$((timeout-1))
    done

    if [ ! -S ${FabricCrioSock} ]; then
        echo "${logsub}crio daemon failed to start. initiate full cleanup..." | logger
        clean_crio_and_cc
        clean_crioroot
    else
        podid=$($mydir/crictl runp <(echo '{"metadata":{"name":"fst_pod","namespace":"ns_fst_pod"} }'))
        if [ $? -ne 0 ]; then
            echo "${logsub}crio daemon started but not working. initiate full cleanup..." | logger
            clean_crio_and_cc
            clean_crioroot
        else
            echo "${logsub}crio daemon started successfully" | logger
            $mydir/crictl stopp $podid
            $mydir/crictl rmp $podid
            break
        fi
    fi
    retries=$((retries-1))
done

if [ ! -S ${FabricCrioSock} ]; then
    echo "${logsub}crio daemon failed to started after retries. crio.sh has to exit" | logger
    clean_containerlogrotate
    exit 2
fi

touch /var/run/crio/crio.ready

while [ $toexit -eq 0 ]; do
    kill -0 $crio_pid >/dev/null 2>&1 || break
    sleep 1
done

if ! kill -0 $crio_pid > /dev/null 2>&1; then
    echo "${logsub}crio exited. initiate full cleanup..." | logger
    clean_crio_and_cc
    clean_crioroot
    rm -f ${FabricCrioSock}
    rm -f /var/run/crio/crio.ready
else
    echo "${logsub}FabricHost exited. cleaning up runtimes..." | logger
    clean_crio_and_cc
fi

clean_containerlogrotate

sleep 5
echo "${logsub}crio.sh exited" | logger

