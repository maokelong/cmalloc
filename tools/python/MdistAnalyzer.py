#!/usr/local/bin/python
# -*- coding: utf-8 -*-
import os

# 设置保存结果的文件
FILE_RESULT = "result"

# 计算测试数据所在目录
BASE_DIR = os.path.dirname(__file__)
RELA_PATH_BENCHDATA = (".." + os.sep) * 2 + "benchmark" + \
    os.sep + "mdist" + os.sep + "data"
PATH_BENCHDATA = os.path.join(BASE_DIR, RELA_PATH_BENCHDATA)

# 递归遍历测试数据目录
for dir_path, dirs_name, files_name in os.walk(PATH_BENCHDATA):
    # ！ 跳过无数据文件的目录
    if len(files_name) == 0:
        continue

    target_file = open(os.path.join(dir_path, FILE_RESULT + ".tmp"), "w+")

    # 迭代当前目录下所有文件
    for file_name in files_name:
        # ! 跳过非法文件
        if not file_name.endswith(".dat"):
            continue
        source_name = os.path.join(dir_path, file_name)
        source_file = open(source_name, "r")

        # 迭代当前文件中所有行
        total_writes = 0
        for line in source_file.readlines():
            cur_addr = int(line, 16)
            # 若当前行写存的地址位于数据段或栈则跳过
            if cur_addr >= 0x7f0000000000 or cur_addr <= 0x610a60:
                continue
            # 否则计数
            total_writes += 1

        # 当前文件迭代结束后输出统计信息
        splited_file_name = file_name.split('.')
        request_size = int(splited_file_name[0])
        target_file.write("%-8s%d\t%d\n" %
                          (splited_file_name[1], request_size, total_writes))
    # 待所有文件统计完毕后，对统计信息进行进一步提炼
    target_file.seek(0)
    total_dict = {}
    usr_dict = {}
    for line in target_file.readlines():
        # 加载所有数据
        spilted_line = line.split()
        if spilted_line[0] == "total":
            total_dict[spilted_line[1]] = spilted_line[2]
        else:
            usr_dict[spilted_line[1]] = spilted_line[2]
    target_file.close()
    ratio_dict = {}
    for key, value in total_dict.items():
        # 计算用户写/总写比例
        ratio_dict[int(key)] = float(usr_dict[key]) / float(total_dict[key])
    target_file = open(os.path.join(dir_path, FILE_RESULT + ".txt"), "w+")
    for each in sorted(ratio_dict.items(), key=lambda k: k[0]):
        # 输出统计结果
        target_file.write("%s\t%f\n" % (each[0], each[1]))
    # visiting files
# visiting dirs
