#include "sha1.h"

#include <openssl/sha.h> // Used for hashing because I don't understand SHA1
#include <cstdio>
#include <cstdlib>

static SHA_CTX sha1_ctx;

union SHA_CTRL
{
	uint32_t value;
} sha_ctrl;

uint32_t sha_src_address;

void SHA::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d030000:
	{
		sha_ctrl.value = value;
		if (value & (1 << 31))
		{
			printf("[SHA1]: Starting SHA transfer\n");
			exit(1);
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