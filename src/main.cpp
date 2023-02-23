#include <src/memory/bus.h>
#include <src/cpu/starlet.h>
#include <src/memory/otp.h>
#include <src/memory/nand.h>

#include <csignal>

void sig(int)
{
    exit(1);
}

int main()
{
    Bus::LoadBoot0("boot0.bin");
    OTP::LoadOTP("keys.bin");
    NAND::LoadNAND("nand.bin");
    Starlet::Reset();

    std::signal(SIGINT, sig);

    std::atexit(Starlet::Dump);
    std::atexit(Bus::Dump);

    while (1)
        Starlet::Clock();
}