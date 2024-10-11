#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "payload_bin.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

FILE *romfile;
FILE *outfile;
uint32_t romsize;
uint8_t rom[0x02000000];
char signature[] = "<3 from ChisBreadRumble";

int pause_exit(int argc, char **argv)
{
    if (argc <= 2)
    {
		scanf("%*s");
        return 1;
    }
    return 1;
}

static inline char* get_rom_identifier(char* rombuffer, int romsize) {
    char* game_title_raw = (char*)malloc(18);
    for (int i = 0xA0; i < 0xAC; i++) {
        game_title_raw[i - 0xA0] = rombuffer[i];
        if (game_title_raw[i - 0xA0] == '\x00' || game_title_raw[i - 0xA0] == '\n' || game_title_raw[i - 0xA0] == ' ') {
            game_title_raw[i - 0xA0] = '_';
        }
    }
    game_title_raw[12] = '_';
    for (int i = 0xAC; i < 0xB0; i++) {
        game_title_raw[13 + i - 0xAC] = rombuffer[i];
        if (game_title_raw[i - 0xA0] == '\x00' || game_title_raw[i - 0xA0] == '\n' || game_title_raw[i - 0xA0] == ' ') {
            game_title_raw[i - 0xA0] = '_';
        }
    }

    // 去掉首尾的_
    while (game_title_raw[0] == '_') {
        for (int i = 0; i < 18; i++) {
            game_title_raw[i] = game_title_raw[i + 1];
        }
    }
    // 去掉连续的_，替换为单个_
    for (int i = 0; i < 18; i++) {
        if (game_title_raw[i] == '_' && game_title_raw[i + 1] == '_') {
            for (int j = i; j < 18; j++) {
                game_title_raw[j] = game_title_raw[j + 1];
            }
        }
    }
    return game_title_raw;
}

//BL xx 跳转指令的机器码，
//address：执行这个 BL xx 指令时候，处于哪个地址
//entry：跳转的目标地址，就是xx
static unsigned int NE_MakeBLmachineCode2(unsigned int address, unsigned int entry)
{ 
	unsigned int offset, imm32, low, high;
	offset = ( entry - address - 4 ) & 0x007fffff;
 
	high = 0xF000 | offset >> 12;
	low = 0xF800 | (offset & 0x00000fff) >> 1;
 
	imm32 = (low << 16) | high;
 
	return imm32;
}

static uint8_t *memfind(uint8_t *haystack, size_t haystack_size, uint8_t *needle, size_t needle_size, int stride)
{
    for (size_t i = 0; i < haystack_size - needle_size; i += stride)
    {
        if (!memcmp(haystack + i, needle, needle_size))
        {
            return haystack + i;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("Wrong number of args. Try dragging and dropping your ROM onto the .exe file in the file browser.");
        return pause_exit(argc, argv);
    }
	
	memset(rom, 0x00ff, sizeof rom);
    
    size_t romfilename_len = strlen(argv[1]);
    if (romfilename_len < 4 || strcasecmp(argv[1] + romfilename_len - 4, ".gba"))
    {
        puts("File does not have .gba extension.");
        return pause_exit(argc, argv);
    }

    // Open ROM file
    if (!(romfile = fopen(argv[1], "rb")))
    {
        puts("Could not open input file");
        puts(strerror(errno));
        return pause_exit(argc, argv);
    }

    // Load ROM into memory
    fseek(romfile, 0, SEEK_END);
    romsize = ftell(romfile);

    if (romsize > sizeof rom)
    {
        puts("ROM too large - not a GBA ROM?");
        return pause_exit(argc, argv);
    }

    if (romsize & 0x3ffff)
    {
		puts("ROM has been trimmed and is misaligned. Padding to 256KB alignment");
		romsize &= ~0x3ffff;
		romsize += 0x40000;
    }

    fseek(romfile, 0, SEEK_SET);
    fread(rom, 1, romsize, romfile);

    char *game_title = get_rom_identifier(rom, romsize);
    printf("Game title: %s\n", game_title);
    free(game_title);

    // Check if ROM already patched.
    if (memfind(rom, romsize, signature, sizeof signature - 1, 4))
    {
        puts("Signature found. ROM already patched!");
        return pause_exit(argc, argv);
    }
    
    // puts("Is the payload THUMB? (y/n)");
    int is_thumb = 1;
    // char thumb[32];
    // scanf("%s", thumb);
    // if (thumb[0] != 'n')
    // {
    //     is_thumb = 1;
    // }
    
    puts("Enter the address of the ROM you want to patch(e.g. 0x08000000):");
    puts("Enter 'q' to finish:");
    uint32_t rom_addr_to_patch[1024] = {0}; // 0存储地址个数
    int i = 0;
    while (1)
    {
        char addr[32];
        scanf("%s", addr);
        if (addr[0] == 'q')
        {
            break;
        }
        for (int j = 0; j < 32; j++)
        {
            if (!(addr[j] >= '0' && addr[j] <= '9') && !(addr[j] >= 'a' && addr[j] <= 'f') && !(addr[j] >= 'A' && addr[j] <= 'F') && addr[j] != 'x')
            {
                addr[j] = '\0';
            }
        }
        if (addr[0] == '\0')
        {
            continue;
        }
        rom_addr_to_patch[i + 1] = strtol(addr, NULL, 16);
        if (rom_addr_to_patch[i + 1] >= 0x08000000) {
            rom_addr_to_patch[i + 1] -= 0x08000000;
        }
        i++;
    }
    if (i == 0)
    {
        puts("No address to patch");
        return pause_exit(argc, argv);
    }
    rom_addr_to_patch[0] = i;
    // init payload_bin
    for (int i = 0; i < rom_addr_to_patch[0]; i++)
    {
        uint32_t patch_addr = rom_addr_to_patch[i + 1];
        printf("--------------- Patching thumb address %08x ---------------\n", patch_addr);
        // Find a location to insert the payload immediately before a 0x1000 byte sector
        uint32_t payload_base_p = -1, payload_base = -1;
        // 在patch_addr左右两侧，找一片空间，大小为payload_bin_len+0x16
        // 空间内的数据都是0xff或者0x00
        // 范围是正负0x400000
        for (payload_base_p = patch_addr>0x400000?patch_addr-0x400000:0;
                        payload_base_p < patch_addr+0x400000-(payload_bin_len+0x16) 
                        && payload_base_p >= 0 && payload_base_p < romsize; payload_base_p+=4) 
        {
            int is_all_zeroes = 1, is_all_ones = 1;
            for (int k = 0; k < payload_bin_len+0x16 && payload_base_p + k >= 0 && payload_base_p + k < romsize; k++)
            {
                if (rom[payload_base_p + k] != 0)
                {
                    is_all_zeroes = 0;
                }
                if (rom[payload_base_p + k] != 0xff)
                {
                    is_all_ones = 0;
                }
            }
            if (is_all_zeroes || is_all_ones)
            {
                payload_base = payload_base_p + 0x8;
                break;
            }
        }
        if (payload_base == -1)
        {
            // 没有找到空间，尝试从尾部扩容
            // 先计算尾部空间长度
            uint32_t tail_space = 0;
            int is_all_zeroes = 1, is_all_ones = 1;
            for (int j = romsize - 1; j >= 0; j--)
            {
                if (rom[j] != 0)
                {
                    is_all_zeroes = 0;
                }
                if (rom[j] != 0xff)
                {
                    is_all_ones = 0;
                }
                if (!is_all_zeroes && !is_all_ones)
                {
                    break;
                }
                tail_space++;
            }
            printf("Tail space: %x\n", tail_space);
            if (romsize + (payload_bin_len+0x16 - tail_space) > 0x2000000) {
                puts("No space found to insert payload");
                return pause_exit(argc, argv);
            }
            // 扩容
            romsize += payload_bin_len+0x16 - tail_space;
            payload_base = romsize - (payload_bin_len+0x8);
            printf("Expanding ROM to %x bytes\n", romsize);
        }
        printf("BL jump offset: %08x\n", payload_base > patch_addr ? payload_base - patch_addr : patch_addr - payload_base);
        // patch rom
        uint32_t machineCode;
        if (!is_thumb) {
            /*
            当前地址0000D11C，目的地址00021888，偏移值=（00021888-（0000D11C+8））/4=0x0051D9
            */
            uint32_t offset = (payload_base - patch_addr - 8) / 4;
            machineCode = 0xEB000000 | offset;
        }
        else 
        {
            machineCode = NE_MakeBLmachineCode2(patch_addr+0x08000000, payload_base+0x08000000);
        }
        printf("Save original machine code %02x%02x%02x%02x\n", rom[patch_addr], rom[patch_addr + 1], rom[patch_addr + 2], rom[patch_addr + 3]);
        memcpy(payload_bin, rom + patch_addr, 4);
        printf("Patching thumb address %08x to jump to %08x with machine code %08x\n", patch_addr, payload_base, machineCode);
        rom[patch_addr] = machineCode & 0xFF;
        rom[patch_addr + 1] = (machineCode >> 8) & 0xFF;
        rom[patch_addr + 2] = (machineCode >> 16) & 0xFF;
        rom[patch_addr + 3] = (machineCode >> 24) & 0xFF;
        printf("Installing payload at offset %08x\n", payload_base);
        memcpy(rom + payload_base, payload_bin, payload_bin_len);
        puts("--------------- Payload installed ---------------\n");
    }
    

	// Patch the ROM entrypoint to init sram and the dummy IRQ handler, and tell the new entrypoint where the old one was.
	if (rom[3] != 0xea)
	{
		puts("Unexpected entrypoint instruction");
		scanf("%*s");
		return 1;
	}
	

	// Flush all changes to new file
    char *suffix = "_rumble.gba";
    size_t suffix_length = strlen(suffix);
    char new_filename[FILENAME_MAX];
    strncpy(new_filename, argv[1], FILENAME_MAX);
    strncpy(new_filename + romfilename_len - 4, suffix, strlen(suffix));
    
    if (!(outfile = fopen(new_filename, "wb")))
    {
        puts("Could not open output file");
        puts(strerror(errno));
        return pause_exit(argc, argv);
    }
    
    fwrite(rom, 1, romsize, outfile);
    fflush(outfile);

    printf("Patched successfully. Changes written to %s\n", new_filename);
    pause_exit(argc, argv);
	return 0;
	
}
