#include "nand.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include <src/memory/bus.h>

uint8_t* nand;

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

union NAND_CTRL
{
	uint32_t value;
	struct
	{
		uint32_t blocklen : 12,
		ecc : 1,
		rd : 1,
		wr : 1,
		wait : 1,
		cmd : 8,
		a1 : 1,
		a2 : 1,
		a3 : 1,
		a4 : 1,
		a5 : 1,
		err : 1,
		irq : 1,
		exec : 1;
	};
} nand_ctrl;

uint32_t databuf;
uint32_t eccbuf;
uint32_t addr1;
uint32_t addr2;

void NAND::LoadNAND(std::string fname)
{
	std::ifstream n(fname, std::ios::binary | std::ios::ate);

	size_t size = n.tellg();
	nand = new uint8_t[size];
	
	n.seekg(0, std::ios::beg);
	n.read((char*)nand, size);
}

void NAND::write32_starlet(uint32_t addr, uint32_t data)
{
	switch (addr)
	{
	case 0x0d010000:
		nand_ctrl.value = data;
		if (nand_ctrl.exec)
		{
			printf("[NAND]: Running command 0x%02x\n", nand_ctrl.cmd);
			switch (nand_ctrl.cmd)
			{
			case 0x00:
				printf("[NAND]: Read prefix\n");
				nand_ctrl.exec = 0;
				break;
			case 0x30:
			{
				uint32_t off = addr2 * 0x840;

				uint8_t* local_buf = new uint8_t[nand_ctrl.blocklen];

				memcpy(local_buf, (void*)&nand[off], nand_ctrl.blocklen - 64);

				uint8_t* ecc_buf = new uint8_t[64];

				for (int i = 0; i < nand_ctrl.blocklen - 64; i++)
				{
					Bus::write8_starlet(databuf + i, local_buf[i]);
				}
				printf("NAND transfer done, transfering ECC\n");
				for (int i = nand_ctrl.blocklen - 64, pos = 0; i < nand_ctrl.blocklen; i++, pos++)
				{
					Bus::write8_starlet(eccbuf + pos, local_buf[i]);
				}

				nand_ctrl.exec = 0;

				delete ecc_buf;
				delete local_buf;

				break;
			}
			case 0xff:
				printf("[NAND]: Reset\n");
				nand_ctrl.value = 0;
				break;
			default:
				exit(1);
			}
		}
		break;
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
	case 0x0d010000:
		return nand_ctrl.value;
	case 0x0d010004:
		return nand_cfg.data;
	}
}
