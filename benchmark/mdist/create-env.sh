#!/bin/sh

PATH_PIN="pin-2.14-71313-gcc.4.4.7-linux/"
NAME_PINTOOL="WriteTrace"

wget http://software.intel.com/sites/landingpage/pintool/downloads/pin-2.14-71313-gcc.4.4.7-linux.tar.gz
tar -xzf pin-2.14-71313-gcc.4.4.7-linux.tar.gz
rm -Rf pin-2.14-71313-gcc.4.4.7-linux.tar.gz

cp ../../tools/pin/${NAME_PINTOOL}.cpp ${PATH_PIN}source/tools/MyPinTool/
cd ${PATH_PIN}source/tools/MyPinTool/
mkdir obj-intel64
make obj-intel64/${NAME_PINTOOL}.so
