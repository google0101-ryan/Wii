#pragma once

#include <cstdint>

namespace NAND
{

void write32_starlet(uint32_t addr, uint32_t data);
uint32_t read32_starlet(uint32_t addr);

}