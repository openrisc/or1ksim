#!/bin/bash

# Copyright (C) 2010 ORSoC AB
# Copyright (C) 2010 Embecosm Limited

# Contributor Julius Baxter <julius.baxter@orsoc.se>
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

# This file is a superuser script to close down an Ethernet bridge and restore
# the simple Ethernet interface.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.          

# ------------------------------------------------------------------------------

# Pre-requisites: bridge-utils must be installed.

# Usage: ./brend.sh <bridge> <eth> <tap>

# - <bridge> is the bridge interface to use, e.g. br0
# - <eth> is the hardware ethernet interface to use, e.g. eth0
# - <tap> is the tap interface to use, e.g. tap0

# Check we have the right number of arguments
if [ "x$#" != "x3" ]
then
    echo "Usage: ./brend.sh <bridge> <eth> <tap>"
    exit 1
fi

# Check we are root
euid=`id -un`
if [ "x${euid}" != "xroot" ]
then
    echo "Must run as root"
    exit 1
fi

# Break out the arguments
br=$1
eth=$2
tap=$3

# Determine the IP address, netmask and broadcast of the bridge.
eth_ip=`ifconfig $br | \
        grep "inet addr" | \
        head -1 | \
        sed -e 's/^.*inet addr:\([^ \t]*\).*$/\1/'`
eth_netmask=`ifconfig $br | \
        grep "Mask" | \
        head -1 | \
        sed -e 's/^.*Mask:\([^ \t]*\).*$/\1/'`
eth_broadcast=`ifconfig $br | \
        grep "Bcast" | \
        head -1 | \
        sed -e 's/^.*Bcast:\([^ \t]*\).*$/\1/'`

# Close the firewall to the tap and bridge
iptables -D INPUT -i ${tap} -j ACCEPT
iptables -D INPUT -i ${br} -j ACCEPT
iptables -D FORWARD -i ${br} -j ACCEPT

# Take down the bridge and delete it
ifconfig ${br} down

if [ $? != 0 ]
then
    echo "Failed to take down ${br}"
    exit 1
fi

brctl delbr ${br}

if [ $? != 0 ]
then
    echo "Failed to take delete ${br}"
    exit 1
fi

# Delete the TAP interface. Note we mustn't have anything using it. It's
# rather harsh, but we use fuser to ensure this (it will take out all users of
# any TAP/TUN interface).
fuser -k /dev/net/tun
openvpn --rmtun --dev ${tap}

if [ $? != 0 ]
then
    echo "Failed to remove ${tap}"
    exit 1
fi

# Restore the Ethernet interface. We could use ifconfig with the IP address,
# netmask and broadcast mask from earlier, but this does not work in a DHCP
# world (the MAC has changed). Instead we use a single shot dhcp
# configuration. In future the extant eth0 dhclient will refresh the lease.
dhclient -1 -d ${eth0}

if [ $? != 0 ]
then
    echo "Failed to get lease for ${eth}"
    exit 1
fi

# Kill the outstanding br0 DHCL client
kill `ps ax | grep "dhclient.*${br}" | grep -v "grep" | cut -c 1-5`
