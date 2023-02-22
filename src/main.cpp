#include <src/memory/bus.h>
#include <src/cpu/starlet.h>

int main()
{
    Bus::LoadBoot0("boot0.bin");
    Starlet::Reset();

    std::atexit(Starlet::Dump);

    while (1)
        Starlet::Clock();
}