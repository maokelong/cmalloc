#!/usr/local/bin/python
# -*- coding: utf-8 -*-
import os

# 打开保存结果的文件
NAME_RESULT = os.path.join(os.path.dirname(__file__), "result.c")
FILE_RESULT = open(NAME_RESULT, "w")

NUM_SIZE_CLASS = 24
NUM_MEDIUM_SIZE_CLASS = 16
SIZE_SDB = 65536
SIZE_SHADOW = 2

# 生成模块 1
BLOCK_SIZE = []
NUM_BLOCKS = []
RATIO_DATA_SHADOW = []

for i in range(0, NUM_SIZE_CLASS):
    if i < NUM_MEDIUM_SIZE_CLASS:
        BLOCK_SIZE.append((i + 1) * 16)
    else:
        BLOCK_SIZE.append(BLOCK_SIZE[15] * 2**(i + 1 - NUM_MEDIUM_SIZE_CLASS))

    NUM_BLOCKS.append(SIZE_SDB / BLOCK_SIZE[i])

    RATIO_DATA_SHADOW.append(BLOCK_SIZE[i] / SIZE_SHADOW)

# 打印模块 1

FILE_RESULT.write("int BLOCK_SIZE[NUM_SIZE_CLASSES] = {")
for each in BLOCK_SIZE:
    FILE_RESULT.write(str(each) + ",")
FILE_RESULT.write("};\n\n")

FILE_RESULT.write("int NUM_BLOCKS[NUM_SIZE_CLASSES] = {")
for each in NUM_BLOCKS:
    FILE_RESULT.write(str(int(each)) + ",")
FILE_RESULT.write("};\n\n")

FILE_RESULT.write("int RATIO_DATA_SHADOW[NUM_SIZE_CLASSES] = {")
for each in RATIO_DATA_SHADOW:
    FILE_RESULT.write(str(int(each)) + ",")
FILE_RESULT.write("};\n\n")
