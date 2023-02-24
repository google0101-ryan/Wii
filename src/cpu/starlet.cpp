#include "starlet.h"

#include <src/memory/bus.h>

#include <cstring>
#include <cassert>

#include <bit>

#define printf(x, ...) 0

union PSR
{
    uint32_t val;
    struct
    {
        uint32_t mode : 5;
        uint32_t t : 1;
        uint32_t f : 1;
        uint32_t i : 1;
        uint32_t a : 1;
        uint32_t e : 1;
        uint32_t : 14;
        uint32_t j : 1;
        uint32_t : 2;
        uint32_t q : 1;
        uint32_t v : 1;
        uint32_t c : 1;
        uint32_t z : 1;
        uint32_t n : 1;
    } flags;
} cpsr, *cur_spsr;

uint32_t* registers[16];
uint32_t regs_system[16]; // Also used for user mode

bool is_thumb = false;

bool IsBranchExchange(uint32_t opcode)
{
    uint32_t branchAndExchangeFormat = 0b00000001001011111111111100010000;

    uint32_t formatMask = 0b00001111111111111111111111110000;

    uint32_t extractedFormat = opcode & formatMask;

    return extractedFormat == branchAndExchangeFormat;
}

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

bool CondPassed(uint8_t cond)
{
	switch (cond)
	{
    case 0b0000:
        return cpsr.flags.z;
    case 0b0001:
        return !cpsr.flags.z;
    case 0b0010:
        return cpsr.flags.c;
    case 0b0011:
        return !cpsr.flags.c;
	case 0b0101:
		return !cpsr.flags.n;
    case 0b1000:
        return cpsr.flags.c && !cpsr.flags.z;
    case 0b1001:
        return !cpsr.flags.c || cpsr.flags.z;
    case 0b1011:
        return cpsr.flags.v != cpsr.flags.n;
	case 0b1110:
		return true;
	default:
		printf("[Starlet]: Unknown cond-code 0x%02x\n", cond);
		exit(1);
	}
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

		if (!CondPassed((instr >> 28) & 0xF))
		{
			*registers[15] += 4;
			return;
		}

        printf("0x%08x (0x%08x): ", instr, *registers[15] - 8);

        if (IsBranchExchange(instr))
        {
            BranchExchange(instr);
        }
        else if (IsBlockTransfer(instr))
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

void Starlet::BranchExchange(uint32_t instr)
{
    uint8_t rn = instr & 0xF;

    uint32_t addr = *registers[rn];

    if (addr & 1)
        is_thumb = true;
    
    *registers[15] = (addr & ~1) + (is_thumb ? 4 : 8);

    printf("bx r%d\n", rn);
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

	if (*registers[15] == 0xFFFF0588)
	{
		printf("Panic!\n");
		exit(1);
	}
    
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

    uint32_t offset;

    if (!i)
    {
        offset = instr & 0xFFF;

        printf("%s%s r%d, [r%d, #%s%d]\n", l ? "ldr" : "str", b ? "b" : "", rd, rn, u ? "" : "-", offset);
    }
    else
    {
        uint8_t rm = instr & 0xf;
        uint16_t shift = (instr >> 4) & 0xFF;

        offset = *registers[rm];
        printf("%s%s r%d, [r%d, r%d]\n", l ? "ldr" : "str", b ? "b" : "", rd, rn, rm);

        if (shift & 1)
        {
            assert(0);
        }
        else
        {
            uint8_t shift_type = (instr >> 5) & 3;
            uint8_t shamt = (instr >> 7) & 0x1F;

            if (shamt)
            {
                switch (shift_type)
                {
                case 0:
                    offset <<= shamt;
                    break;
                case 1:
                    offset >>= shamt;
                    break;
                default:
                    printf("Unknown data-processing shift type %d\n", shift_type);
                    exit(1);
                }
            }
        }
    }

    uint32_t address = *registers[rn];

    if (p)
        address += u ? offset : -offset;
    
    if (l)
    {
        if (b)
            assert(0);
        else
            *registers[rd] = Bus::read32_starlet(address);
    }
    else
    {
        if (b)
            Bus::write8_starlet(address, *registers[rd]);
        else
    		Bus::write32_starlet(address, *registers[rd]);
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
    bool s = (instr >> 20) & 1;
    uint8_t opcode = (instr >> 21) & 0xF;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;

    uint32_t second_op;
    std::string second_op_disasm;

    if (i)
    {
        uint8_t imm = instr & 0xFF;
        int shamt = (instr >> 8) & 0xF;
		shamt *= 2;

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
            uint8_t shift_type = (instr >> 5) & 3;
            uint8_t shamt = (instr >> 7) & 0x1F;

            if (shamt)
            {
                switch (shift_type)
                {
                case 0:
                    second_op <<= shamt;
                    second_op_disasm += " lsl #" + std::to_string(shamt);
                    break;
                case 1:
                    second_op >>= shamt;
                    second_op_disasm += " lsr #" + std::to_string(shamt);
                    break;
                default:
                    printf("Unknown data-processing shift type %d\n", shift_type);
                    exit(1);
                }
            }
        }
    }

    bool modified_rd = true;

    switch (opcode)
    {
	case 0x00:
	{
		uint32_t result = *registers[rn] & second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = false;
        }

		printf("and r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		*registers[rd] = result;
		break;
	}
	case 0x01:
	{
		uint32_t result = *registers[rn] ^ second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = false;
        }

		printf("eor r%d, r%d, %s (0x%08x, 0x%08x)\n", rd, rn, second_op_disasm.c_str(), *registers[rn], second_op);

		*registers[rd] = result;
		break;
	}
	case 0x02:
	{
		uint32_t result = *registers[rn] - second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = !(second_op > *registers[rn]);
            cpsr.flags.v = ((second_op & (1 << 31)) != (*registers[rn]) & (1 << 31)) && (result & (1 << 31)) == (second_op & (1 << 31));
        }

		printf("sub r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		*registers[rd] = result;
		break;
	}
	case 0x04:
	{
		uint32_t result = *registers[rn] + second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = result < *registers[rn];
            cpsr.flags.v = ((second_op & (1 << 31)) == (*registers[rn]) & (1 << 31)) && (result & (1 << 31)) != (second_op & (1 << 31));
        }

		printf("add r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		*registers[rd] = result;
		break;
	}
	case 0x08:
	{
		uint32_t result = *registers[rn] & second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = false;
        }

		printf("tst r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		break;
	}
	case 0x0A:
	{
		uint32_t result = *registers[rn] - second_op;

        cpsr.flags.z = (result == 0);
        cpsr.flags.n = (result >> 31) & 1;
        cpsr.flags.c = !(second_op > *registers[rn]);
        cpsr.flags.v = ((second_op & (1 << 31)) != (*registers[rn]) & (1 << 31)) && (result & (1 << 31)) == (second_op & (1 << 31));

		printf("cmp r%d, %s (0x%08x, 0x%08x, 0x%08x)\n", rn, second_op_disasm.c_str(), *registers[rn], second_op, result);

		break;
	}
	case 0x0B:
	{
		uint32_t result = *registers[rn] + second_op;

		cpsr.flags.z = (result == 0);
		cpsr.flags.n = (result >> 31) & 1;
		cpsr.flags.c = result < *registers[rn];
        cpsr.flags.v = ((second_op & (1 << 31)) == (*registers[rn]) & (1 << 31)) && (result & (1 << 31)) != (second_op & (1 << 31));

		printf("cmn r%d, %s\n", rn, second_op_disasm.c_str());

		break;
	}
	case 0x0C:
	{
		uint32_t result = *registers[rn] | second_op;

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = false;
        }

		printf("orr r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		*registers[rd] = result;
		break;
	}
    case 0x0d:
        printf("mov r%d, %s\n", rd, second_op_disasm.c_str());
        *registers[rd] = second_op;
        break;
	case 0x0e:
	{
		uint32_t result = *registers[rn] & ~(second_op);

        if (s)
        {
            cpsr.flags.z = (result == 0);
            cpsr.flags.n = (result >> 31) & 1;
            cpsr.flags.c = false;
        }

		printf("bic r%d, r%d, %s\n", rd, rn, second_op_disasm.c_str());

		*registers[rd] = result;
		break;
	}
    default:
        printf("[Starlet]: Unknown data processing op 0x%x\n", opcode);
        exit(1);
    }

    if (rd == 15 && modified_rd)
        *registers[15] += 8;
    else
        *registers[15] += 4;
}
