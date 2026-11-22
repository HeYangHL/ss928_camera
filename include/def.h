#ifndef _DEF_H_
#define _DEF_H_

#include <stdio.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <thread>

#include "Fifo_Buffer.hpp"

#define ss_print(fmt...)                             \
    do                                               \
    {                                                \
        printf("[%s]-%d: ", __FUNCTION__, __LINE__); \
        printf(fmt);                                 \
    } while (0)

#define ss_pause()                                                          \
    do                                                                      \
    {                                                                       \
        printf("---------------press enter key to exit!---------------\n"); \
        getchar();                                                          \
    } while (0)


#endif
