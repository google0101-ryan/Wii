#include "starlet.h"

#include <src/memory/bus.h>

#include <cstring>
#include <cassert>

#include <bit>

uint32_t* registers[16];
uint32_t regs_system[16]; // Also used for user mode

bool is_thumb = false;

bool IsBlockTransfer(uint32_t opcode)
{
    return ((opcode >> 25) & 0x7) == 0b100;
}

bool IsBranchLink(uint32_t opcode)
{
    return ((opcode >> 25) & 0x7) == 0b101;
}

bool IsSingleDataTransfer(uint32_t opcode)
{
    return ((opcode >> 26) & 0x3) == 0b01;
}

bool IsDataProcessing(uint32_t opcode)
{
    return ((opcode >> 26) & 0x3) == 0b00;
}

void Starlet::Reset()
{
    memset(regs_system, 0, sizeof(regs_system));
    for (int i = 0; i < 16; i++)
        registers[i] = &regs_system[i];
    
    *registers[15] = 0xFFFF0008; // Add 8 to account for prefetch (damn prefetch)
}

void Starlet::Clock()
{
    if (is_thumb)
    {
        assert(0);
    }
    else
    {
        uint32_t instr = Bus::read32_starlet(*registers[15] - 8);

        printf("0x%08x (0x%08x): ", instr, *registers[15] - 8);

        if (IsBlockTransfer(instr))
        {
            BlockTransfer(instr);
        }
        else if (IsBranchLink(instr))
        {
            BranchLink(instr);
        }
        else if (IsSingleDataTransfer(instr))
        {
            SingleDataTransfer(instr);
        }
        else if (IsDataProcessing(instr))
        {
            DataProcessing(instr);
        }
        else
        {
            printf("[Starlet]: Unknown ARM instruction 0x%08x\n", instr);
            exit(1);
        }
    }
}

void Starlet::Dump()
{
    for (int i = 0; i < 16; i++)
        printf("[Starlet]: r%d\t->\t0x%08x\n", i, *registers[i]);
}

void Starlet::BlockTransfer(uint32_t instr)
{
    bool p = (instr >> 24) & 1;
    bool u = (instr >> 23) & 1;
    bool s = (instr >> 22) & 1;
    bool w = (instr >> 21) & 1;
    bool l = (instr >> 20) & 1;

    uint8_t rn = (instr >> 16) & 0xF;

    uint16_t reg_list = instr & 0xffff;

    uint32_t addr = *registers[rn];

    if (l)
    {
        assert(0);
    }
    else
    {
        printf("stm r%d%s, {", rn, w ? "!" : "");
        for (int i = 0; i < 16; i++)
        {
            if (!(reg_list & (1 << i)))
                continue;
            printf("r%d, ", i);
            if (p)
                addr += u ? 4 : -4;
            Bus::write32_starlet(addr, *registers[i]);
            if (!p)
                addr += u ? 4 : -4;
        }
        printf("\b\b}\n");
    }

    if (w)
        *registers[rn] = addr;

    if (l && (reg_list & (1 << 15)))
        *registers[15] += 8;
    else
        *registers[15] += 4;
}

void Starlet::BranchLink(uint32_t instr)
{
    bool l = (instr >> 24) & 1;

    int32_t imm = ((int32_t)(((instr & 0xFFFFFF) << 2) << 6)) >> 6;

    if (l)
        *registers[14] = *registers[15] - 4;
    
    *registers[15] += imm;
    
    // Account for pipelining
    *registers[15] += 8;

    printf("b%s 0x%08x\n", l ? "l" : "", *registers[15] - 8);
}

void Starlet::SingleDataTransfer(uint32_t instr)
{
    bool i = (instr >> 25) & 1;
    bool p = (instr >> 24) & 1;
    bool u = (instr >> 23) & 1;
    bool b = (instr >> 22) & 1;
    bool w = (instr >> 21) & 1;
    bool l = (instr >> 20) & 1;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint16_t offset = instr & 0xFFF;

    printf("%s r%d, [r%d, #%d]\n", l ? "ldr" : "str", rd, rn, offset);

    uint32_t address = *registers[rn];

    if (p)
        address += u ? offset : -offset;
    
    if (l)
    {
        *registers[rd] = Bus::read32_starlet(address);
    }
    else
    {
        assert(0);
    }

    if (!p)
    {
        address += u ? offset : -offset;
        if (rn != rd)
            *registers[rn] = address;
    }
    if (w)
    {
        *registers[rn] = address;
    }

    if (rd == 15 && l)
    {
        *registers[15] += 8;
    }
    else
    {
        *registers[15] += 4;
    }
}

void Starlet::DataProcessing(uint32_t instr)
{
    bool i = (instr >> 25) & 1;
    uint8_t opcode = (instr >> 21) & 0xF;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;

    uint32_t second_op;
    std::string second_op_disasm;

    if (i)
    {
        uint8_t imm = instr & 0xFF;
        uint8_t shamt = (instr >> 8) & 0xF;

        second_op = std::rotr<uint32_t>(imm, shamt);

        second_op_disasm = "#" + std::to_string(imm);
        if (shamt)
            second_op_disasm += ", #" + std::to_string(shamt);
    }
    else
    {
        uint8_t rm = instr & 0xf;
        uint16_t shift = (instr >> 4) & 0xFF;

        second_op = *registers[rm];

        second_op_disasm = "r" + std::to_string(rm);

        if (shift & 1)
        {
            assert(0);
        }
        else
        {
            uint8_t shift_type = (instr >> 1) & 3;
            uint8_t shamt = (instr >> 7) & 0x1F;

            if (shamt)
            {
                assert(0);
            }
        }
    }

    bool modified_rd = true;

    switch (opcode)
    {
    case 0x0d:
        printf("mov r%d, %s\n", rd, second_op_disasm.c_str());
        *registers[rd] = second_op;
        break;
    default:
        printf("[Starlet]: Unknown data processing op 0x%x\n", opcode);
        exit(1);
    }

    if (rd == 15 && modified_rd)
        *registers[15] += 8;
    else
        *registers[15] += 4;
}
