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

path_pin = $(path_tools)/pin# pin 工具所在目录
path_python = $(path_tools)/python# python 工具所在目录

path_pin_home = $(path_pin)/pin-2.14-71313-gcc.4.4.7-linux# pin 文件夹
path_pin_custools = $(path_pin_home)/source/tools/MyPinTool# 定制工具源文件目标文件夹

relapath_pin_cuslib = obj-intel64#定制工具动态库目标文件夹（相对路径）

# 指定文件名称
cmalloc_srcs = $(wildcard $(path_cmalloc_srcs)/*.c)# cmalloc 源代码
cmalloc_OBJs = $(notdir $(cmalloc_srcs:.c=.o))# cmalloc 中间代码

testcase_srcs = $(wildcard *.c)# 各测试用例的源文件
testcase_out = $(testcase_srcs:.c=.out)# 各测试用例编译后生成的可执行文件

pin_tar = $(path_pin)/pin-2.14-71313-gcc.4.4.7-linux.tar.gz# pin 解压包名
pin_sh = $(path_pin_home)/pin.sh

pintool_src = MemTrack.cpp# pintool 源文件名称（不含路径）
pintool_src_ori = $(addprefix $(path_pin)/,$(pintool_src))# pintool 源文件名称（原位置）
pintool_src_tar = $(addprefix $(path_pin_custools)/,$(pintool_src))# pintool 源文件名称（应拷贝位置）

pintool_lib = $(pintool_src:.cpp=.so)# pintool 生成的动态库的名称（不含路径）
pintool_lib_rel = $(addprefix $(relapath_pin_cuslib)/,$(pintool_lib))# pintool 生成的动态库的名称（相对于$(path_pin_custools)的路径）
pintool_lib_abs = $(addprefix $(path_pin_custools)/$(relapath_pin_cuslib)/,$(pintool_lib))# pintool 生成的动态库的名称（相对当前目录的路径）

pintool_print = $(pintool_src:.cpp=.out)# 保存 pintool 执行结果的文件名（不含路径）
pintool_print_oir = $(addprefix $(path_pin_custools)/,$(pintool_print))# 保存 pintool 执行结果的文件名（原路径）
pintool_print_cpy = $(testcase_srcs:.c=.pinout)# 保存 pintool 执行结果的文件名（修改后的名称）

# 指定 pin 下载链接
pin_wget_src = http://software.intel.com/sites/landingpage/pintool/downloads/pin-2.14-71313-gcc.4.4.7-linux.tar.gz

# 指定编译选项
CFLAGS += -O0 -Wstrict-prototypes -g3 -Wall -pthread -fomit-frame-pointer
CC = gcc

# ==========================================================
# define rules
# ==========================================================

# 解释伪规则
.PHONY: all pintest clean

# 编译连接所有测试用例
all:$(cmalloc_OBJs) $(testcase_out)

$(cmalloc_OBJs):
	$(CC) -c $(CFLAGS) $(cmalloc_srcs) -I$(path_cmalloc_headers)

$(testcase_out):%.out:%.c
	$(CC) $(CFLAGS) $^ $(cmalloc_OBJs) -o $@

test:
	@echo $(pintool_lib_rel)

# 使用 pintools 测试分析所有测试用例（暂时仅支持一个PinTool，未来可能会添加多pintool支持）
pintest:$(pin_tar) $(pin_sh) $(pintool_lib) $(pintool_print_cpy)
	 
$(pin_tar):
	cd $(path_pin); wget $(pin_wget_src)

$(pin_sh):$(pin_tar)
	tar -xzf $(pin_tar) -C $(path_pin)

$(pintool_lib):$(pin_sh)
	cp $(pintool_src_ori) $(pintool_src_tar)
	cd $(path_pin_custools);mkdir -p $(relapath_pin_cuslib); make $(pintool_lib_rel)

$(pintool_print_cpy)::$(pintool_lib)
$(pintool_print_cpy)::%.pinout:%.out
	$(pin_sh) -t $(pintool_lib_abs) -- ./$^ > $^.print
	cp $(pintool_print) $@

clean:
	-rm *.o *.out *.print *.pinout $(path_pin_home) -Rf