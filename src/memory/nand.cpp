#include "nand.h"

#include <cstdio>

union NAND_CONF
{
	uint32_t data;
	struct
	{
		uint32_t attr4 : 8;
		uint32_t attr3 : 8;
		uint32_t attr2 : 7;
		uint32_t attr1 : 4;
		uint32_t enable : 1;
		uint32_t attr0 : 4;
	};
} nand_cfg;

uint32_t databuf;
uint32_t eccbuf;
uint32_t addr1;
uint32_t addr2;

void NAND::write32_starlet(uint32_t addr, uint32_t data)
{
	switch (addr)
	{
	case 0x0d010004:
		nand_cfg.data = data;
		if (nand_cfg.enable)
			printf("[NAND]: Enabled NAND\n");
		break;
	case 0x0d010008:
		addr1 = data;
		printf("[NAND]: Setting addr1 to 0x%08x\n", addr1);
		break;
	case 0x0d01000C:
		addr2 = data;
		printf("[NAND]: Setting addr2 to 0x%08x\n", addr2);
		break;
	case 0x0d010010:
		databuf = data;
		printf("[NAND]: Setting data buf to 0x%08x\n", databuf);
		break;
	case 0x0d010014:
		eccbuf = data;
		printf("[NAND]: Setting ecc buf to 0x%08x\n", databuf);
		break;
	}
}

uint32_t NAND::read32_starlet(uint32_t addr)
{
	switch (addr)
	{
	case 0x0d010004:
		return nand_cfg.data;
	}
}
