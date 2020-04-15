/******************************************************************************
    模块与app直连通信
*******************************************************************************/

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "gpio.h"
#include "espconn.h"
#include "common.h"

struct espconn tcp_server;

void ICACHE_FLASH_ATTR tcpserver_send(char *pusrdata, unsigned short length)
{
    struct espconn *pespconn = (struct espconn *)&tcp_server;

    DBG("send data to ip=%d.%d.%d.%d,port=%d\n",IP2STR(tcp_server.proto.tcp->remote_ip),tcp_server.proto.tcp->remote_port);
    espconn_sent(pespconn, pusrdata,length);
}

LOCAL void ICACHE_FLASH_ATTR
tcp_server_send(void *arg, char *pusrdata, unsigned short length)
{    
    struct espconn *pespconn = arg;

    espconn_sent(pespconn, pusrdata,length);     
}

LOCAL void ICACHE_FLASH_ATTR
tcpserver_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    //透传至RT400
    int i;
    
    os_printf("\nTCP Server Recv:\n");

    for(i=0;i<length;i++)
    {
        os_printf("%02X ",pusrdata[i]);
    }    
    os_printf("\n");

    uart_data_send(pusrdata,length);//透传至RT400
}

LOCAL void ICACHE_FLASH_ATTR
tcpclient_discon_cb(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    DBG("## tcpclient_discon_cb ##\n");
    DBG("remote tcp port=%d\n",(pespconn)->proto.tcp->remote_port);
    DBG("remote tcp ip %d.%d.%d.%d\n", ((pespconn)->proto.tcp->remote_ip[0]), ((pespconn)->proto.tcp->remote_ip[1]),
        ((pespconn)->proto.tcp->remote_ip[2]), ((pespconn)->proto.tcp->remote_ip[3]));
}

LOCAL void ICACHE_FLASH_ATTR
tcpclient_connnect_cb(void *arg)
{
    struct espconn *pesp_conn = (struct espconn *)arg;

    DBG("tcpclient_connnect_cb !!! \r\n");
    DBG("Remote tcp port=%d\n",(pesp_conn)->proto.tcp->remote_port);
    DBG("remote tcp ip %d.%d.%d.%d\n", ((pesp_conn)->proto.tcp->remote_ip[0]), ((pesp_conn)->proto.tcp->remote_ip[1]),              ((pesp_conn)->proto.tcp->remote_ip[2]), ((pesp_conn)->proto.tcp->remote_ip[3]));
}

LOCAL void ICACHE_FLASH_ATTR
tcpclient_reconn_cb(void *arg, sint8 errType)
{
    struct espconn *pesp_conn = (struct espconn *)arg;

    DBG("tcpclient_reconn_cb !!!  errType=%d\r\n",errType);
    DBG("Remote tcp port=%d\n",(pesp_conn)->proto.tcp->remote_port);
    DBG("remote tcp ip %d.%d.%d.%d\n", ((pesp_conn)->proto.tcp->remote_ip[0]), ((pesp_conn)->proto.tcp->remote_ip[1]),
        ((pesp_conn)->proto.tcp->remote_ip[2]), ((pesp_conn)->proto.tcp->remote_ip[3]));
}

LOCAL void ICACHE_FLASH_ATTR
tcpserver_sent_cb(void *arg)
{
    DBG("## tcpserver_sent_cb ##\n");
    struct espconn *pespconn = (struct espconn *)arg;
    DBG("remote tcp port=%d\n",(pespconn)->proto.tcp->remote_port);
    DBG("remote tcp ip %d.%d.%d.%d\n\n", ((pespconn)->proto.tcp->remote_ip[0]), ((pespconn)->proto.tcp->remote_ip[1]),
        ((pespconn)->proto.tcp->remote_ip[2]), ((pespconn)->proto.tcp->remote_ip[3]));
}

/*---------------------------------------------------------------------
函数功能:创建tcp服务器
---------------------------------------------------------------------*/
void ICACHE_FLASH_ATTR tcp_server_init(void) //只能连接一个client
{
    DBG("Create TCP server\n");
    
    tcp_server.type = ESPCONN_TCP;
    tcp_server.proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    tcp_server.proto.tcp->local_port = 8267;

    espconn_regist_disconcb(&tcp_server, tcpclient_discon_cb);
    espconn_regist_connectcb(&tcp_server, tcpclient_connnect_cb);// register conn success cb
    espconn_regist_reconcb(&tcp_server, tcpclient_reconn_cb);// register conn success cb
    espconn_regist_recvcb(&tcp_server, tcpserver_recv_cb);
    espconn_regist_sentcb(&tcp_server, tcpserver_sent_cb);
    
    espconn_accept(&tcp_server);
    
    //espconn_regist_time(&tcp_server,30,0);//60 seconds timeout
}

