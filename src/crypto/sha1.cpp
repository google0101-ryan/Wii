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

uint32_t h[5];

void SHA::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d030000:
	{
		sha_ctrl.value = value;
		if (value & 0x80000000)
		{
			uint8_t* buf = new uint8_t[(sha_ctrl.blocks + 1) * 64];

			for (int i = 0; i < (sha_ctrl.blocks + 1) * 64; i++)
			{
				buf[i] = Bus::read8_starlet(sha_src_address);
				sha_src_address++;
			}
			
			SHA1(buf, (sha_ctrl.blocks + 1) * 64, (uint8_t*)h);

			printf("[SHA]: Hash for 0x%08x is 0x%08x%08x%08x%08x%08x\n", sha_src_address - ((sha_ctrl.blocks + 1) * 64), h[0], h[1], h[2], h[3], h[4]);

			sha_ctrl.exec = 0;

			if (sha_ctrl.irq)
				assert(0);
		}
		else
		{
			printf("[SHA1]: Resetting SHA engine\n");
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
	case 0x0d030008:
		return h[0];
	case 0x0d03000C:
		return h[1];
	case 0x0d030010:
		return h[2];
	case 0x0d030014:
		return h[3];
	case 0x0d030018:
		return h[4];
	}
}
