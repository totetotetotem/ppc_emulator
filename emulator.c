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

void load_immidiate(Emulator* emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x10;
    uint8_t dst = (code >> 16) & 0x10;
    uint16_t immidiate = code & 0xffff;

    printf("li r%d, %d\n", src, immidiate);
    emu->registers[src] = immidiate;
    emu->pc += 4;
}

void load_immidiate_shifted(Emulator* emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x1f;
    uint8_t dst = (code >> 16) & 0x1f;
    int16_t immidiate = code & 0xffff;

    printf("lis r%d, %d\n", src, immidiate);
    emu->registers[src] = (immidiate << 16);
    emu->pc += 4;
}

void load_word_and_zero(Emulator* emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t dst = (code >> 21) & 0x1f;
    uint8_t src = (code >> 16) & 0x1f;
    uint16_t immidiate = code & 0xffff;

    printf("lwz r%d, %d(r%d)\n", dst, immidiate, src);
    uint32_t address = emu->registers[src] & 0x0fffffff;
    emu->registers[dst] = emu->memory[address + immidiate];
    emu->pc += 4;
}

void add_immidiate(Emulator *emu) {
    uint32_t code = get_code(emu,0);
    uint8_t opcode = (code >> 24) & 0xfc;
    uint8_t src = (code >> 21) & 0x1f;
    uint8_t dst = (code >> 16) & 0x1f;
    uint16_t immidiate = code & 0xffff;

    emu->registers[src] = emu->registers[dst] + immidiate;
    printf("addi r%d, r%d, %d\n", src, dst, immidiate);
    emu->pc += 4;
}

typedef void instruction_func_t(Emulator*);
instruction_func_t* instructions[256];
void init_instructions(void) {
    instructions[0x3c] = load_immidiate_shifted;
    instructions[0x38] = add_immidiate;
    instructions[0x80] = load_word_and_zero;
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

    init_instructions();

    while(emu->pc < MEMORY_SIZE) {
        uint32_t code = get_code(emu,0);
        if(code != 0)
            printf("0x%08x \n", code);

        uint8_t opcode = (code >> 24) & 0xfc;

        if(instructions[opcode] == NULL) {
            printf("%02x is not registered opcode\n", opcode);
            break;
        }

        if(opcode == 0x00) {
            printf("\nend of program\n\n");
            break;
        }
        instructions[opcode](emu);
    }


    destroy_emulator(emu);
    return 0;
}

