#include "otp.h"

#include <fstream>
#include <cstring>
#include <assert.h>

uint8_t otp[0x20*4];

void OTP::LoadOTP(std::string otp_name)
{
    std::ifstream one_tp(otp_name, std::ios::binary);

    char sig[13];
    sig[12] = '\0';
    one_tp.read(sig, 12);

    if (strncmp(sig, "BackupMii v1", 12))
    {
        printf("[OTP]: Error: Please supply BackupMii format OTP\n");
        exit(1);
    }

    one_tp.seekg(0x100); // Offset of OTP data
    one_tp.read((char*)otp, 0x80);

	char header[256];
	one_tp.seekg(0);
	one_tp.read(header, 256);

	printf("%s\n", header);
}

uint32_t otp_ctrl = 0;
uint32_t otp_data = 0;

void OTP::write32_starlet(uint32_t address, uint32_t data)
{
    switch (address)
    {
    case 0x0d8001ec:
        otp_ctrl = data;
        if (otp_ctrl & (1 << 31))
        {
			int index = otp_ctrl & 0x1F;
            printf("[OTP]: Reading data from 0x%x\n", otp_ctrl & 0x1F);
            otp_data = __bswap_32(otp[index << 2]);
        }
        break;
    }
}

uint32_t OTP::read32_starlet(uint32_t address)
{
    assert(address == 0x0d8001f0);

    return otp_data;
}
