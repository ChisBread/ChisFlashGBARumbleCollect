## Building

`$DEVKITARM/bin/arm-none-eabi-gcc -mcpu=arm7tdmi -nostartfiles -nodefaultlibs -mthumb -fPIE -Os -fno-toplevel-reorder payload.c -T payload.ld -o payload.elf; $DEVKITARM/bin/arm-none-eabi-objcopy -O binary payload.elf payload.bin; xxd -i payload.bin > payload_bin.c ; gcc -g patcher.c payload_bin.c`

for mingw, use

`$DEVKITARM/bin/arm-none-eabi-gcc -mcpu=arm7tdmi -nostartfiles -nodefaultlibs -mthumb -fPIE -Os -fno-toplevel-reorder payload.c -T payload.ld -o payload.elf; $DEVKITARM/bin/arm-none-eabi-objcopy -O binary payload.elf payload.bin; xxd -i payload.bin > payload_bin.c ; x86_64-w64-mingw32-gcc -g patcher.c payload_bin.c -static-libgcc -static-libstdc++ -static`

depatcher

`gcc -g depatcher.c payload_bin.c -o b.out`

`x86_64-w64-mingw32-gcc -g depatcher.c payload_bin.c -static-libgcc -static-libstdc++ -static -o b.exe`

## Credits

Thanks to
- [gba-auto-batteryless-patcher](https://github.com/metroid-maniac/gba-auto-batteryless-patcher) 