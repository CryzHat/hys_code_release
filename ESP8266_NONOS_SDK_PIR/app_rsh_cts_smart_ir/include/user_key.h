#ifndef __user_key_h__
#define __user_key_h__

#include "c_types.h"
#include "os_type.h"
#include "osapi.h"






#define KEY_recvTaskQueueLen    4

LOCAL os_event_t KEY_recvTaskQueue [KEY_recvTaskQueueLen];
LOCAL os_event_t KEY_procTaskQueue[KEY_recvTaskQueueLen];











#endif



