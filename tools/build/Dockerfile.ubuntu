# Increment version number in UBUNTUIMAGE_VERSION file after any changes in this file.

FROM ubuntu:16.04

ADD tools/build/UBUNTUIMAGE_VERSION .
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    bison \
    btrfs-tools \
    build-essential \
    chrpath \
    cmake \
    curl \
    flex \
    git \
    go-md2man \
    libassuan-dev \
    libc6-dev \
    libcgroup-dev \
    libcurl3 \
    libcurl4-openssl-dev \
    libdevmapper-dev \
    libedit-dev \
    libglib2.0-0 \
    libglib2.0-dev \
    libgpg-error-dev \
    libgpgme11-dev \
    libib-util \
    liblttng-ust-dev \
    liblttng-ctl-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libpopt-dev \
    libseccomp-dev \
    libselinux1-dev \
    libssh2-1-dev \
    libssl-dev \
    libtool \
    libtool \
    libxml2-dev \
    locales \
    lttng-modules-dkms \
    lttng-tools \
    ninja-build \
    openssh-server \
    pkg-config \
    runc \
    software-properties-common \
    sudo \
    swig \
    unzip \
    uuid-dev \
    wget \
    acl

ADD tools/build/sf-prep.sh /setup/

# .NET Core <3.0 adds ~1.0GB of bloat to the container in the NugetFallbackFolder (removed in >=3.0)
# by caching numerous packages your build may not need 
# These settings help mitigate this bloat.
ENV NUGET_XMLDOC_MODE=skip
ENV DOTNET_SKIP_FIRST_TIME_EXPERIENCE=1
RUN /setup/sf-prep.sh && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
