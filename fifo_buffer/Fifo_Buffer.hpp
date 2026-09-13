#ifndef _FIFO_BUFFER_HPP_
#define _FIFO_BUFFER_HPP_


#include <iostream>
#include <stdint.h>
#include <list>
#include <vector>

#include "Pthread_Lock.hpp"

class CHN_FIFO{
public:
    CHN_FIFO(){
        len = 0;
        pts = 0;
        data = NULL;
    }
    ~CHN_FIFO(){};
    int len = 0;
    uint64_t pts = 0;
    int8_t *data = nullptr;
};
class ENC_FIFO{
public:
    ENC_FIFO(){
        len = 0;
        pts = 0;
        key = false;
        data = NULL;
    }
    ~ENC_FIFO(){};
    int len = 0;
    uint64_t pts = 0;
    bool key = false;
    int8_t *data = nullptr;
};

class STREAM_QUEUE{
public:
    STREAM_QUEUE(){
        len = 0;
        pts = 0;
        key = false;
        data = NULL;
    }
    ~STREAM_QUEUE(){};
    int len = 0;
    uint64_t pts = 0;
    bool key = false;
    int8_t *data = nullptr;
    unsigned char read_mirror;//读索引是不是在镜像缓冲区标志
    unsigned int read_index;//读索引
    unsigned char write_mirror;//写索引是不是在镜像缓冲区标志
    unsigned int write_index;//写索引
    unsigned int buffer_size;//缓冲区容量
};

class List{
public:
    List(){
        pthVar.Init_Mutex_Cond(mutex, r_cond, w_cond);
        pthVar.Init_Mutex_Cond(enc_mutex, r_enc_cond, w_enc_cond);
        pthVar.Init_Mutex_Cond(queue_mutex, r_queue_cond, w_queue_cond);
        // _q_list.resize(32);
    }
    ~List(){};
    int write(int8_t *buf, int len, uint64_t pts, int ms);
    int read(int8_t **buf, int *len, uint64_t *pts, int ms);
    int write_enc(int8_t *buf, int len, uint64_t pts, bool key, int ms);
    int read_enc(int8_t **buf, int *len, uint64_t *pts, bool *key, int ms);
    int clean_enc_fifo(void);
    int write_queue(int8_t *buf, int len, uint64_t pts, bool key, int ms);
    int read_queue(int8_t **buf, int *len, uint64_t *pts, bool *key, int ms);
    int realse_queue(int num);
    std::list<CHN_FIFO> Get_List(void){return _i_list;}
    int Clear_Fifo(void);
private:
    std::list<CHN_FIFO> _i_list;
    std::list<ENC_FIFO> _e_list;
    std::vector<STREAM_QUEUE> _q_list;
    PTH_LOCK pthVar;
    pthread_cond_t r_cond;
    pthread_cond_t w_cond;
    pthread_mutex_t mutex;

    pthread_cond_t r_enc_cond;
    pthread_cond_t w_enc_cond;
    pthread_mutex_t enc_mutex;
    
    pthread_cond_t r_queue_cond;
    pthread_cond_t w_queue_cond;
    pthread_mutex_t queue_mutex;
};


#endif