#!/usr/local/bin/python
# -*- coding: utf-8 -*-
import os

# 打开保存结果的文件
NAME_RESULT = os.path.join(os.path.dirname(__file__), "result.c")
FILE_RESULT = open(NAME_RESULT, "w")

# 生成比例表（sizeof(super datablock) / (num * sizeof(shadowblock))
RATIO_DATA_SHADOW = []
for num in range(2, 32 + 1, 2):
    RATIO_DATA_SHADOW.append(num)
for power in range(6, 13 + 1, 1):
    RATIO_DATA_SHADOW.append(2**power)

# 生成加速表
ACC_ARRAY_A = ""
ACC_ARRAY_B = ""

for i in range(0, 24):
    if i < 16:
        # 生成加速表A
        ACC_ARRAY_A += "\n{"
        for offset_data in range(0, SIZE_SDB, 16):
            ACC_ARRAY_A += str(int(offset_data / (8 * (i + 1))))
            if offset_data + 16 < SIZE_SDB:
                ACC_ARRAY_A += ","
        ACC_ARRAY_A += "}"
        if i != 15:
            ACC_ARRAY_A += ",\n"

     # 生成加速表B
    if i < 16:
        ACC_ARRAY_B += "0,"
    else:
        ACC_ARRAY_B += str(i - 16 + 6)
        if i != 23:
            ACC_ARRAY_B += ","


FILE_RESULT.write("// Array that accelerate the conversion process from\n")
FILE_RESULT.write("// datablock's offset to shadowblock's offset\n")
FILE_RESULT.write(
    "int HELPER_DATA_SHADOW_HIGH[] = {" + ACC_ARRAY_B + "}; \n")
FILE_RESULT.write(
    "uint16_t HELPER_DATA_SHADOW_LOW[16][SIZE_SDB >> 4] = {" + ACC_ARRAY_A + "}; \n")
