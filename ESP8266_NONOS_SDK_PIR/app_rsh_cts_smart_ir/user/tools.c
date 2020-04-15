#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "common.h"
#include "upgrade.h"
#include "c_types.h"
#include "gpio.h"

#define INVALID_DATA_16 0xFFFF
#define INVALID_DATA_32 0xFFFFFFFF

////192.168.1.1 --> 0xC0A80101
void ICACHE_FLASH_ATTR str2ip(char *str, unsigned char ip[])
{
    uint8 *ptr,i=1;    
    
    //os_printf("str=%s\n",str);
    
    ip[0] = atoi(str);
    
    while(*str++ != '\0')
    {
        if(*str == '.')
        {
            ip[i++] = atoi(str+1);
            //os_printf("%02x\n",atoi(str+1));
        }
        
        if(i == 4)
        {
            break;
        }
    }          
    os_printf("[%s]===[%02x%02x%02x%02x]\n",str,ip[0],ip[1],ip[2],ip[3]);
}

//head不参与计算
char ICACHE_FLASH_ATTR checksum(char *str,int len)
{
    int i,sum=0;

    for(i=0; i<len; i++)
    {
        sum += str[i];
    }
	
    return (char)(sum&0xFF);
}

void ICACHE_FLASH_ATTR debug_print_hex(uint8_t *data, size_t size)
{
    size_t i;
    
    DBG("HEX:");
    
    for (i = 0; i < size; i++)
    {
        DBG("%02X ", data[i]);
    }
    DBG("\n");
}

/*
void ICACHE_FLASH_ATTR dev_config_read(DEV_INFO_ST *dev_info)
{
	int ret;
	
    ret = spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_2) * SPI_FLASH_SEC_SIZE,(uint32 *)dev_info, sizeof(DEV_INFO_ST));
    
	DBG("dev_config_read read ret=%d\n",ret);                   
}

void ICACHE_FLASH_ATTR dev_config_save(DEV_INFO_ST *dev_info)
{
	int ret;
	
    ret = spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_2);
    DBG("dev_config_save erase ret=%d\n",ret);
    
    ret = spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_2) * SPI_FLASH_SEC_SIZE,(uint32 *)dev_info, sizeof(DEV_INFO_ST));
	DBG("dev_config_save write ret=%d\n",ret);
}
*/
void ICACHE_FLASH_ATTR dev_config_clear()
{
    //spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_2);
}

#if 0

int ret ICACHE_FLASH_ATTR cal_config_read(ELE_CNT *dev_info)
{
	int ret;
    char buf[32]={0};
	
    ret = spi_flash_read((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0) * SPI_FLASH_SEC_SIZE,(uint32 *)buf, 32);

    os_memcpy((char *)dev_info,(char *)buf,sizeof(ELE_CNT));
    
	DBG("dev_config_read read ret=%d\n",ret);   
    return ret;
}

int ret  ICACHE_FLASH_ATTR cal_config_save(ELE_CNT *dev_info)
{
	int ret;
	char buf[32]={0};
    
    ret = spi_flash_erase_sector(ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0);
    DBG("dev_config_save erase ret=%d\n",ret);

    os_memcpy((char *)buf,(char *)dev_info,sizeof(ELE_CNT));
    
    ret = spi_flash_write((ESP_PARAM_START_SEC + ESP_PARAM_SAVE_0) * SPI_FLASH_SEC_SIZE,(uint32 *)buf, 32);
	DBG("dev_config_save write ret=%d\n",ret);

    return ret;
}
#endif
