#ifndef SIZE_CLASSES_H
#define SIZE_CLASSES_H

#include <stdbool.h>
#include <unistd.h>
/*
 * @ File Name：size_classes.h
 *
 * @ Intro：infos and operators of size class
 *
 */

extern const size_t MAX_TINY_CLASSES;
extern const size_t STEP_TINY_CLASSES;
extern const size_t MAX_MEDIUM_CLASSES;

bool InSmallSize(int size_class);
int SizeToSizeClass(size_t size);
size_t SizeToBLockSize(size_t size);
size_t SizeClassToBlockSize(int size_class);
size_t SizeClassToNumDataBlocks(int size_class);
#endif // SIZE_CLASSES_H