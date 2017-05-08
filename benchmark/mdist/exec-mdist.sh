#!/bin/sh

#TODO export LD_PRELOAD=

PATH_PIN="pin-2.14-71313-gcc.4.4.7-linux/"
NAME_PINTOOL="WriteTrace"

REQUEST_ITER="1000"

mkdir data
gcc src/mdist.c -o mdist

REQUEST_SIZE=2
while [ $REQUEST_SIZE -lt 256 ]
do
    #{PATH_PIN}pin.sh -t \
    rm data/${REQUEST_SIZE}.dat.2 -Rf
    rm data/${REQUEST_SIZE}.dat.2 -Rf
    ${PATH_PIN}pin.sh -t ${PATH_PIN}source/tools/MyPinTool/obj-intel64/${NAME_PINTOOL}.so \
        -- ./mdist $REQUEST_ITER $REQUEST_SIZE > data/${REQUEST_SIZE}.dat.1
    cp ${NAME_PINTOOL}.out data/${REQUEST_SIZE}.dat.2
    rm ${NAME_PINTOOL}.out -Rf
    REQUEST_SIZE=`expr $REQUEST_SIZE + 2`
done

#TODO:使用 python 脚本原始分析数据
