#include "bus.h"
#include <fstream>
#include <cstdio>
#include <cstdlib>

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
}

uint32_t Bus::read32_starlet(uint32_t address)
{
    uint32_t data;

    if (address >= 0xFFFF0000)
    {
        data = *(uint32_t*)&boot0[address - 0xFFFF0000];
    }
    else
    {
        printf("[Bus/Starlet]: Read32 from unknown address 0x%08x\n", address);
        exit(1);
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
    else
    {
        printf("[Bus/Starlet]: Write32 to unknown address 0x%08x\n", address);
        exit(1);
    }
}
