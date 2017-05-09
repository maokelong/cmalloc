#!/bin/sh

# 检查参数
if [ $# -ne 1 ]; then
  echo "usage: exec-mdist.sh <preload>"
  echo ""
  echo "build, execute and analsis the mdist benchmark"
  echo ""
  echo "example:"
  echo "  ./exec-mdist.sh path/to/allocator.so 1"
  exit 1
fi

PRELOAD=$1

PATH_PIN="pin-2.14-71313-gcc.4.4.7-linux/"
NAME_PINTOOL="WriteTrace"

# 编译 mdist
echo "building mdist..."
gcc src/mdist.c -o mdist


# 计算保存数据目录的名称
if [ !$PRELOAD ]; 
then
    PATH_DATA="data/ptmalloc2"
else
    PATH_DATA="data/$(basename $PRELOAD .so)"
fi
mkdir -p ${PATH_DATA}
rm ${PATH_DATA}/* -Rf

# 开始测试
echo "executeing mdist..."
export LD_PRELOAD=$PRELOAD
REQUEST_ITER=1000
REQUEST_SIZE=2
while [ $REQUEST_SIZE -lt 258 ]
do
    ${PATH_PIN}pin.sh -t ${PATH_PIN}source/tools/MyPinTool/obj-intel64/${NAME_PINTOOL}.so \
        -- ./mdist $REQUEST_ITER $REQUEST_SIZE \
        > ${PATH_DATA}/${REQUEST_SIZE}.usr.dat
    cp ${NAME_PINTOOL}.out ${PATH_DATA}/${REQUEST_SIZE}.total.dat
    rm ${NAME_PINTOOL}.out -Rf
    REQUEST_SIZE=`expr $REQUEST_SIZE + 2`
done
export LD_PRELOAD=
echo "analysising mdist..."
