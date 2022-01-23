#!/bin/sh
# Install pre-requisite packages

apt-get update
apt-get install protobuf-c-compiler libncurses-dev libjansson-dev libglib2.0-dev libprotobuf-c-dev libcurl4-openssl-dev dh-sysuser devscripts librtlsdr-dev
ldconfig
