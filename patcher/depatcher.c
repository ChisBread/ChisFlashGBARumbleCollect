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
//BL xx 跳转指令的机器码，
//address：执行这个 BL xx 指令时候，处于哪个地址
//entry：跳转的目标地址，就是xx
unsigned int NE_MakeBLmachineCode2(unsigned int address, unsigned int entry)
{ 
	unsigned int offset, imm32, low, high;
	offset = ( entry - address - 4 ) & 0x007fffff;
 
	high = 0xF000 | offset >> 12;
	low = 0xF800 | (offset & 0x00000fff) >> 1;
 
	imm32 = (low << 16) | high;
 
	return imm32;
}

static uint8_t *memfind(uint8_t *haystack, int haystack_size, uint8_t *needle, int needle_size, int stride)
{
    for (int i = 0; i < haystack_size - needle_size; i += stride)
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
    
    int romfilename_len = strlen(argv[1]);
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

    // Check if ROM already patched.
    if (!memfind(rom, romsize, signature, sizeof signature - 1, 4))
    {
        puts("Signature found. ROM is not patched!");
        return pause_exit(argc, argv);
    }
    // 找到payload_bin的地址,用payload_bin+4去匹配
    unsigned int payload_base_list[0x100];
    int payload_base_list_index = 0;
    for (int i = 4; i < romsize - 4; i += 4)
    {
        if (!memcmp(rom + i, payload_bin + 4, payload_bin_len - 4))
        {
            payload_base_list[payload_base_list_index++] = i - 4; // payload_base
        }
    }
    printf("Found %d payload base\n", payload_base_list_index);
    printf("ROM patched at\n");
    char *suffix = "_rumble_entrys.txt";
    size_t suffix_length = strlen(suffix);
    char new_filename[FILENAME_MAX];
    strncpy(new_filename, argv[1], FILENAME_MAX);
    strncpy(new_filename + romfilename_len - 4, suffix, strlen(suffix));
    FILE *fp = fopen(new_filename, "w");
    for (int i = 2; i < romsize - 2; i += 2) {
        // 找到BL指令
        unsigned int mechine_code = *(unsigned int *)(rom + i);
        for (int j = 0; j < payload_base_list_index; j++) {
            if (NE_MakeBLmachineCode2(i, payload_base_list[j]) == mechine_code) {
                printf("0x08%06X\n", i);
                //write to file
                fprintf(fp, "0x08%06X\n", i);
                break;
            }
        }
    }
    printf("q\n");
    fprintf(fp, "q\n");
    fclose(fp);
	return 0;
	
}
