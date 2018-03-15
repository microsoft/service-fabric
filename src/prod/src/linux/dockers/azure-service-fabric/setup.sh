#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

apt update
apt install -y openssh-server curl libc++1 cifs-utils
apt install -y libssh2-1 libunwind8 libib-util
apt install -y lttng-tools lttng-modules-dkms liblttng-ust0 chrpath members sshpass nodejs nodejs-legacy npm locales
apt install -y cgroup-bin acl net-tools apt-transport-https rssh vim atop libcurl3 openjdk-8-jre
sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list'
apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893
apt-get update
apt-get install -y dotnet-runtime-2.0.0
/etc/init.d/ssh start
locale-gen en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US:en
export LC_ALL=en_US.UTF-8