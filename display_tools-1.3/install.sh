#!/bin/sh

kill -9 `pidof dispd`
kill -9 `pidof menud`

rm -rf /usr/local/bin/disp
rm -rf /usr/local/bin/dispd

cp -v src/dispd /usr/local/bin
cp -v src/disp /usr/local/bin
cp -v src/menud /usr/local/bin
cp -v src/disp-bwm /usr/local/bin

cp -v scripts/menu/menu.conf /etc
cp -v scripts/menu/iface /etc
cp -v scripts/info/info-line-1.sh /usr/local/bin
cp -v scripts/info/info-line-2.sh /usr/local/bin
cp -v scripts/info/disp-showip.sh /usr/local/bin

cp -v scripts/rc/rc.local /etc/rc.d

/etc/rc.d/rc.local
