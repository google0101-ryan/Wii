#pragma once

#include <cstdint>

namespace Starlet
{

void Reset();
void Clock();
void Dump();

void BlockTransfer(uint32_t instr);
void BranchLink(uint32_t instr);
void SingleDataTransfer(uint32_t instr);
void DataProcessing(uint32_t instr);

}