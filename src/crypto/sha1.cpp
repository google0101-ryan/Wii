#include "sha1.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <bit>

#include <src/memory/bus.h>

const uint32_t K[4] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

struct Sha1State
{
	uint32_t digest[5];
	uint8_t buf[64];
};

void ProcessMessage(Sha1State* state)
{
	uint32_t a = state->digest[0];
	uint32_t b = state->digest[1];
	uint32_t c = state->digest[2];
	uint32_t d = state->digest[3];
	uint32_t e = state->digest[4];

	uint32_t w[80] = {0};
	for (int i = 0; i < 64; i += 4)
	{
		uint8_t word[4] = {0};
		memcpy(word, state->buf+i, 4);
		w[(i / 4)] = (word[0] << 24);
		w[(i / 4)] |= (word[1] << 16);
		w[(i / 4)] |= (word[2] << 8);
		w[(i / 4)] |= word[3];
	}

	for (int t = 16; t < 80; t++)
	{
		uint32_t word = w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16];
		w[t] = std::rotl(word, 1);
	}

	for (int t = 0; t < 20; t++)
	{
		uint32_t temp = std::rotl(a, 5) 
						+ ((b & c) | ((!b) & d))
						+ e
						+ w[t]
						+ K[0];
		e = d;
		d = c;
		c = std::rotl(b, 30);
		b = a;
		a = temp;
	}

	for (int t = 20; t < 40; t++)
	{
		uint32_t temp = std::rotl(a, 5)
						+ (b ^ c ^ d)
						+ e
						+ w[t]
						+ K[1];
		
		e = d;
		d = c;
		c = std::rotl(b, 30);
		b = a;
		a = temp;
	}

	for (int t = 40; t < 60; t++)
	{
		uint32_t temp = std::rotl(a, 5)
						+ ((b & c) | (b & d) | (c & d))
						+ e
						+ w[t]
						+ K[2];
		
		e = d;
		d = c;
		c = std::rotl(b, 30);
		b = a;
		a = temp;
	}

	for (int t = 60; t < 80; t++)
	{
		uint32_t temp = std::rotl(a, 5)
						+ (b ^ c ^ d)
						+ e
						+ w[t]
						+ K[3];

		e = d;
		d = c;
		c = std::rotl(b, 30);
		b = a;
		a = temp;
	}

	state->digest[0] = state->digest[0] + a;
	state->digest[1] = state->digest[1] + b;
	state->digest[2] = state->digest[2] + c;
	state->digest[3] = state->digest[3] + d;
	state->digest[4] = state->digest[4] + e;
}

void Sha1_update(Sha1State* state, uint8_t* input, int len)
{
	assert(len % 64 == 0);
	for (int i = 0; i < len / 64; i++)
	{
		memcpy(state->buf, input+(i*64), 64);
		ProcessMessage(state);
	}
}

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

Sha1State state;

void SHA::write32_starlet(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x0d030000:
	{
		sha_ctrl.value = value;
		if (value & 0x80000000)
		{

			uint32_t og_buf = sha_src_address;
			uint8_t* buf = new uint8_t[(sha_ctrl.blocks + 1) * 64];

			for (int i = 0; i < (sha_ctrl.blocks + 1) * 64; i++)
			{
				buf[i] = Bus::read8_starlet(sha_src_address);
				sha_src_address++;
			}

			Sha1_update(&state, buf, (sha_ctrl.blocks + 1) * 64);

			printf("[SHA]: Hash for 0x%08x -> 0x%08x is 0x%08x%08x%08x%08x%08x\n", og_buf, sha_src_address, state.digest[0], state.digest[1], state.digest[2], state.digest[3], state.digest[4]);

			sha_ctrl.exec = 0;

			if (sha_ctrl.irq)
				assert(0);
		}
		else
		{
			printf("[SHA1]: Resetting SHA engine\n");
			memset(&state, 0, sizeof(state));
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
		printf("[SHA1]: Reading hash value h0\n");
		return state.digest[0];
	case 0x0d03000C:
		printf("[SHA1]: Reading hash value h1\n");
		return state.digest[1];
	case 0x0d030010:
		printf("[SHA1]: Reading hash value h2\n");
		return state.digest[2];
	case 0x0d030014:
		printf("[SHA1]: Reading hash value h3\n");
		return state.digest[3];
	case 0x0d030018:
		printf("[SHA1]: Reading hash value h4\n");
		return state.digest[4];
	}
}
