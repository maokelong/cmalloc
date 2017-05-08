# ==========================================================
# basic check，
# ie. checking the architecture of target machine
# ==========================================================

# 检查目标机器架构
ARCH = $(shell uname -m)
SUPPORTED_ARCH = NO

ifeq ($(ARCH), x86_64)
SUPPORTED_ARCH = YES
endif

ifeq ($(SUPPORTED_ARCH), NO)
$(error Your architecture $(ARCH) is not currently supported. See README.)
endif

# ==========================================================
# assign value to variables,
# including paths, filenames, compile cfgs.
# ==========================================================

# 指定文件路径
vpath %.h sources/includes
vpath %.c sources/

path_cmalloc_headers = ../sources/includes# cmalloc 头文件
path_cmalloc_srcs = ../sources# cmalloc 源文件

path_tools = tools# 工具文件夹
path_res = results# 结果输出目录

# 指定文件名称
cmalloc_srcs = $(wildcard $(path_cmalloc_srcs)/*.c)# cmalloc 源代码
cmalloc_OBJs = $(notdir $(cmalloc_srcs:.c=.o))# cmalloc 中间代码

testcase_srcs = $(wildcard *.c)# 各测试用例的源文件
testcase_out = $(testcase_srcs:.c=.out)# 各测试用例编译后生成的可执行文件

# 指定编译选项
CFLAGS += -O0 -Wstrict-prototypes -g3 -Wall -pthread -fomit-frame-pointer
CC = gcc

# ==========================================================
# define rules
# ==========================================================

# 解释伪规则
.PHONY: all clean

# 编译连接所有测试用例
all:$(cmalloc_OBJs) $(testcase_out)

$(cmalloc_OBJs):
	$(CC) -c $(CFLAGS) $(cmalloc_srcs) -I$(path_cmalloc_headers)

$(testcase_out):%.out:%.c
	$(CC) $(CFLAGS) $^ $(cmalloc_OBJs) -o $@

clean:
	-rm *.o *.out -Rf