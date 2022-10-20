FROM centos:7

ADD tools/build/CENTOSIMAGE_VERSION .

RUN rpm --import http://repos.azul.com/azul-repo.key && \
         curl http://repos.azul.com/azure-only/zulu-azure.repo -o /etc/yum.repos.d/zulu-azure.repo && \
         yum -q -y update && \
         yum -q -y install zulu-8-azure-jdk

RUN yum update -y && yum install -y \
    epel-release \
    && yum groupinstall 'Development Tools' -y \
    && yum install -y \
    sudo \
    openssl \
    openssh-server \
    lsof \
    rsyslog \
    ca-certificates \
    net-tools \
    patch \
    git \
    mercurial \
    wget \
    curl \
    tar \
    bzip2 \
    zip \
    unzip \
    devtoolset-7 \
    cmake3 \
    rssh \
    openssl-devel \
    libcurl-devel \
    libxml2-devel \
    libssh2-devel \
    libuuid-devel \
    libunwind-devel \
    libicu-devel \
    libcgroup-devel \
    chrpath \
    && yum clean all

RUN ln -s /usr/bin/cmake3 /usr/bin/cmake

RUN rpm -Uvh https://packages.microsoft.com/config/rhel/7/packages-microsoft-prod.rpm && yum install -y dotnet-sdk-2.0.3

RUN wget -P /etc/yum.repos.d/ https://packages.efficios.com/repo.files/EfficiOS-RHEL7-x86-64.repo
RUN rpmkeys --import https://packages.efficios.com/rhel/repo.key

RUN yum install -y lttng-tools lttng-ust lttng-ust-devel lttng-tools-devel

ADD https://linuxbuilds.blob.core.windows.net/dockers/clang50_gor_1120-centos.tgz /tmp
RUN tar -xvf /tmp/clang50_gor_1120-centos.tgz -C /usr/lib && \
    rm /tmp/clang50_gor_1120-centos.tgz

RUN chown -R root:root /usr/lib/llvm-5.0 && \
    ln -fs /usr/lib/llvm-5.0/bin/clang /usr/bin/clang && \
    ln -fs /usr/lib/llvm-5.0/bin/clang++ /usr/bin/clang++

ADD https://linuxbuilds.blob.core.windows.net/dockers/libcpp-3.5-2-headers.tgz /tmp
RUN tar -xvf /tmp/libcpp-3.5-2-headers.tgz -C /usr/include/c++ && \
    rm /tmp/libcpp-3.5-2-headers.tgz

RUN localedef -v --quiet -c -i en_US -f UTF-8 en_US.UTF-8; exit 0

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
