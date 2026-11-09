#define _GNU_SOURCE
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

extern int enter_ia32(void *stack, void *argument, uintptr_t entry);
extern void return_ia32(void);

static void *map(uintptr_t address, size_t length, int protection)
{
    void *memory = mmap((void *)address, length, protection,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (memory == MAP_FAILED) {
        perror("mmap x86 fixture");
        exit(96);
    }
    return memory;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 97;
    FILE *elf = fopen(argv[1], "rb");
    if (!elf)
        return 98;
    Elf32_Ehdr header;
    if (fread(&header, sizeof(header), 1, elf) != 1 || header.e_machine != EM_386 ||
        header.e_phentsize != sizeof(Elf32_Phdr))
        return 99;
    // The ELF harness is static and linked at the conventional i386 address.
    map(0x08000000u, 0x01000000u, PROT_READ | PROT_WRITE | PROT_EXEC);
    for (unsigned int index = 0; index < header.e_phnum; ++index) {
        Elf32_Phdr segment;
        if (fseek(elf, header.e_phoff + index * sizeof(segment), SEEK_SET) ||
            fread(&segment, sizeof(segment), 1, elf) != 1)
            return 100;
        if (segment.p_type == PT_LOAD && segment.p_filesz &&
            (fseek(elf, segment.p_offset, SEEK_SET) ||
             fread((void *)(uintptr_t)segment.p_vaddr, 1, segment.p_filesz, elf) != segment.p_filesz))
            return 101;
    }
    fclose(elf);
    map(0x00400000u, 0x00500000u, PROT_READ | PROT_WRITE | PROT_EXEC);
    map(0x12000000u, 0x00100000u, PROT_READ | PROT_WRITE | PROT_EXEC);
    unsigned char *shared = map(0x30000000u, 0x10000u, PROT_READ | PROT_WRITE);
    uint32_t gate = (uintptr_t)return_ia32;
    memcpy(shared, &gate, sizeof(gate));
    shared[4] = 0x33;
    shared[32] = argv[2][0];
    int result = enter_ia32(shared + 0xfff0, shared + 32, header.e_entry);
    if (!result && shared[32] == '0')
        fwrite(shared + 8, 1, 24, stdout);
    return result;
}
