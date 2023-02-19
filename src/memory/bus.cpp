#include "bus.h"
#include <fstream>
#include <cstdio>
#include <cstdlib>

#include <src/crypto/aes.h>
#include <src/crypto/sha1.h>
#include <src/memory/nand.h>
#include <src/memory/otp.h>

std::ofstream console;

uint8_t* boot0;
uint8_t sram[0x18000];

void Bus::LoadBoot0(std::string name)
{
    std::ifstream boot(name, std::ios::ate | std::ios::binary);

    size_t size = boot.tellg();
    boot.seekg(0, std::ios::beg);

    boot0 = new uint8_t[size];
    boot.read((char*)boot0, size);

    boot.close();

	console.open("error_console.dump");
}

void Bus::Dump()
{
	std::ofstream sr("sram.dump");

	for (int i = 0; i < 0x18000; i++)
		sr << sram[i];
	
	sr.close();
}

uint8_t Bus::read8_starlet(uint32_t address)
{
	if (address >= 0x0D400000 && address < 0x0D418000)
    {
        return sram[address - 0x0D400000];
    }
	else
	{
		switch (address)
		{
		default:
			printf("[Bus/Starlet]: Read8 from unknown address 0x%08x\n", address);
			exit(1);
		}
	}
}

uint32_t Bus::read32_starlet(uint32_t address)
{
    uint32_t data;

    if (address >= 0xFFFF0000)
    {
        data = *(uint32_t*)&boot0[address - 0xFFFF0000];
    }
    else if (address >= 0x0D400000 && address < 0x0D418000)
    {
        data = *(uint32_t*)&sram[address - 0x0D400000];
    }
	else
	{
	
		switch (address)
		{
		case 0x0d010000:
		case 0x0d010004:
			return NAND::read32_starlet(address);
		case 0x0d020000:
			return AES::read32_starlet(address);
		case 0x0d030000:
		case 0x0d030008:
		case 0x0d03000C:
		case 0x0d030010:
		case 0x0d030014:
		case 0x0d030018:
			return SHA::read32_starlet(address);
		case 0x0d8000c0 ... 0x0d8000fc:
			return 0;
		case 0x0d8001f0:
			return OTP::read32_starlet(address);
		default:
			printf("[Bus/Starlet]: Read32 from unknown address 0x%08x\n", address);
			exit(1);
		}
	
	}

    return __bswap_32(data);
}

void Bus::write32_starlet(uint32_t address, uint32_t data)
{
    if (address >= 0x0D400000 && address < 0x0D418000)
    {
        *(uint32_t*)&sram[address - 0x0D400000] = __bswap_32(data);
        return;
    }
	
	switch (address)
	{
	case 0x0d800060:
		break;
	case 0x0d010000:
	case 0x0d010004:
	case 0x0d010008:
	case 0x0d01000C:
	case 0x0d010010:
	case 0x0d010014:
		NAND::write32_starlet(address, data);
		break;
	case 0x0d020000:
	case 0x0d020004:
	case 0x0d020008:
	case 0x0d02000C:
	case 0x0d020010:
		AES::write32_starlet(address, data);
		break;
	case 0x0d030000:
	case 0x0d030004:
	case 0x0d030008:
	case 0x0d03000C:
	case 0x0d030010:
	case 0x0d030014:
	case 0x0d030018:
		SHA::write32_starlet(address, data);
		break;
    case 0x0d8001ec:
		OTP::write32_starlet(address, data);
		break;
	case 0x0d8000c0 ... 0x0d8000df:
		break;
	case 0x0d8000e0:
		console << (char)(data >> 16);
		break;
	case 0x0d8000e1 ... 0x0d8000fc:
		break;
	default:
        printf("[Bus/Starlet]: Write32 to unknown address 0x%08x\n", address);
        exit(1);
	}
}

void Bus::write8_starlet(uint32_t address, uint32_t data)
{
	if (address >= 0x0D400000 && address < 0x0D418000)
    {
        sram[address - 0x0D400000] = data;
        return;
    }
	else
	{
		switch (address)
		{
		default:
			printf("[Bus/Starlet]: Write8 to unknown address 0x%08x\n", address);
			exit(1);
		}
	}
}
