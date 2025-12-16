#pragma once

struct opcodeinfo {
    uint16_t size;
    uint8_t  type;
    uint8_t version;
};

struct chunkinfo {
    uint16_t size;
    uint16_t type;
};


void parse_opcode(opcodeinfo op, uint8_t* buffer);
void parse_chunk_ops(uint8_t* chunk, chunkinfo info);
