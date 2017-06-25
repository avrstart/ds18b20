#include "include.h"

float  t1, t2;

int main(void)
{   
    if(ds18b20_init() == 0)  {
        //ERROR NO SENSORS
    }
    
    while(1){
        ds18b20_start_convert();
        for (uint32_t i=0; i<1000000; i++);   //wait 1000ms
        t1 = ds18b20_get_temp(0);             //get t from sensor 1
        t2 = ds18b20_get_temp(1);             //get t from sensor 2
    }

}