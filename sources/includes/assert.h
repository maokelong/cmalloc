#ifndef CMALLOC_ASSERT_H
#define CMALLOC_ASSERT_H

#ifndef CMALLOC_DEGUB
#define DEBUG
#endif // ! DEGUB

void error_and_exit(char *msg, ...);
#endif // ! ASSERT_H