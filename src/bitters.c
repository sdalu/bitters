#include "bitters/gpio.h"
#include "bitters/spi.h"

int 
bitters_init(void) 
{
    int rc = 0;
    
    if ((rc = bitters_gpio_init()) < 0)
	return rc;
	
    if ((rc = bitters_spi_init()) < 0)
	return rc;

    return rc;
}    
