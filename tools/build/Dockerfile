# Increment version number in DOCKERIMAGE_VERSION file after any changes in this file.

FROM ubuntu:16.04

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
    wget

# Install packages from PPA(s)
RUN add-apt-repository ppa:gophers/archive && add-apt-repository ppa:alexlarsson/flatpak && apt-get update && apt install -y --no-install-recommends libostree-dev golang-1.9

# Put the PPA Go 1.9 onto the path
RUN ln -s /usr/lib/go-1.9/bin/go /usr/local/bin/go

# Install .NET sdk for building the managed code of product.
RUN apt-get install -y apt-transport-https ca-certificates && \
    sh -c 'curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg' && \
    mv microsoft.gpg /etc/apt/trusted.gpg.d/microsoft.gpg && \
    sh -c 'echo "deb [arch=amd64] https://packages.microsoft.com/repos/microsoft-ubuntu-xenial-prod xenial main" > /etc/apt/sources.list.d/dotnetdev.list' && \
    apt-get update && \
    apt-get install -y dotnet-sdk-2.0.3

# Install the .NET runtime dependency.  Required for running the product.
RUN sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list' && \
    apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893 && \
    apt-get install -y apt-transport-https apt-utils && \
    apt-get update && \
    apt-get install -y dotnet-runtime-2.0.7
    # apt-get remove -y apt-transport-https apt-utils

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add - && \
    sh -c 'echo "deb [arch=amd64] http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" > /etc/apt/sources.list.d/clang6.list' && \
    apt-get update && \
    apt-get install -y clang-6.0

RUN locale-gen en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# Version file added last so that it doesnt reset all previous layers
ADD tools/build/DOCKERIMAGE_VERSION .
