import os

BASE_DIR = os.path.dirname(__file__)
REL_PATH_CMALLOC = (".." + os.sep) * 2 + "sources"
PATH_CMALLOC = os.path.join(BASE_DIR, REL_PATH_CMALLOC)

TOTAL_LINES = 0
for dir_path, dirs_name, files_name in os.walk(PATH_CMALLOC):
    for file_name in files_name:
        source_name = os.path.join(dir_path, file_name)
        source_file = open(source_name, "r", encoding='UTF-8')
        source_lines = len(source_file.readlines())
        print("%s:%d" % (file_name, source_lines))
        TOTAL_LINES += source_lines
print(TOTAL_LINES)
