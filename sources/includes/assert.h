#ifndef ASSERT_H
#define ASSERT_H

#ifndef DEGUB
#define DEBUG
#endif // ! DEGUB

void error_and_exit(char *msg, ...);
#endif // ! ASSERT_H