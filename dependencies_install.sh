#!/bin/bash

# Install CRIU dependencies
sudo apt-get update
sudo apt-get install libprotobuf-dev libprotobuf-c-dev protobuf-c-compiler protobuf-compiler python3-protobuf
sudo apt-get install --no-install-recommends pkg-config python-ipaddress libbsd-dev iproute2 libnftables-dev libcap-dev libnl-3-dev libnet-dev libaio-dev libgnutls28-dev python3-future
sudo apt-get install asciidoc xmltosudo dmidecode -s system-manufacturer

# Install podman and runc
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_20.04/ /" | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
wget -qO - https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_20.04/Release.key | sudo apt-key add -
sudo apt-get update
sudo apt-get -y install podman buildah runc

# Install udpcast
sudo apt-get install -y udpcast