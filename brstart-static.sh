#!/bin/bash

# Copyright (C) 2010 ORSoC AB
# Copyright (C) 2010 Embecosm Limited

# Contributor Julius Baxter <julius.baxter@orsoc.se>
# Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

# This file is a superuser script to set up an Ethernet bridge that can be
# used with Or1ksim via the TUN/TAP interface.

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

# Usage: ./brstart-static.sh <username> <groupname> <bridge> <eth> <tap>

# - <bridge> is the bridge interface to use, e.g. br0
# - <eth> is the hardware ethernet interface to use, e.g. eth0
# - <tap> is/are the persistent TAP interface(s)

# Check we have the right number of arguments
if [ "x$#" != "x5" ]
then
    echo "Usage: ./brstart-static.sh <username> <groupname> <bridge> <eth> <tap>"
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
username=$1
groupname=$2
br=$3
eth=$4
tap=$5

# Determine the IP address, netmask and broadcast of the current Ethernet
# interface. This is used if the bridge is set up manually, rather than using
# DHCP.
eth_ip=`ifconfig $eth | \
        grep "inet addr" | \
        head -1 | \
        sed -e 's/^.*inet addr:\([^ \t]*\).*$/\1/'`
eth_netmask=`ifconfig $eth | \
        grep "Mask" | \
        head -1 | \
        sed -e 's/^.*Mask:\([^ \t]*\).*$/\1/'`
eth_broadcast=`ifconfig $eth | \
        grep "Bcast" | \
        head -1 | \
        sed -e 's/^.*Bcast:\([^ \t]*\).*$/\1/'`

# Create the TAP interface
openvpn --mktun --dev ${tap} --user ${username} --group ${groupname}

if [ $? != 0 ]
then
    echo "Failed to create ${tap}"
    exit 1
fi

# Create the bridge
brctl addbr ${br}

if [ $? != 0 ]
then
    echo "Failed to create ${br}"
    exit 1
fi

# Add the host Ethernet and TAP interfaces, removing the IP addresses of the
# underlying interfaces.
for i in ${eth} ${tap}
do
    # Add the interface
    brctl addif ${br} ${i}

    if [ $? != 0 ]
    then
	echo "Failed to create ${i}"
	exit 1
    fi

    # Remove the IP address
    ifconfig ${i} 0.0.0.0 promisc up

    if [ $? != 0 ]
    then
	echo "Failed to remove IP interface of ${i}"
	exit 1
    fi
done

# Reconfigure the bridge to have the original IP address, netmask and
# broadcast mask
ifconfig ${br} ${eth_ip} netmask ${eth_netmask} broadcast ${eth_broadcast}

# Open up firewall to the tap and bridge. We have a generic reject at the end
# of the chain, so we insert these at the start.
iptables -I INPUT 1 -i ${tap} -j ACCEPT
iptables -I INPUT 1 -i ${br} -j ACCEPT
iptables -I FORWARD 1 -i ${br} -j ACCEPT
