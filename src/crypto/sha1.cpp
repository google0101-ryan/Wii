#include "sha1.h"

#include <openssl/sha.h> // Used for hashing because I don't understand SHA1
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <src/memory/bus.h>

static SHA_CTX sha1_ctx;

union SHA_CTRL
{
	uint32_t value;
	struct
	{
		uint32_t blocks : 10,
		: 19,
		err : 1,
		irq : 1,
		exec : 1;
	};
} sha_ctrl;

uint32_t sha_src_address;

void SHA::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d030000:
	{
		sha_ctrl.value = value;
		if (value & 0x80000000)
		{
			printf("[SHA1]: Running transfer\n");

			uint8_t* buf = new uint8_t[(sha_ctrl.blocks + 1) * 64];

			for (int i = 0; i < (sha_ctrl.blocks + 1) * 64; i += 4)
			{
				*(uint32_t*)&buf[i] = Bus::read32_starlet(sha_src_address);
				sha_src_address += 4;
			}
			
			SHA1_Update(&sha1_ctx, (void*)buf, (sha_ctrl.blocks + 1) * 64);

			sha_ctrl.exec = 0;

			if (sha_ctrl.irq)
				assert(0);
		}
		else
		{
			printf("[SHA1]: Resetting SHA engine\n");
			SHA1_Init(&sha1_ctx);
		}
		return;
	}
	case 0x0d030004:
		printf("Setting SHA src 0x%08x\n", value);
		sha_src_address = value;
		break;
	case 0x0d030008:
	case 0x0d03000C:
	case 0x0d030010:
	case 0x0d030014:
	case 0x0d030018:
		return;
	}
}

uint32_t SHA::read32_starlet(uint32_t address)
{
	switch (address)
	{
	case 0x0d030000:
		return sha_ctrl.value;
	}
}
