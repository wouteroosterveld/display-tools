#!/bin/bash
IFACE=`cat /etc/iface`

/usr/local/bin/disp --info1 "`echo IP:``ifconfig $IFACE | grep "inet addr" | awk '{ print $2 }' |sed -e {s/addr://}`"
