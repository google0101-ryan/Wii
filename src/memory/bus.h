#pragma once

#include <cstdint>
#include <string>

namespace Bus
{

void LoadBoot0(std::string name);
void Dump();

uint8_t read8_starlet(uint32_t address);
uint32_t read32_starlet(uint32_t address);

void write32_starlet(uint32_t address, uint32_t data);
void write8_starlet(uint32_t address, uint32_t data);

}