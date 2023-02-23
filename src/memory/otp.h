#pragma once

#include <cstdint>
#include <string>

namespace OTP
{

void LoadOTP(std::string otp_name);

void write32_starlet(uint32_t address, uint32_t data);
uint32_t read32_starlet(uint32_t address);

}