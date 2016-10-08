#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define COND 32
#define LINK 33
#define COUNT 34
#define INT_EXCEPTION 35
#define SP 1

#define MEMORY_SIZE 0x40000

typedef struct{
    uint32_t registers[36];
    uint32_t pc;
    uint8_t *memory;
} Emulator;

Emulator* create_emulator(size_t size, uint32_t pc, uint32_t sp) {
    Emulator* emu = malloc(sizeof(Emulator));
    emu->memory = malloc(size);

    memset(emu->registers, 0, sizeof(emu->registers));

    emu->pc = pc;
    emu->registers[SP] = sp;

    return emu;
}


void destroy_emulator(Emulator* emu) {
    free(emu->memory);
    free(emu);
}

uint32_t get_code(Emulator* emu, int index) {
    uint32_t ret = 0;
    for(int i = 0; i < 4; i++) {
        ret += emu->memory[emu->pc+index+i] << ((3-i)*8);
    }
    return ret;
}

typedef void instruction_func_t(Emulator*);
instruction_func_t* instructions[256];
void init_instructions(void) {
}

int main (int argc, char* argv[]) {
    FILE *binary;
    Emulator *emu;

    if(argc != 2) {
        printf("usage: ./ppc_emu filename\n");
        return 1;
    }

    emu = create_emulator(MEMORY_SIZE, 0xb0, 0x500); 

    printf("argv[1] = %s\n", argv[1]);
    binary = fopen(argv[1], "rb");

    if(binary == NULL) {
        printf("cannot open file.\n");
        return 1;
    }

    fread(emu->memory, 0x200, 1, binary);

    fclose(binary);
    while(emu->pc < MEMORY_SIZE) {
        uint32_t code = get_code(emu,0);
        if(code != 0)
            printf("0x%08x \n", code);

        uint8_t opcode = (code >> 24) & 0xfc;
        if(instructions[opcode] == NULL) {
            printf("%02x is not registered opcode\n", opcode);
            return 1;
        }
        emu->pc += 4;
    }


    destroy_emulator(emu);
    return 0;
}

