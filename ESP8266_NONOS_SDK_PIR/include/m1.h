#if 1
#include "c_types.h"
#include "gpio.h"

typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef signed int INT;
typedef int *PINT;
typedef void *PVOID;
typedef signed char CHAR;
typedef unsigned char UCHAR;
typedef char *PCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef short *PSHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef long *PLONG;
typedef unsigned int UINT;

#define ELE_PARAM_SEC    0x7C

#define REVERSE(v)	(v == 0 ? 1 : 0)

typedef struct
{
    u32     save_flag;
    
    u32     U32_P_REF_PLUSEWIDTH_TIME;          //参考功率 脉冲周期
    u32     U32_V_REF_PLUSEWIDTH_TIME;          //参考电压 脉冲周期
    u32     U32_I_REF_PLUSEWIDTH_TIME;          //参考电流 脉冲周期
    u32     U16_REF_001_E_Pluse_CNT;            //0.1度电脉冲总数参考值
    
    u32     E_VAL;//上电开始的总用电量  通过指令可以清除  产测模式下清0
} ELE_CNT;

struct esp_platform_saved_param 
{
    uint8 value[40];
    uint8 token[40];
    uint8 activeflag;
    uint8 pad[3];
};

#if 0//minitool

#define WIFI_SW_IO_NUM        14
#define WIFI_SW_IO_NAME       PERIPHS_IO_MUX_MTMS_U
#define WIFI_SW_IO_FUNC       FUNC_GPIO14

#define WIFI_STATE_LED_IO_NUM      4
#define WIFI_STATE_LED_IO_NAME     PERIPHS_IO_MUX_GPIO4_U
#define WIFI_STATE_LED_IO_FUNC     FUNC_GPIO4

#define KEY_IO_NUM      13
#define KEY_IO_MUX 		PERIPHS_IO_MUX_MTCK_U
#define KEY_IO_FUNC  	FUNC_GPIO13

#define WIFI_RLED_IO_NUM        0
#define WIFI_RLED_IO_NAME       PERIPHS_IO_MUX_GPIO0_U
#define WIFI_RLED_IO_FUNC       FUNC_GPIO0

#else //RSH HYS-01-085  RSH

#if 0//BSD

#define WIFI_SW_IO_NUM        12
#define WIFI_SW_IO_NAME       PERIPHS_IO_MUX_MTDI_U
#define WIFI_SW_IO_FUNC       FUNC_GPIO12

#define WIFI_RLED_IO_NUM        0
#define WIFI_RLED_IO_NAME       PERIPHS_IO_MUX_GPIO0_U
#define WIFI_RLED_IO_FUNC       FUNC_GPIO0

#define WIFI_STATE_LED_IO_NUM      15
#define WIFI_STATE_LED_IO_NAME     PERIPHS_IO_MUX_MTDO_U
#define WIFI_STATE_LED_IO_FUNC     FUNC_GPIO15

#define KEY_IO_NUM      13
#define KEY_IO_MUX 		PERIPHS_IO_MUX_MTCK_U
#define KEY_IO_FUNC  	FUNC_GPIO13

#else

#define WIFI_SW_IO_NUM        15
#define WIFI_SW_IO_NAME       PERIPHS_IO_MUX_MTDO_U
#define WIFI_SW_IO_FUNC       FUNC_GPIO15

#define WIFI_RLED_IO_NUM        0
#define WIFI_RLED_IO_NAME       PERIPHS_IO_MUX_GPIO0_U
#define WIFI_RLED_IO_FUNC       FUNC_GPIO0

#define WIFI_STATE_LED_IO_NUM      2
#define WIFI_STATE_LED_IO_NAME     PERIPHS_IO_MUX_GPIO2_U
#define WIFI_STATE_LED_IO_FUNC     FUNC_GPIO2

#define KEY_IO_NUM      13
#define KEY_IO_MUX 		PERIPHS_IO_MUX_MTCK_U
#define KEY_IO_FUNC  	FUNC_GPIO13

#endif

#endif

#define Get_KEY_PORT   GPIO_ID_PIN(13)  //key

#define USER_KEY                GPIO_ID_PIN(13)   //key
#define USER_KEY_PIN_IO_MUX     PERIPHS_IO_MUX_MTCK_U
#define USER_KEY_PIN_IO_NUM     GPIO_ID_PIN(13)
#define USER_KEY_PIN_IO_FUNC    FUNC_GPIO13

#if 0 //BSD 标准定义  校准M1

#define Get_ELE_PORT       GPIO_ID_PIN(5)  //CF   电量统计口
#define CF_PIN_IO_MUX      PERIPHS_IO_MUX_GPIO5_U
#define CF_PIN_IO_NUM      GPIO_ID_PIN(5)
#define CF_PIN_IO_FUNC     FUNC_GPIO5

#define Get_CUR_VOL    	   GPIO_ID_PIN(14)  //CF1  电流电压统计口
#define CF1_PIN_IO_MUX     PERIPHS_IO_MUX_MTMS_U
#define CF1_PIN_IO_NUM     GPIO_ID_PIN(14)
#define CF1_PIN_IO_FUNC    FUNC_GPIO14

#define CUR_VOL_SWITCH     GPIO_ID_PIN(3)   //SEL  电流电压切换  
#define SEL_PIN_IO_MUX     PERIPHS_IO_MUX_U0RXD_U
#define SEL_PIN_IO_NUM     GPIO_ID_PIN(3)
#define SEL_PIN_IO_FUNC    FUNC_GPIO3

#else //085

#define Get_ELE_PORT       GPIO_ID_PIN(5)  //CF   电量统计口
#define CF_PIN_IO_MUX      PERIPHS_IO_MUX_GPIO5_U
#define CF_PIN_IO_NUM      GPIO_ID_PIN(5)
#define CF_PIN_IO_FUNC     FUNC_GPIO5

#define Get_CUR_VOL    	   GPIO_ID_PIN(14)  //CF1  电流电压统计口
#define CF1_PIN_IO_MUX     PERIPHS_IO_MUX_MTMS_U
#define CF1_PIN_IO_NUM     GPIO_ID_PIN(14)
#define CF1_PIN_IO_FUNC    FUNC_GPIO14

#define CUR_VOL_SWITCH     GPIO_ID_PIN(12)   //SEL  电流电压切换  
#define SEL_PIN_IO_MUX     PERIPHS_IO_MUX_MTDI_U
#define SEL_PIN_IO_NUM     GPIO_ID_PIN(12)
#define SEL_PIN_IO_FUNC    FUNC_GPIO12

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define D_ERR_MODE                	0x00        //错误提示模式
#define D_NORMAL_MODE		      	0x10	    //正常工作模式
#define D_CAL_START_MODE		    0x21	    //校正模式，启动
#define D_CAL_END_MODE		        0x23	    //校正模式，完成
//--------------------------------------------------------------------------------------------

#define INCREMENT(x,n) (x+n) // 增量

typedef INT OPERATE_RET; // 操作结果返回值

#define OPRT_COMMON_START   0 // 通用返回值开始
#define OPRT_OK             INCREMENT(OPRT_COMMON_START,0)              // 执行成功
#define OPRT_COM_ERROR      INCREMENT(OPRT_COMMON_START,1)       // 通用错误
#define OPRT_INVALID_PARM   INCREMENT(OPRT_COMMON_START,2)    // 无效的入参
#define OPRT_MALLOC_FAILED  INCREMENT(OPRT_COMMON_START,3)   // 内存分配失败
#define OPRT_COMMON_END     OPRT_MALLOC_FAILED // 通用返回值结束

OPERATE_RET get_coefficient(void);

OPERATE_RET ele_cnt_init(INT mode);

void get_ele_par(UINT *P,UINT *V,UINT *I);

void get_ele(UINT *E);

#endif


