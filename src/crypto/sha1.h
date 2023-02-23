#pragma once

#include <cstdint>

namespace SHA
{

void write32_starlet(uint32_t address, uint32_t value);
uint32_t read32_starlet(uint32_t address);

}