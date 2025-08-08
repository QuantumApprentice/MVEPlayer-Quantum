#pragma once

struct opcodeinfo {
    uint16_t size;
    uint8_t  type;
    uint8_t version;
};

void parse_opcode(opcodeinfo op, uint8_t* buffer);
