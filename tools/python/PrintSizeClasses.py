#!/usr/local/bin/python
# -*- coding: utf-8 -*-
"""
@ 文件名：
  PrintSizeClasses.py
@ 说明：
  打印所有的 Size Class，以及能够被 65536 整除的 Size Class
"""

# 计算所有 Size Class
CURRENT_SIZE_CLASS = 0
SIZE_CLASSES = []
while CURRENT_SIZE_CLASS < 65536:
    if CURRENT_SIZE_CLASS < 256:
        CURRENT_SIZE_CLASS += 16
    else:
        CURRENT_SIZE_CLASS *= 2
    SIZE_CLASSES.append(CURRENT_SIZE_CLASS)

print("所有 Size Class：\n%s\n共计%d个" % (SIZE_CLASSES, len(SIZE_CLASSES)))

# 计算能够被 65536 整除的 Size Class
MULTIPLE_SIZE_CLASSES = []
for size_class in SIZE_CLASSES:
    if 65536 % size_class == 0:
        MULTIPLE_SIZE_CLASSES.append(size_class)

print("其中能够被 65536 整除的包括：\n%s\n共计%d个" %
      (MULTIPLE_SIZE_CLASSES, len(MULTIPLE_SIZE_CLASSES)))
