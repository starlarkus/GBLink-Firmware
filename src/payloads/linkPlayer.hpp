#include <zephyr/kernel.h>

#pragma once

struct LinkPlayer
{
    /* 0x00 */ uint16_t version;
    /* 0x02 */ uint16_t lp_field_2;
    /* 0x04 */ uint16_t trainerId;
    /* 0x06 */ uint16_t secretId;
    /* 0x08 */ uint8_t name[8];
    /* 0x10 */ uint8_t progressFlags; // (& 0x0F) is hasNationalDex, (& 0xF0) is hasClearedGame
    /* 0x11 */ uint8_t neverRead;
    /* 0x12 */ uint8_t progressFlagsCopy;
    /* 0x13 */ uint8_t gender;
    /* 0x14 */ uint32_t linkType;
    /* 0x18 */ uint16_t id; // battle bank in battles
    /* 0x1A */ uint16_t language;
};

struct LinkPlayerBlock
{
    char magic1[16];
    struct LinkPlayer linkPlayer;
    char magic2[16];
};

const struct LinkPlayerBlock* linkPLayer(uint32_t linkype);

const struct LinkPlayerBlock* corruptedLinkPLayer();