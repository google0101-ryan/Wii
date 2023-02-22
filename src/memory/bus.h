#pragma once

#include <cstdint>
#include <string>

namespace Bus
{

void LoadBoot0(std::string name);

uint32_t read32_starlet(uint32_t address);

void write32_starlet(uint32_t address, uint32_t data);

}