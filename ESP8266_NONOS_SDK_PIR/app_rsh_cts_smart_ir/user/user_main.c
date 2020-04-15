/******************************************************************************

*******************************************************************************/

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "common.h"
#include "gpio.h"
#include "smartconfig.h"
#include "driver/key.h"

#define WIFI_STATE_LED_IO_NUM      12
#define WIFI_STATE_LED_IO_NAME     PERIPHS_IO_MUX_MTDI_U
#define WIFI_STATE_LED_IO_FUNC     FUNC_GPIO12

#define KEY_IO_NUM      0
#define KEY_IO_MUX 		PERIPHS_IO_MUX_GPIO0_U
#define KEY_IO_FUNC  	FUNC_GPIO0

#define USER_KEY                GPIO_ID_PIN(0)   //key
#define USER_KEY_PIN_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define USER_KEY_PIN_IO_NUM     GPIO_ID_PIN(0)
#define USER_KEY_PIN_IO_FUNC    FUNC_GPIO0

#define PROD_TEST_SSID "rsh_factory_test"

uint8 wifi_led_level;
os_timer_t   wifi_state_timer; //wifi×´Ì¬¼ì²â
os_timer_t smartconfig_check_timer;

uint8 G_mac[6];
uint8 G_mode;

os_timer_t led_timer;

int wifi_check_counter;
os_timer_t wifi_check_timer;
os_timer_t wifi_inform_timer;

os_timer_t factory_test_timer;//

LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[1];

#define CNT_FLASH_SEC 0x7d


void user_key_short_press(void);

void ICACHE_FLASH_ATTR user_wifi_led_timer_init(uint32 tt);
void ICACHE_FLASH_ATTR product_test_start();

void ICACHE_FLASH_ATTR STA_init(void)
{
    struct station_config inf;

    G_mode = DISCONNECTED_MODE;

    //¿ªÊ¼¶ÁÈ¡ÉÏÒ»´ÎµÄÅäÖÃ
    wifi_station_get_config(&inf);

    if(os_strlen(inf.ssid))
    {
        os_printf(" connect to Router %s\n",inf.ssid);

        wifi_station_connect();
        wifi_station_set_auto_connect(1);
    }
}

void ICACHE_FLASH_ATTR SoftAP_init(void)
{
    struct softap_config soft_ap;
    struct ip_info info;
    char macaddr[6];
    uint8 fix_ssid[33] = {0};

    G_mode = SOFT_AP_MODE;

    sint8 ret = wifi_set_opmode_current(SOFTAP_MODE);
    DBG(" softap ret=%d\n",ret);

    wifi_get_macaddr(STATION_IF, macaddr);

    os_sprintf(fix_ssid,"SPG_%02X%02X",macaddr[4],macaddr[5]);

    os_memset(soft_ap.ssid,0,sizeof(soft_ap.ssid));
    os_memset(soft_ap.password,0,sizeof(soft_ap.password));
    os_memcpy(soft_ap.ssid,fix_ssid,8);

    soft_ap.ssid_len        = 8;
    soft_ap.channel         = 6;
    soft_ap.authmode        = AUTH_OPEN;
    soft_ap.beacon_interval = 100;
    soft_ap.max_connection  = 2;
    soft_ap.ssid_hidden     = 0;

    ret = wifi_softap_set_config_current(&soft_ap);

    DBG("************new ap info************\n");
    DBG("soft_ap ssid=%s\n",soft_ap.ssid);
    DBG("soft_ap password=%s\n",soft_ap.password);
    DBG("soft_ap beacon_interval=%d\n",soft_ap.beacon_interval);
    DBG("soft_ap.ssid_hidden =%d\n",soft_ap.ssid_hidden);
    DBG("soft_ap.channel =%d\n",soft_ap.channel);
    DBG("soft_ap.authmode =%d\n",soft_ap.authmode);
    DBG("soft_ap.max_connection =%d\n",soft_ap.max_connection);
    DBG("************************\n\n");
}

void ICACHE_FLASH_ATTR smartconfig_check_timer_cb()
{
    os_timer_disarm(&smartconfig_check_timer);

    G_mode = DISCONNECTED_MODE;//¶ÏÍøÄ£Ê½

    system_soft_wdt_feed();

    smartconfig_stop();
}

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{
    switch(status)
    {
    case SC_STATUS_WAIT:
        os_printf("SC_STATUS_WAIT\n");
        break;
    case SC_STATUS_FIND_CHANNEL:
        os_printf("SC_STATUS_FIND_CHANNEL\n");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
        sc_type *type = pdata;

        if (*type == SC_TYPE_ESPTOUCH)
        {
            os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
        }
        else
        {
            os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
        }

        break;
    case SC_STATUS_LINK:
        os_printf("SC_STATUS_LINK,start link\n");
        struct station_config *sta_conf = pdata;

        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();




		
        break;
    case SC_STATUS_LINK_OVER:
        os_printf("SC_STATUS_LINK_OVER\n");

        if(pdata != NULL)
        {
            uint8 phone_ip[4] = {0};
            os_memcpy(phone_ip, (uint8*)pdata, 4);
            os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
        }



		
        smartconfig_stop();
        break;
    }
}

void ICACHE_FLASH_ATTR smart_config(int32 tt)
{
    DBG("smart config ver=%s\n",smartconfig_get_version());

    G_mode = SMARTCONFIG_MODE;

    wifi_station_disconnect();//Í£Ö¹Á¬½Ó

    wifi_station_set_auto_connect(FALSE);

    system_soft_wdt_feed();
    smartconfig_stop();
    system_soft_wdt_feed();

    smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);

    esptouch_set_timeout(tt);

    system_soft_wdt_feed();

    smartconfig_start(smartconfig_done);

    os_timer_disarm(&smartconfig_check_timer);
    os_timer_setfn(&smartconfig_check_timer, (os_timer_func_t *)smartconfig_check_timer_cb, NULL);
    os_timer_arm(&smartconfig_check_timer, 3*60*1000, 0);
}

void ICACHE_FLASH_ATTR switch_to_sta()
{
    os_timer_disarm(&factory_test_timer);

    smart_config(180);
}

#if 1 //for factory test

void scan_done(void *arg, STATUS status)
{
    DEBUG();

    DBG("Sys time %d\n",system_get_time()/1000);

    if(status == OK)
    {
        struct bss_info *bss_link = (struct bss_info *)arg;
        if(bss_link)
        {
            if(os_memcmp(bss_link->ssid,PROD_TEST_SSID,os_strlen(bss_link->ssid)) ==0)
            {
                if(bss_link->rssi  >= -60)
                {
                    DBG("-----------PROD Factory OK -----------\n");                    
                    G_mode = PROD_MODE_SUCCESS;

                    return;//for prodtest
                }
                else
                {
                    DBG("-----------PROD Factory FAILED -----------\n");

                    G_mode = PROD_MODE_FAILED;

                    //wifiÖ¸Ê¾µÆ³£ÁÁ
                    //GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), 0);
                    return;//for prodtest
                }
            }
        }
    }

    os_printf(" can't find AP factory_test,set SmartConfig mode \n");

    os_timer_disarm(&factory_test_timer);
    os_timer_setfn(&factory_test_timer, (os_timer_func_t *)switch_to_sta,0);
    os_timer_arm(&factory_test_timer, 100, 0);
}

void ICACHE_FLASH_ATTR product_test_start()
{
    os_timer_disarm(&factory_test_timer);

    //scan test AP
    struct scan_config config;

    os_memset(&config,0,sizeof(struct scan_config));

    config.ssid = PROD_TEST_SSID;

    DBG("product_test_start scan  factory_test AP\n");

    wifi_station_scan(&config,scan_done);
}
#endif

void ICACHE_FLASH_ATTR user_wifi_led_timer_init(uint32 tt);

LOCAL void ICACHE_FLASH_ATTR user_wifi_led_timer_cb(void)
{
    struct ip_info ipconfig;

    if(G_mode == PROD_MODE_SUCCESS)//½»Ìæ¿ìÉÁ
    {
        GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), wifi_led_level);//Ö¸Ê¾µÆoff
        wifi_led_level = (~wifi_led_level) & 0x01;//
    }
    else
    {
        wifi_led_level = (~wifi_led_level) & 0x01;//Î´Á¬½Ó ¿ìÉÁ  AP ÂýÉÁ

        GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), wifi_led_level);
    }
}

void ICACHE_FLASH_ATTR user_wifi_led_timer_init(uint32 tt)
{
    os_timer_disarm(&led_timer);
    os_timer_setfn(&led_timer, (os_timer_func_t *)user_wifi_led_timer_cb, NULL);
    os_timer_arm(&led_timer, tt, 1);
}

void user_key_short_press(void)
{
    DBG("user_key_short_press...\n");

    if(G_mode == PROD_MODE_SUCCESS)
    {
        
    }
    else if(G_mode == PROD_MODE_FAILED)
    {
        
    }
    else
    {

    }
}

//ÅäÍø
void user_key_long_press(void)
{
    os_printf("long press..\n");
    int MODE = 0;

    if(wifi_get_opmode() == STATION_MODE)
    {
        if(G_mode == SMARTCONFIG_MODE)
        {
            MODE = SOFTAP_MODE;
            os_printf(" change to SoftAP config mode after reboot\n");
        }
        else
        {
            MODE = STATION_MODE;//
            os_printf(" change to Smartconfig mode after reboot\n");
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

void ICACHE_FLASH_ATTR user_key_init()
{    
    single_key[0] = key_init_single(KEY_IO_NUM, KEY_IO_MUX, KEY_IO_FUNC,user_key_long_press, user_key_short_press);

    keys.key_num    = 1;
    keys.single_key = single_key;

    key_init(&keys);    
}




void user_rf_pre_init(void)
{

}

void ICACHE_FLASH_ATTR wifi_check_timer_cb()
{
    struct ip_info info;

    wifi_get_ip_info(STATION_IF,&info);

    os_printf("dev ip is %d.%d.%d.%d\n",(info.ip.addr>>0)&0xFF,(info.ip.addr>>8)&0xFF,(info.ip.addr>>16)&0xFF,(info.ip.addr>>24)&0xFF);
    os_printf("RSSI %d\n",wifi_station_get_rssi());

    if(info.ip.addr == 0 || wifi_station_get_rssi() == 31)
    {
        wifi_check_counter++;
    }
    else
    {
        wifi_check_counter = 0;
    }

    if(wifi_check_counter > 40)//120s
    {
        wifi_check_counter = 0;

        os_printf("wifi try to reconnect\n");

        wifi_station_disconnect();

        wifi_station_connect();

        wifi_station_set_auto_connect(1);
    }
}

char inform_counter = 0;

void ICACHE_FLASH_ATTR wifi_inform_timer_cb()
{
    os_printf("wifi_inform_timer %d\n",inform_counter);
    inform_counter++;
    if(inform_counter >= 10)
    {
        os_timer_disarm(&wifi_inform_timer);
    }
    wifi_inform_proc();
}

void ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt)
{
    uint8 i;

    switch (evt->event)
    {
    case EVENT_STAMODE_CONNECTED:
        DBG("dev connected to router\n");
        break;
    case EVENT_STAMODE_GOT_IP:
        DBG("dev got ip\n");

        G_mode = CONNECTED_MODE;

        os_timer_disarm(&smartconfig_check_timer);

        //report_ver_proc();

        wifi_check_counter = 0;

        os_timer_disarm(&wifi_check_timer);
        os_timer_setfn(&wifi_check_timer, (os_timer_func_t *)wifi_check_timer_cb,0);
        os_timer_arm(&wifi_check_timer, 30*1000, 1);

        inform_counter = 0;
        os_timer_disarm(&wifi_inform_timer);
        os_timer_setfn(&wifi_inform_timer, (os_timer_func_t *)wifi_inform_timer_cb,0);
        os_timer_arm(&wifi_inform_timer, 500, 1);

		uint8 temp = wifi_get_channel();
		spi_flash_erase_sector(CNT_FLASH_SEC);
		spi_flash_write((CNT_FLASH_SEC+0) * SPI_FLASH_SEC_SIZE,(uint32 *)&temp,sizeof(temp));
		os_printf("########### write channel == > %x\n",(uint8)temp);

		

		

        break;
    case EVENT_STAMODE_DISCONNECTED:

        G_mode = DISCONNECTED_MODE;

        DBG("dev disconnected from router\n");

        break;

    case EVENT_SOFTAPMODE_STACONNECTED:
        DBG(" sta is connected\n");
        break;

    case EVENT_SOFTAPMODE_STADISCONNECTED:
        DBG(" sta is disconnected\n");
        break;

    default:
        break;
    }
}

void ICACHE_FLASH_ATTR wfl_timer_cb()
{
    static char last_wf_stat = 0xff;
    char wf_stat = G_mode;

    if(last_wf_stat != wf_stat)
    {
        last_wf_stat = wf_stat;

        //INFO("wfl_timer_cb wf_stat:%d ,last_wf_stat:%d\n",wf_stat,last_wf_stat);

        switch(wf_stat)
        {        
        case PROD_MODE_FAILED://²ú²âÄ£Ê½Ê§°Ü
        {
            os_printf(" # Prod Test Fail #\n");
            os_timer_disarm(&led_timer);
        }
        break;
        case PROD_MODE_SUCCESS://²ú²âÄ£Ê½  ½»Ìæ¿ìÉÁ
        {
            os_printf(" # Prod Test Succ #\n");
            user_wifi_led_timer_init(250);
        }
        break;
        case SMARTCONFIG_MODE://ÅäÍøÄ£Ê½  ¿ìÉÁ
        {
            os_printf(" # Smart Config #\n");
            user_wifi_led_timer_init(250);
        }
        break;

        case SOFT_AP_MODE://softAPÅäÍøÄ£Ê½ ÂýÉÁ
        {
            os_printf(" # MODE_SOFTAP_CONFIG #\n");
            user_wifi_led_timer_init(1500);
        }
        break;

        case CONNECTED_MODE://
        {
            os_printf(" # MODE_CONNECTED_ROUTER #\n");
            
            os_timer_disarm(&led_timer);

            GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), 0);
            //GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), ((G_dev_info.sw)));//Ö¸Ê¾µÆ            
        }
        break;

        case DISCONNECTED_MODE://¶ÏÍøÀ¶µÆÂýÉÁ±íÊ¾¼ÌµçÆ÷¹Ø±Õ×´Ì¬ ºìµÆÂýÉÁ±íÊ¾¼ÌµçÆ÷Êä³ö×´Ì¬
        {
            os_printf(" # MODE_DISCONNECTED_ROUTER #\n");
            os_timer_disarm(&led_timer);        
            GPIO_OUTPUT_SET(GPIO_ID_PIN(WIFI_STATE_LED_IO_NUM), 1);
        }
        break;
        default:
            break;
        }
    }
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
    case FLASH_SIZE_4M_MAP_256_256:
        rf_cal_sec = 128 - 5;
        break;

    case FLASH_SIZE_8M_MAP_512_512:
        rf_cal_sec = 256 - 5;
        break;

    case FLASH_SIZE_16M_MAP_512_512:
    case FLASH_SIZE_16M_MAP_1024_1024:
        rf_cal_sec = 512 - 5;
        break;

    case FLASH_SIZE_32M_MAP_512_512:
    case FLASH_SIZE_32M_MAP_1024_1024:
        rf_cal_sec = 1024 - 5;
        break;

    default:
        rf_cal_sec = 0;
        break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR system_init_done()
{
   // tcp_server_init();//²»Ðètcp-------------------------------------------------------------------
    
    //uart_proc_init();
	//¶ÁÈ¡channelÐÅÏ¢
    uint8 temp;
    spi_flash_read((CNT_FLASH_SEC+0) * SPI_FLASH_SEC_SIZE,(uint32 *)&temp, sizeof(temp));
	uint8 channel = (uint8)temp;
	os_printf("read channel == %x  \n",channel);
	//Ð´Èërtc memory
	WRITE_PERI_REG(0x600011f4,1<<16|channel);
	

    wifi_get_macaddr(STATION_IF,G_mac);

    os_printf("#MAC#%02X:%02X:%02X:%02X:%02X:%02X\n\n",G_mac[0],G_mac[1],G_mac[2],G_mac[3],G_mac[4],G_mac[5]);

    system_set_os_print(1);//disable when release

    PIN_FUNC_SELECT(WIFI_STATE_LED_IO_NAME, WIFI_STATE_LED_IO_FUNC);
    PIN_FUNC_SELECT(USER_KEY_PIN_IO_MUX, USER_KEY_PIN_IO_FUNC);

    //wifi×´Ì¬¼à²â
    os_timer_disarm(&wifi_state_timer);
    os_timer_setfn(&wifi_state_timer, (os_timer_func_t *)wfl_timer_cb, NULL);
    os_timer_arm(&wifi_state_timer, 500, 1);

    wifi_set_event_handler_cb(wifi_handle_event_cb);

    //for test 
    char buf[6];//45 34 09 04 86

    buf[0] = 0x45;
    buf[1] = 0x34;
    buf[2] = 0x09;    
    buf[3] = 0x04;
    buf[4] = 0x86;   
	
    uart_data_send(buf,5);
	
	uart_proc_init();//ÊÕµ½uartÊý¾Ý½âÎö++å+å+++++++++++å++++++++++å++++++++++++++++++++å++++++++++++++++++å++++++++++++++

    //user_key_init();----------------------------------------------------------------

    user_create_udp_server();

    //ÅäÍøÄ£Ê½ÇÐ»»   AP <³¤°´°´¼ü5Ãë> STA
    int MODE = 0;
    int PROD_MODE = 0;

    system_rtc_mem_read(64,&MODE,sizeof(MODE));

    os_printf("MODE = %d\n",MODE);

    PROD_MODE = MODE;

    if(MODE != STATION_MODE && MODE != SOFTAP_MODE)
    {
        MODE = STATION_MODE;//default
    }

    if(MODE == SOFTAP_MODE)
    {
        os_printf(" SET softAP MODE\n");

        if(wifi_get_opmode() != SOFTAP_MODE)
        {
            wifi_set_opmode_current(SOFTAP_MODE);
        }
    }
    else
    {
        os_printf(" SET STA MODE\n");

        if(wifi_get_opmode() != STATION_MODE)
        {
            wifi_set_opmode_current(STATION_MODE);
        }
    }

    if(wifi_get_opmode() == STATION_MODE)
    {
        //¿ªÊ¼¶ÁÈ¡ÉÏÒ»´ÎµÄÅäÖÃ
        os_printf(" STA Mode\n");

        wifi_set_sleep_type(NONE_SLEEP_T);

        struct station_config inf;

        memset(&inf,0,sizeof(inf));

        wifi_station_get_config(&inf);

        if(strlen(inf.ssid))
        {
            os_printf("reconnect to wifi %s\n",inf.ssid);

            //²»½ø²ú²â
            STA_init();
        }
        else
        {
            os_printf("Start prod test mode\n"); //Ä¬ÈÏ½øÈëÅäÍøÄ£Ê½

            G_mode = DISCONNECTED_MODE;

            //É¨ÃèÈÈµã
            os_timer_disarm(&factory_test_timer);
            os_timer_setfn(&factory_test_timer, (os_timer_func_t *)product_test_start,0);
            os_timer_arm(&factory_test_timer, 100, 0);
        }
    }
    else if(wifi_get_opmode() == SOFTAP_MODE)
    {
        os_printf(" SOFTAP Mode\n");
        SoftAP_init();
    }
    else
    {
        os_printf(" Error Mode\n");
    }
//-----------------------------------------ÎÂÊª¶È--------------------------------------------------
    //Sht3x_Init();
    
    //s16 temp,humi;
    
   // SHT3X_GetTempAndHumi(0x2C06,&temp,&humi);//
//-------------------------------------------------------------------------------------------------
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void)
{
    uart_init(115200,74880);

    system_init_done_cb(system_init_done);

    os_printf("SDK version:%s\n", system_get_sdk_version());
    os_printf("FW Compiled date:%s--%s\n",__DATE__,__TIME__);
    os_printf("FW VERSION = %d.%d.%d\n",USER_VER_1,USER_VER_2,USER_VER_3);

    if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
    {
        os_printf("---sys run from bin1---\n");
    }
    else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
    {
        os_printf("---sys run from bin2---\n");
    }

    struct rst_info *rst_info = system_get_rst_info();

    os_printf("reset reason: %x\n", rst_info->reason);

    if(rst_info->reason == REASON_WDT_RST || rst_info->reason == REASON_EXCEPTION_RST || rst_info->reason == REASON_SOFT_WDT_RST)
    {
        if (rst_info->reason == REASON_EXCEPTION_RST)
        {
            os_printf("Fatal exception (%d):\n", rst_info->exccause);
        }
        os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                  rst_info->epc1, rst_info->epc2, rst_info->epc3, rst_info->excvaddr, rst_info->depc);
    }
}

