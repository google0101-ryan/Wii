#pragma once

#include <cstdint>
#include <string>

namespace NAND
{

void LoadNAND(std::string fname);

void write32_starlet(uint32_t addr, uint32_t data);
uint32_t read32_starlet(uint32_t addr);

}