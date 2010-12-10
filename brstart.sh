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

# Usage: ./brset.sh <bridge> <eth> <tap> [<tap> <tap> ...]

# - <bridge> is the bridge interface to use, e.g. br0
# - <eth> is the hardware ethernet interface to use, e.g. eth0
# - <tap> is/are the persistent TAP interface(s)

# The tap interfaces must have been previously set up persistently by the
# superuser using for example:

# openvpn --mktun --dev tap<n> --user <username> --group <groupname>

# Define Bridge Interface
br=$1
shift

# Host ethernet interface to use
eth=$1
shift

# Determine the IP address, netmask and broadcast of the host.
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

# Define list of TAP interfaces to be bridged,
tap=$*

echo "Creating bridge $br"
echo "  Host Ethernet device: $eth"
echo "  Host IP address:      $eth_ip"
echo "  Host netmask:         $eth_netmask"
echo "  Host broadcast:       $eth_broadcast"
echo "  Target TAP device(s): $tap"

# Create the bridge
brctl addbr $br

# Add the host Ethernet and TAP interfaces
brctl addif $br $eth

for t in $tap; do
    brctl addif $br $t
done

# Remove the IP addresses of the underlying interfaces
ifconfig $eth 0.0.0.0 promisc up

for t in $tap; do
    ifconfig $t 0.0.0.0 promisc up
done

# Reconfigure the bridge to have the Ethernet address that had been used just
# by $eth.
# ifconfig $br $eth_ip netmask $eth_netmask broadcast $eth_broadcast
dhclient $br
