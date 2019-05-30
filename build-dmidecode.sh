#!/bin/sh
wget http://git.savannah.gnu.org/cgit/dmidecode.git/snapshot/dmidecode-3-2.tar.gz
tar xfz dmidecode-3-2.tar.gz
cd dmidecode-3-2/
make
cd ..
gcc -Wall extract.c -o extract -g -O
