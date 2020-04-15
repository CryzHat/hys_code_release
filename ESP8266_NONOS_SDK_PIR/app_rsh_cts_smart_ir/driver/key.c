#if 1
//公版袁总国标插座 按键驱动
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "user_interface.h"
#include "driver/key.h"

#define  GPIO_HIGH  1
#define  GPIO_LOW   0

os_timer_t key_noise_timer;//过滤超短干扰

volatile uint32 key_counter = 0;
volatile uint32 long_key = 0;

LOCAL void key_intr_handler(struct keys_param *keys);

/******************************************************************************
 * FunctionName : key_init_single
 * Description  : init single key's gpio and register function
 * Parameters   : uint8 gpio_id - which gpio to use
 *                uint32 gpio_name - gpio mux name
 *                uint32 gpio_func - gpio function
 *                key_function long_press - long press function, needed to install
 *                key_function short_press - short press function, needed to install
 * Returns      : single_key_param - single key parameter, needed by key init
*******************************************************************************/
struct single_key_param * ICACHE_FLASH_ATTR key_init_single(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, key_function long_press, key_function short_press)
{
    struct single_key_param *single_key = (struct single_key_param *)os_zalloc(sizeof(struct single_key_param));

    single_key->gpio_id = gpio_id;
    single_key->gpio_name = gpio_name;
    single_key->gpio_func = gpio_func;
    single_key->long_press = long_press;
    single_key->short_press = short_press;

    return single_key;
}

/******************************************************************************
 * FunctionName : key_init
 * Description  : init keys
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR key_init(struct keys_param *keys)
{
    uint8 i;

    ETS_GPIO_INTR_ATTACH(key_intr_handler, keys);

    ETS_GPIO_INTR_DISABLE();

    for (i = 0; i < keys->key_num; i++)
    {
        keys->single_key[i]->key_level = GPIO_HIGH;//zs

        PIN_FUNC_SELECT(keys->single_key[i]->gpio_name, keys->single_key[i]->gpio_func);

        gpio_output_set(0, 0, 0, GPIO_ID_PIN(keys->single_key[i]->gpio_id));

        gpio_register_set(GPIO_PIN_ADDR(keys->single_key[i]->gpio_id), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                          | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                          | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

        //clear gpio14 status
        GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(keys->single_key[i]->gpio_id));

        //enable interrupt
        gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_NEGEDGE);//zs 下降沿触发
    }

    ETS_GPIO_INTR_ENABLE();    
}

/******************************************************************************
 * FunctionName : key_100ms_cb
 * Description  : 50ms timer callback to check it's a real key push
 * Parameters   : single_key_param *single_key - single key parameter
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR key_100ms_cb(struct single_key_param *single_key)//一直等到释放
{
	key_counter++;
	
    if(GPIO_LOW == GPIO_INPUT_GET((GPIO_ID_PIN(0))))//zs
    {
		//os_printf(" low ,key_counter=%d\n",key_counter);

		if(key_counter >= 10*30) //3000 ms
		{
			//os_printf(" Long press\n");
			
			if(single_key->long_press && long_key == 0)
	        {
				long_key = 0xAA;
	            single_key->long_press();
	        }	       
	        
	        //等待释放 按键松开
	 	}
    }       
	else
	{
		//os_printf(" high ,key_counter=%d\n",key_counter);
		
		single_key->key_level = GPIO_HIGH;

		//key up
		os_timer_disarm(&single_key->key_100ms);
		
		long_key = 0;

		if(key_counter >= 5 && key_counter < 30) //80 ms  -- 300 ms
		{			
			//os_printf(" short press\n");
			
			if(single_key->short_press)
	        {
	            single_key->short_press();
	        }
		}
		else
		{
			//os_printf(" key noise or long key release \n");
		}
		
		key_counter = 0;		
		
		//下降沿  for next key pressed
		ETS_GPIO_INTR_ENABLE();    
        
		gpio_pin_intr_state_set(GPIO_ID_PIN(single_key->gpio_id), GPIO_PIN_INTR_NEGEDGE);//继续等待下降沿
		
	}    
}

/******************************************************************************
 * FunctionName : key_intr_handler
 * Description  : key interrupt handler
 * Parameters   : key_param *keys - keys parameter, which inited by key_init_single
 * Returns      : none
*******************************************************************************/
LOCAL void key_intr_handler(struct keys_param *keys) //只检测下降沿
{
    uint8 i;
    uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	ETS_GPIO_INTR_DISABLE();
    
	os_printf(" key intr\n");
    
    for (i = 0; i < keys->key_num; i++)
    {
        if (gpio_status & BIT(keys->single_key[i]->gpio_id))
        {
        	//os_printf(" intr 1...\n");
        	key_counter=0;
            //disable interrupt
            gpio_pin_intr_state_set(GPIO_ID_PIN(keys->single_key[i]->gpio_id), GPIO_PIN_INTR_DISABLE);

            //clear interrupt status
            GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(keys->single_key[i]->gpio_id));

            if(keys->single_key[i]->key_level == GPIO_HIGH)//
            {                    
                os_timer_disarm(&keys->single_key[i]->key_100ms);
                os_timer_setfn(&keys->single_key[i]->key_100ms, (os_timer_func_t *)key_100ms_cb, keys->single_key[i]);
                os_timer_arm(&keys->single_key[i]->key_100ms, 10, 1);
            }     
            else
            {
				//os_printf(" intr 3...\n");
            }
        }
    }
}
#endif

