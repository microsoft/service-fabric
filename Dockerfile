FROM ubuntu:16.04

ADD DOCKER_VERSION .

# Add legacy binary dependencies
ADD https://sfossdeps.blob.core.windows.net/binaries/v0.1.tgz /tmp
RUN mkdir -p /external && tar -xvf /tmp/v0.1.tgz -C / && \
     rm /tmp/v0.1.tgz

RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    openssh-server \
    build-essential \
    libtool \
    cmake \
    libunwind-dev \
    uuid-dev \
    libxml2-dev \
    libssl-dev \
    libssh2-1-dev \
    lttng-tools \
    lttng-modules-dkms \
    liblttng-ust-dev \
    locales \
    libcgroup-dev \
    libib-util \
    libcurl3 \
    git \
    libtool \
    autoconf \
    automake \
    libglib2.0-0 \
    libglib2.0-dev \
    bison \
    flex \
    libpopt-dev \
    libncurses5-dev \
    libncursesw5-dev \
    swig \
    libedit-dev \
    chrpath

# Install the .NET runtime dependency.  Required for running the product.
RUN sh -c 'echo "deb [arch=amd64] https://apt-mo.trafficmanager.net/repos/dotnet-release/ xenial main" > /etc/apt/sources.list.d/dotnetdev.list' && \
    apt-key adv --keyserver apt-mo.trafficmanager.net --recv-keys 417A0893 && \
    apt-get install -y apt-transport-https apt-utils && \
    apt-get update && \
    apt-get install -y dotnet-runtime-2.0.0 && \
    apt-get remove -y apt-transport-https apt-utils

RUN locale-gen en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
