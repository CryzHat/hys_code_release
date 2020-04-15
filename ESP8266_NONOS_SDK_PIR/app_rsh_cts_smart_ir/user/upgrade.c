#if 1
/******************************************************************************
 * Copyright 2015-2018 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/3/06, v1.0 create this file.
*******************************************************************************/
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "upgrade.h"
#include "common.h"
#include "gpio.h"

#define AT_CUSTOM_UPGRADE

#ifdef AT_CUSTOM_UPGRADE
#endif

#if 1//def AT_UPGRADE_SUPPORT
#ifdef AT_CUSTOM_UPGRADE

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

/**/
struct espconn *pespconn = NULL;
struct upgrade_server_info *upServer = NULL;
static os_timer_t at_delay_check;
static struct espconn *pTcpServer = NULL;
static ip_addr_t host_ip;

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR at_upDate_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;

    if(server->upgrade_flag == true)
    {
        DBG("device_upgrade_success\r\n");   
        system_upgrade_reboot();
    }
    else
    {
        DBG("device_upgrade_failed\r\n");
    }

    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR at_upDate_discon_cb(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    uint8_t idTemp = 0;

    if(pespconn->proto.tcp != NULL)
    {
        os_free(pespconn->proto.tcp);
    }

    if(pespconn != NULL)
    {
        os_free(pespconn);
    }

    DBG("disconnect\r\n");
    DEBUG();
    if(system_upgrade_start(upServer) == false)
    {
        DEBUG();
    }
    else
    {
        DEBUG();
    }
}

/**
  * @brief  Udp server receive data callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
at_upDate_recv(void *arg, char *pusrdata, unsigned short len)
{
    struct espconn *pespconn = (struct espconn *)arg;
    char temp[32] = {0};
    uint8_t user_bin[12] = {0};
    uint8_t i = 0;

    os_timer_disarm(&at_delay_check);

    upServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));

    upServer->upgrade_version[5] = '\0';

    upServer->pespconn = pespconn;

    os_memcpy(upServer->ip, pespconn->proto.tcp->remote_ip, 4);

    upServer->port = pespconn->proto.tcp->remote_port;

    upServer->check_cb = at_upDate_rsp;
    
    upServer->check_times = 60000;

    if(upServer->url == NULL)
    {
        upServer->url = (uint8 *)os_zalloc(1024);
    }

    if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
    {
        os_memcpy(user_bin, "user2.bin", 10);
    }
    else if(system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
    {
        os_memcpy(user_bin, "user1.bin", 10);
    }

    os_sprintf(upServer->url,"GET /%s HTTP/1.1\r\nHost: "IPSTR"\r\n"pheadbuffer"",user_bin, IP2STR(upServer->ip));
}

LOCAL void ICACHE_FLASH_ATTR at_upDate_wait(void *arg)
{
    struct espconn *pespconn = arg;
    
    os_timer_disarm(&at_delay_check);

    if(pespconn != NULL)
    {
        espconn_disconnect(pespconn);
    }
    else
    {
        DEBUG();
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR at_upDate_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;

    os_timer_disarm(&at_delay_check);
    os_timer_setfn(&at_delay_check, (os_timer_func_t *)at_upDate_wait, pespconn);
    os_timer_arm(&at_delay_check, 5000, 0);

    DBG("at_upDate_sent_cb\r\n");
}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR at_upDate_connect_cb(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    uint8_t user_bin[9] = {0};
    char *temp = NULL;

    espconn_regist_disconcb(pespconn, at_upDate_discon_cb);
    espconn_regist_recvcb(pespconn, at_upDate_recv);////////
    espconn_regist_sentcb(pespconn, at_upDate_sent_cb);

    temp = (uint8 *)os_zalloc(512);

    os_sprintf(temp,"GET /v1/device/rom/?is_format_simple=true HTTP/1.0\r\nHost: "IPSTR"\r\n"pheadbuffer"",
               IP2STR(pespconn->proto.tcp->remote_ip));

    espconn_sent(pespconn, temp, os_strlen(temp));
    os_free(temp);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR at_upDate_recon_cb(void *arg, sint8 errType)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if(pespconn->proto.tcp != NULL)
    {
        os_free(pespconn->proto.tcp);
    }
    os_free(pespconn);

    DBG("disconnect\r\n");
    DEBUG();
    if(upServer != NULL)
    {
        os_free(upServer);
        upServer = NULL;
    }

}

/******************************************************************************
 * FunctionName : upServer_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upServer_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *) arg;
//  char temp[32];

    if(ipaddr == NULL)
    {
        return;
    }

    if(host_ip.addr == 0 && ipaddr->addr != 0)
    {
        if(pespconn->type == ESPCONN_TCP)
        {
            os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
            espconn_regist_connectcb(pespconn, at_upDate_connect_cb);
            espconn_regist_reconcb(pespconn, at_upDate_recon_cb);
            espconn_connect(pespconn);
        }
    }
}

extern uint8 web_server_addr[32];
extern uint8 web_server_port;

void ICACHE_FLASH_ATTR at_exeCmdCiupdate()
{    
    DBG("start update...\n");
    
    pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    pespconn->type = ESPCONN_TCP;
    pespconn->state = ESPCONN_NONE;
    pespconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    pespconn->proto.tcp->local_port = espconn_port();
    pespconn->proto.tcp->remote_port = web_server_port;

    host_ip.addr = ipaddr_addr(web_server_addr);

    os_memcpy(pespconn->proto.tcp->remote_ip, &host_ip.addr, 4);
    espconn_regist_connectcb(pespconn, at_upDate_connect_cb);
    espconn_regist_reconcb(pespconn, at_upDate_recon_cb);
    espconn_connect(pespconn);
    
    DEBUG();
}
#endif
#endif

#endif
