#!/usr/bin/env bash

set -x
set -e

yum -y install    \
    dos2unix      \
    gcc-c++       \
    openssl-devel \
    mysql-devel   \
    ncurses-devel \
    libffi-devel  \
    zlib-devel \
    make \
    logrotate

dos2unix configure.sh
./configure.sh
make clean
make all

install --mode=0755 --directory /etc/ua2/
install --mode=0755 --directory /var/lib/ua2/

install --mode=0755 ./bin/ICE /usr/sbin/
install --mode=0755 ./bin/MessageSpec /usr/bin/
install --mode=0755 ./bin/ua /usr/bin/
install --mode=0755 ./bin/libua /usr/libexec/
install --mode=0644 ./bin/ua.edf /etc/ua2/
install --mode=0644 ./dist/uadata.edf /var/lib/ua2/
install --mode=0755 ./dist/init /etc/init.d/ua2-server
install --mode=0644 ./dist/logrotate /etc/logrotate.d/ua2-server

yum clean all
rm -rf /var/cache/yum
