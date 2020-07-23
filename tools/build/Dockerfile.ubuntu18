# Increment version number in UBUNTUIMAGE_VERSION file after any changes in this file.

FROM ubuntu:18.04

ADD tools/build/UBUNTU18IMAGE_VERSION .

ADD tools/build/sf-prep-1804.sh /setup/
RUN /setup/sf-prep-1804.sh

RUN apt install locales && locale-gen en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
