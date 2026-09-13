#ifndef _RTSP_HPP_
#define _RTSP_HPP_

#include <iostream>
#include "rtspserver.hpp"
#include "Fifo_Buffer.hpp"
#include "def.h"

class RTSP{
public:
    RTSP();
    ~RTSP();
    int rtsp_setver_start(void);
    int add_media_stream(void);
    List get_sensor0_fifo(void){return Sensor0_Fifo;}
    List get_sensor1_fifo(void){return Sensor1_Fifo;}
    static int sensor0_rv(void *opaque, uint8_t *data, int len, uint64_t *pts);
    static int sensor1_rv(void *opaque, uint8_t *data, int len, uint64_t *pts);
    int a = 0;

private:
    RTSP_S rtsp_s;
    List Sensor0_Fifo;
    List Sensor1_Fifo;

//live555
public:
    

private:

};

#endif