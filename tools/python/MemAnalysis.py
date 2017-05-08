"""
@ 文件名：
  MemAnalysis.py
@ 说明：
  读取MemTtack分析结果，将内存切片并统计每片内存的写入次数
  统计结果保存在命名为源文件名加.stat后缀的文件中
"""
import re
import math

# 定义内存切片粒度
MEM_GRAIN = 256
# 定义保存结果的字典
STAT_DICT = {}

# 打开源文件
NAME_SOURCE_FILE = input("请输入保存MemTrack分析结果的文件名（含路径）：")
SOURCE_FILE = open(NAME_SOURCE_FILE, 'r')

# 切片、统计
PATTERN = re.compile('Access: Write to (0x[0-9a-f]+)')
for each_line in SOURCE_FILE:
    regex_match = PATTERN.match(each_line)
    if regex_match:
        key = int(regex_match.group(1), 16) // MEM_GRAIN
        if key in STAT_DICT:
            STAT_DICT[key] += 1
        else:
            STAT_DICT[key] = 1
sorted(STAT_DICT.keys())

# 关闭源文件
SOURCE_FILE.close()

# 打开目标文件
NAME_TARGET_FILE = NAME_SOURCE_FILE + '.stat'
TARGET_FILE = open(NAME_TARGET_FILE, 'w')
for key, value in STAT_DICT.items():
    res_line = '%d\t%d\n' % (key, math.log2(value))
    TARGET_FILE.writelines(res_line)

# 关闭目标文件
TARGET_FILE.close()
