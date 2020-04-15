/*
    udp监听 局域网设备发现
*/
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "common.h"
#include "upgrade.h"
#include "c_types.h"
#include "my_cJSON.h"
#include "user_config.h"

LOCAL struct espconn udp_broadcast;//udp广播
os_timer_t delay_timer;
uint8 web_server_addr[32];
uint8 web_server_port;
extern uint8 G_mac[6];
struct station_config tmp_config;
extern uint8 G_led_change;

struct espconn *udp_conn;
extern os_timer_t save_timer;
extern void ICACHE_FLASH_ATTR save_timer_cb(void);
extern uint8 G_mode;

#define CNT_FLASH_SEC 0x7d

#if 0
os_timer_t   TH_state_timer; //温湿度状态检测
s16 temp,humi;

void ICACHE_FLASH_ATTR TH_state_timer_proc()
{
    os_printf("TH_state_timer_proc\n");    
    SHT3X_GetTempAndHumi(0x2C06,&temp,&humi);//
}
#endif
void ICACHE_FLASH_ATTR delay_ota_proc()
{
    os_printf("delay_ota_proc\n");
    os_timer_disarm(&delay_timer);

    at_exeCmdCiupdate();

    //升级的时候闪烁
}

void ICACHE_FLASH_ATTR delay_default_set_proc()
{
    os_printf("delay_default_set_proc\n");

    system_restore();
    system_restart();
}

//配网完成反馈
void ICACHE_FLASH_ATTR wifi_inform_proc()
{
    os_printf("report_state_proc\n");

    char buf[16] = {0};

    buf[0] = 0x55;
    buf[1] = 0xAA;

    buf[2] = 0xF4;

    buf[3] = G_mac[0];
    buf[4] = G_mac[1];
    buf[5] = G_mac[2];
    buf[6] = G_mac[3];
    buf[7] = G_mac[4];
    buf[8] = G_mac[5];

    struct ip_info info;

    wifi_get_ip_info(STATION_IF,&info);

    buf[9]  = (info.ip.addr>>0)&0xFF;
    buf[10] = (info.ip.addr>>8)&0xFF;
    buf[11] = (info.ip.addr>>16)&0xFF;
    buf[12] = (info.ip.addr>>24)&0xFF;

    os_printf("dev ip is %d.%d.%d.%d\n",buf[9],buf[10],buf[11],buf[12]);

    buf[13] = SMART_PIR_1;
    buf[14] = SMART_PIR_2;
    buf[15] = SMART_PIR_3;
    buf[16] = SMART_PIR_4;    

    buf[17] = checksum(buf,17);

    udp_broadcast.proto.udp->remote_port = 58266;
    udp_broadcast.proto.udp->remote_ip[0] = 255;
    udp_broadcast.proto.udp->remote_ip[1] = 255;
    udp_broadcast.proto.udp->remote_ip[2] = 255;
    udp_broadcast.proto.udp->remote_ip[3] = 255;

    os_printf("inform...\n");
    espconn_sent(&udp_broadcast, buf, 18);
}

void ICACHE_FLASH_ATTR report_ver_proc()
{
    os_printf("report_ver_proc\n");

    char buf[16] = {0};

    buf[0] = 0x55;
    buf[1] = 0xAA;

    buf[2] = 0xFE;

    buf[3] = G_mac[0];
    buf[4] = G_mac[1];
    buf[5] = G_mac[2];
    buf[6] = G_mac[3];
    buf[7] = G_mac[4];
    buf[8] = G_mac[5];

    buf[9]  = USER_VER_1;
    buf[10] = USER_VER_2;
    buf[11] = USER_VER_3;

    buf[12] = checksum(buf,12);

    if(udp_conn)
    {
        
        espconn_sent(udp_conn, buf, 13);
    }
}

//softAP 收到配网信息后保存 然后重启
void ICACHE_FLASH_ATTR inform_Callback(void *ret)
{
    char str[64]= {0};

    os_sprintf(str,"{\"MAC\":\"%02X:%02X:%02X:%02X:%02X:%02X\"}",G_mac[0],G_mac[1],G_mac[2],G_mac[3],G_mac[4],G_mac[5]);

    espconn_sent(udp_conn, str, os_strlen(str));

    if(wifi_get_opmode() != STATION_MODE)
    {
        DBG("inform_Callback Set sta mode\n");
        struct station_config sta_conf;
        int ret;

        ret = wifi_set_opmode_current(STATION_MODE);
        if(ret)
        {
            DBG(" Set STA mode success\n");

            os_memcpy(sta_conf.ssid,tmp_config.ssid,32);
            os_memcpy(sta_conf.password,tmp_config.password,64);
            sta_conf.bssid_set = 0;

            ret = wifi_station_set_config(&sta_conf);
            if(ret)
            {
                DBG("SoftAP save success,Reboot to STA \n");

                struct station_config inf;

                os_memset(&inf,0,sizeof(inf));

                wifi_station_get_config(&inf);

                DBG("SoftAP Config ssid      = %s\n",inf.ssid);
                DBG("SoftAP Config password  = %s\n",inf.password);
                DBG("SoftAP Config bssid_set = %d\n",inf.bssid_set);

                os_timer_disarm(&delay_timer);
                int MODE = 0;

                MODE = STATION_MODE;
                os_printf(" change to STA mode after reboot\n");

                system_rtc_mem_write(64,&MODE,sizeof(MODE));				
                system_restart();
            }
        }
    }
}

LOCAL void ICACHE_FLASH_ATTR udp_server_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    udp_conn = arg;
    remot_info *premot = NULL;
    char udp_data[128] = {};
    uint8 buf[64]= {0};

    if(length < 128)
    {
        os_memcpy(udp_data,pusrdata,length);
    }

    DBG("udp_server_recv_cb ");

    if(espconn_get_connection_info(udp_conn,&premot,0) == ESPCONN_OK)
    {
        udp_conn->proto.udp->remote_port = premot->remote_port;
        udp_conn->proto.udp->remote_ip[0] = premot->remote_ip[0];
        udp_conn->proto.udp->remote_ip[1] = premot->remote_ip[1];
        udp_conn->proto.udp->remote_ip[2] = premot->remote_ip[2];
        udp_conn->proto.udp->remote_ip[3] = premot->remote_ip[3];

        DBG("remote_port=%d remote_ip=%d.%d.%d.%d\n",\
            udp_conn->proto.udp->remote_port,\
            premot->remote_ip[0],premot->remote_ip[1],\
            premot->remote_ip[2],premot->remote_ip[3]);
    }

    if(udp_data[0] == '{')
    {
        my_cJSON *root = NULL;
        my_cJSON *js = NULL;

        DBG("Recv %s\n",pusrdata);

        root = my_cJSON_Parse(udp_data);
        if(root)
        {
            js = my_cJSON_GetObjectItem(root,"SSID");
            if(js && js->type == my_cJSON_String)
            {
                DBG("SSID = %s\n",js->valuestring);
                os_memcpy(tmp_config.ssid,js->valuestring,strlen(js->valuestring));
            }

            js = my_cJSON_GetObjectItem(root,"PASSWORD");
            if(js && js->type == my_cJSON_String)
            {
                DBG("SSID = %s\n",js->valuestring);
                os_memcpy(tmp_config.password,js->valuestring,strlen(js->valuestring));
            }

            os_timer_disarm(&delay_timer);
            os_timer_setfn(&delay_timer, inform_Callback, NULL);
            os_timer_arm(&delay_timer, 500, 1);

            my_cJSON_Delete(root);

            char str[64]= {0};

            os_sprintf(str,"{\"MAC\":\"%02X:%02X:%02X:%02X:%02X:%02X\"}",G_mac[0],G_mac[1],G_mac[2],G_mac[3],G_mac[4],G_mac[5]);

            espconn_sent(udp_conn, str, os_strlen(str));
        }
    }

    debug_print_hex(pusrdata,length);

    if(udp_data[0] == 0x55 && udp_data[1] == 0xAA && checksum(udp_data,length-1) == udp_data[length-1])
    {
        char cmd = udp_data[2];

        if(G_mac[0] == udp_data[3] &&
                G_mac[1] == udp_data[4] &&
                G_mac[2] == udp_data[5] &&
                G_mac[3] == udp_data[6] &&
                G_mac[4] == udp_data[7] &&
                G_mac[5] == udp_data[8])
        {
            os_printf(" Check DATA OK \n");
        }
        else
        {
            os_printf(" Check DATA Failed cmd %02x\n",cmd);

            if(cmd != 0xF0 && cmd != 0xF2)
            {
                os_printf("error cmd\n");
                return;
            }
        }
        
        os_printf("cmd = %02x\n",cmd);
        
        switch(cmd)
        {

		case 0xc0:
		{
		/*********************WIFI--uart--->MCU*调整******************************/
			buf[0] = 0x55;buf[1] = 0xaa;
			buf[2] = 0xc0;	  
			buf[3] = G_mac[0];buf[4] = G_mac[1];
			buf[5] = G_mac[2];buf[6] = G_mac[3];
			buf[7] = G_mac[4];buf[8] = G_mac[5];
		//XX YY ZZ 
			buf[9] = udp_data[9];buf[10] = udp_data[10];buf[11] = udp_data[11];
			buf[12] =  checksum(buf,12);
			uart_data_send(buf,13);
			break;
		}
		case 0xc1:
		{
			/*********************WIFI--uart--->MCU*查询******************************/
			buf[0] = 0x55;buf[1] = 0xaa;
			buf[2] = 0xc1;
			buf[3] = G_mac[0];buf[4] = G_mac[1];
			buf[5] = G_mac[2];buf[6] = G_mac[3];
			buf[7] = G_mac[4];buf[8] = G_mac[5];
			buf[9] = 0x00;
			buf[10] =  checksum(buf,10);
			uart_data_send(buf,11);	
			break;
		}

        case 0xF0://find dev 指定pid查询设备      ###
        {
            DBG(" CMD F0 \n");       
                        
            //SMART_IR
            if(udp_data[3] == SMART_PIR_1 && udp_data[4] == SMART_PIR_2 
                && udp_data[5] == SMART_PIR_3 && udp_data[6] == SMART_PIR_4) 
            {

                buf[0] = 0x55;
                buf[1] = 0xAA;

                buf[2] = 0xF0;

                buf[3] = G_mac[0];
                buf[4] = G_mac[1];
                buf[5] = G_mac[2];
                buf[6] = G_mac[3];
                buf[7] = G_mac[4];
                buf[8] = G_mac[5];

                struct ip_info info;

                wifi_get_ip_info(STATION_IF,&info);

                buf[9]  = (info.ip.addr>>0)&0xFF;
                buf[10] = (info.ip.addr>>8)&0xFF;
                buf[11] = (info.ip.addr>>16)&0xFF;
                buf[12] = (info.ip.addr>>24)&0xFF;

                os_printf("dev ip is %d.%d.%d.%d\n",buf[9],buf[10],buf[11],buf[12]);

                buf[13] = checksum(buf,13);

                espconn_sent(udp_conn, buf, 14);
            }
            break;
        }
        case 0xF1://ota      ###
        {
            DBG(" CMD F1 \n");

            char value = udp_data[9];

            os_printf("web server %d.%d.%d.%d:%d\n",udp_data[9],udp_data[10],udp_data[11],udp_data[12],udp_data[13]);

            os_sprintf(web_server_addr,"%d.%d.%d.%d",udp_data[9],udp_data[10],udp_data[11],udp_data[12]);

            web_server_port = udp_data[13];

            buf[0] = 0x55;
            buf[1] = 0xAA;

            buf[2] = 0xF1;

            buf[3] = G_mac[0];
            buf[4] = G_mac[1];
            buf[5] = G_mac[2];
            buf[6] = G_mac[3];
            buf[7] = G_mac[4];
            buf[8] = G_mac[5];

            buf[9] = 0x00;
            
            buf[10] = checksum(buf,10);

            espconn_sent(udp_conn, buf, 11);

            os_timer_disarm(&delay_timer);
            os_timer_setfn(&delay_timer, (os_timer_func_t *)delay_ota_proc, 0);
            os_timer_arm(&delay_timer, 1*1000, 0);

            G_mode = OTA_MODE;

            break;
        }
        case 0xF2://get all devs   ###
        {                    
            
            DBG(" CMD F2\n" );

            buf[0] = 0x55;
            buf[1] = 0xAA;

            buf[2] = 0xF2;

            buf[3] = G_mac[0];
            buf[4] = G_mac[1];
            buf[5] = G_mac[2];
            buf[6] = G_mac[3];
            buf[7] = G_mac[4];
            buf[8] = G_mac[5];

            struct ip_info info;

            wifi_get_ip_info(STATION_IF,&info);

            buf[9]  = (info.ip.addr>>0)&0xFF;
            buf[10] = (info.ip.addr>>8)&0xFF;
            buf[11] = (info.ip.addr>>16)&0xFF;
            buf[12] = (info.ip.addr>>24)&0xFF;

            os_printf("dev ip is %d.%d.%d.%d\n",buf[9],buf[10],buf[11],buf[12]);

            buf[13] = SMART_PIR_1;
            buf[14] = SMART_PIR_2;
            buf[15] = SMART_PIR_3;
            buf[16] = SMART_PIR_4;

            buf[17] = checksum(buf,17);

            espconn_sent(udp_conn, buf, 18);

            break;
        }
        case 0xF3://get rssi    ###
        {
            DBG(" CMD F3 %02x\n",udp_data[9]);
            char type = udp_data[9];


            buf[0] = 0x55;
            buf[1] = 0xAA;

            buf[2] = 0xF3;

            buf[3] = G_mac[0];
            buf[4] = G_mac[1];
            buf[5] = G_mac[2];
            buf[6] = G_mac[3];
            buf[7] = G_mac[4];
            buf[8] = G_mac[5];

            sint8 rssi = wifi_station_get_rssi();

            if(rssi < 10)
                buf[9] = 0-rssi;
            else
                buf[9] = 0;

            buf[10] = checksum(buf,10);

            espconn_sent(udp_conn, buf, 11);

            break;
        }
        case 0xFE://查询固件版本     ###
        {
            report_ver_proc();
            break;
        }
        case 0xFF://default set     ###
        {
            DBG(" CMD FF \n");

            char value = udp_data[9];

            buf[0] = 0x55;
            buf[1] = 0xAA;

            buf[2] = 0xFF;

            buf[3] = G_mac[0];
            buf[4] = G_mac[1];
            buf[5] = G_mac[2];
            buf[6] = G_mac[3];
            buf[7] = G_mac[4];
            buf[8] = G_mac[5];

            buf[9] = 0x00;
            buf[10] = checksum(buf,10);

            espconn_sent(udp_conn, buf, 11);

            os_timer_disarm(&delay_timer);
            os_timer_setfn(&delay_timer, (os_timer_func_t *)delay_default_set_proc, 0);
            os_timer_arm(&delay_timer, 500, 0);

            break;
        }
        default:
            break;
        }
    }
}

/*----------------------------------------------------------------------------
函数功能:建立udp server监听60000
---------------------------------------------------------------------------*/
void ICACHE_FLASH_ATTR user_create_udp_server()
{
    udp_broadcast.type = ESPCONN_UDP;
    udp_broadcast.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    udp_broadcast.proto.udp->local_port = 8266;

    espconn_regist_recvcb(&udp_broadcast, udp_server_recv_cb);

    espconn_create(&udp_broadcast);

	#if 0
    os_timer_disarm(&TH_state_timer);
    os_timer_setfn(&TH_state_timer, TH_state_timer_proc, NULL);
    //os_timer_arm(&TH_state_timer, 5000, 1);
	#endif
}

