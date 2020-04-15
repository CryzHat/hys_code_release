#include "c_types.h"

#define MY_DEBUG //uncomment this when release

#ifdef MY_DEBUG

#define DBG(x...)	os_printf(x)
#define DEBUG()     os_printf("%s line=%d\n",__func__,__LINE__);
#else
#define DBG(x...)
#define DEBUG()
#endif

#define SW_OFF 0
#define SW_ON  1


#define TIMER_REGISTER(P_TIMER,FUNC,PARAM,TIME,CYCLE)	do \
														{ \
															os_timer_disarm(P_TIMER); \
															os_timer_setfn(P_TIMER, (os_timer_func_t *)FUNC, PARAM); \
														}while(0)

#define TIMER_START(P_TIMER,TIME,CYCLE)	do \
														{ \
															os_timer_arm(P_TIMER, TIME, CYCLE); \
														}while(0)

#define TIMER_STOP(P_TIMER)	do \
														{ \
															os_timer_disarm(P_TIMER); \
														}while(0)

#define ESP_PARAM_START_SEC  0x7c
#define ESP_PARAM_SAVE_0     1
#define ESP_PARAM_SAVE_1     2
#define ESP_PARAM_SAVE_2     3

#define ESP_PARAM_TEST_START_SEC  0x7F //厂测标志存放位置
#define PRODUCT_TEST_FLAG 		  0x5aa55aa5


#define DISCONNECTED_MODE   0
#define CONNECTED_MODE   	1
#define PROD_MODE_SUCCESS   2
#define PROD_MODE_FAILED    3
#define SOFT_AP_MODE 		4
#define SMARTCONFIG_MODE 	5
#define OTA_MODE     		6
#define PROD_MODE_CAL       7 //校准模式

#define DISCOVERY       0xA0 //局域网发现
#define HEART_BEAT      0xA1
#define DEV_LOGIN       0xA2
#define SW_CTRL         0xA3 //开关控制
#define COUNTER_DOWN    0x04 //倒计时
#define QUERY_STATE     0x05  //查询设备状态 上报插座状态
#define REPORT_STATE    0x05 //上报插座状态
#define DEV_UPGRADE     0x06  //设备ota

#define DEV_RECONNECT      0x98  //设备重连
#define DEV_DEFAULT_SET    0x99  //设备重置

/*
PID 4 bytes
Plug:               0xA0 01 00 01
Bulb(RSH-WB008):    0xA0 05 00 01
Smart IR(RSH-IR02): 0xA0 06 00 01
PIR(RSH-MS02):      0xB0 03 00 01
*/
#define SMART_PIR_1 0xB0
#define SMART_PIR_2 0x03
#define SMART_PIR_3 0x00
#define SMART_PIR_4 0x01


//1.0.0
#define USER_VER_1 1
#define USER_VER_2 0
#define USER_VER_3 0

#define SW_ON   1
#define SW_OFF  0

