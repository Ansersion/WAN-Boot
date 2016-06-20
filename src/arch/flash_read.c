#include <flash_read.h>

sint8_t flash_read_l(uint32_t addr, uint32_t * data)
{
	int i;
	uint32_t tmp;
	if(!data) {
		return -1;
	}
	*data = 0;
	for(i = 0; i < sizeof(long); i++) {
		tmp = *((uint8_t *)(addr + i));
		*data += (tmp << (8 * i));
	}
	return 0;
}
