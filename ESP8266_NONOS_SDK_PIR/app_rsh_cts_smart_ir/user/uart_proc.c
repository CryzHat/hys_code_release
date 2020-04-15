/*
    ´¦ÀíÄ£¿éÓëmcuÖ±½ÓµÄ´®¿ÚÊý¾Ý
*/
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "common.h"
#include "upgrade.h"
#include "c_types.h"
#include "gpio.h"
#include "smartconfig.h"

#define UART_DATA_BUF_LEN			(1024)//1204	//byte

#define UART_DATA_PACKAGE_TIME_OUT	50//ms

#define WIFI_recvTaskQueueLen       32

static ETSTimer uart_timeout_timer;

LOCAL  os_event_t WIFI_recvTaskQueue[WIFI_recvTaskQueueLen];

//**********************************
extern struct espconn *udp_conn;
extern uint8 G_mac[6];
extern uint8 G_mode;

//**********************************


typedef struct
{
    uint8_t buf[UART_DATA_BUF_LEN];
    size_t size;
} uart_data_t;





uart_data_t *uart_data = NULL;

void ICACHE_FLASH_ATTR uart_data_send(uint8 *buf, uint16 len)
{
    DBG("uart_data_send:\n");
    debug_print_hex(buf,len);
    uart0_tx_buffer(buf,len);
}

//Í¸´«ÖÁÍø¹Ø

void smart_ap_config()
{

	os_printf("long press..\n");
	int MODE = 0;
	char buf[15]= {0};
	if(wifi_get_opmode() == STATION_MODE)
	{
		if(G_mode == SMARTCONFIG_MODE)
		{
			MODE = SOFTAP_MODE;
			os_printf(" change to SoftAP config mode after reboot\n");
			buf[0] = 0x55;buf[1] = 0xaa;buf[2] = 0xc2;
			buf[3] = G_mac[0];buf[4] =G_mac[1] ;buf[5] = G_mac[2];
			buf[6] = G_mac[3];buf[7] =G_mac[4] ;buf[8] = G_mac[5];

			buf[9] = 0x01;
			buf[10] = checksum(buf,10);
			uart_data_send(buf,11);
		}
		else
		{
			MODE = STATION_MODE;//
			os_printf(" change to Smartconfig mode after reboot\n");
			buf[0] = 0x55;buf[1] = 0xaa;buf[2] = 0xc2;
			buf[3] = G_mac[0];buf[4] =G_mac[1] ;buf[5] = G_mac[2];
			buf[6] = G_mac[3];buf[7] =G_mac[4] ;buf[8] = G_mac[5];

			buf[9] = 0x00;
			buf[10] = checksum(buf,10);
			uart_data_send(buf,11);
		}
	}
	else
	{
		MODE = STATION_MODE;
		os_printf(" change to Smartconfig mode after reboot\n");
	}

	system_rtc_mem_write(64,&MODE,sizeof(MODE));

	dev_config_clear();
	system_restore(); //»Ö¸´³ö³§ÉèÖÃ
	system_restart();

}
	

LOCAL void ICACHE_FLASH_ATTR uart_data_timeout_cb(void *arg)
{

    ETS_UART_INTR_DISABLE();
    uart_data_t **data = arg;

	//DBG("***** Uart Recv From RT400 ****\n");	
    
    debug_print_hex((*data)->buf, (*data)->size);
    
    //tcpserver_send((*data)->buf, (*data)->size);//------------------------------------------
/*******************************udpserver·¢ËÍ¸øÍø¹Ø*******************å**/
	if(((*data)->buf[0] == 0x55)&&((*data)->buf[1] == 0xaa))//55aa
	{
		char str[15]= {0};
		str[0] = 0x55;
		str[1] = 0xaa;
		if((*data)->buf[2] == 0xa0)
		{
			str[2] = 0xa0;
		}
		else if((*data)->buf[2] == 0xa1)
		{
			str[2] = 0xa1;
		}
		else if((*data)->buf[2] == 0xa2)
		{
			str[2] = 0xa2;
		}
		else if((*data)->buf[2] == 0xa3)
		{
			str[2] = 0xa3;
		}
		else if((*data)->buf[2] == 0xb0)
		{
			str[2] = 0xb0;
		}
		else if((*data)->buf[2] == 0xc0)
		{
			str[2] = 0xc0;
		}
		else if((*data)->buf[2] == 0xc1)
		{
			str[2] = 0xc1;
		}
		str[3] = G_mac[0];str[4] = G_mac[1];
		str[5] = G_mac[2];str[6] = G_mac[3];
		str[7] = G_mac[4];str[8] = G_mac[5];

		if((*data)->buf[2] == 0xa2)   //µÍµçÁ¿mcu¾¯±¨
		{
			str[9] = (*data)->buf[9];
			str[10] = checksum(str,10);
			espconn_sent(udp_conn, str, 11);

		}
		else if((*data)->buf[2] == 0xb0)  //×´Ì¬mcu±¨¸æ
		{
			str[9] = (*data)->buf[9];
			str[10] = (*data)->buf[10];
			str[11] = checksum(str,11);
			espconn_sent(udp_conn, str, 12);
		}
		else if((*data)->buf[2] == 0xc1) //²éÑ¯mcu»Ø¸´
		{
			str[9] = (*data)->buf[9];
			str[10] = (*data)->buf[10];
			str[11] = (*data)->buf[11];
			str[12] = checksum(str,12);
			espconn_sent(udp_conn, str, 13);
		}
		else if((*data)->buf[2] == 0xc2)
		{
			//smart ap ÅäÍø
			smart_ap_config();

		}
		else
		{
			str[9] = 0x00;
			str[10] = checksum(str,10);
			espconn_sent(udp_conn, str, 11);
		}	
		
		
	}

/***********************å********å****************å****************************å*****/
    (*data)->size = 0;
    ETS_UART_INTR_ENABLE();//
}

/*
 * @¹¦ÄÜ£º½«´®¿Ú0ÊÕµ½µÄÊý¾Ý½øÐÐ´ò°ü·¢ËÍ¸øapp
 */
void uart_data_recv_proc(uint8_t data)
{

    //DBG("##uart_data_callbcak:%02x\n",data);

    if(uart_data == NULL)
    {
        uart_data = (uart_data_t *)os_malloc(sizeof(uart_data_t));
        if(uart_data == NULL)
        {
            return;
        }
        
        uart_data->size = 0;
        
        TIMER_REGISTER(&uart_timeout_timer, uart_data_timeout_cb, &uart_data, UART_DATA_PACKAGE_TIME_OUT, 0);
    }

    uart_data->buf[uart_data->size++] = data;
	
    if(uart_data->size >= UART_DATA_BUF_LEN)
    {
        uart_data->size = 0;
    }
    
    TIMER_STOP(&uart_timeout_timer);
	TIMER_REGISTER(&uart_timeout_timer, uart_data_timeout_cb, &uart_data, UART_DATA_PACKAGE_TIME_OUT, 0);
    TIMER_START(&uart_timeout_timer, UART_DATA_PACKAGE_TIME_OUT, 0);
}

void ICACHE_FLASH_ATTR uart_data_recvTask(os_event_t *events)
{
    if(events->sig == 1)
    {
        uint8 vchar=(uint8)(events->par);
        
        uart_data_recv_proc(vchar);
    }
    else
    {
        DBG("uart_data_recvTask should not receive the sig =%d\n",events->sig);
    }
}

void ICACHE_FLASH_ATTR uart_proc_init()
{
    system_os_task(uart_data_recvTask, 1,WIFI_recvTaskQueue , WIFI_recvTaskQueueLen);
}

