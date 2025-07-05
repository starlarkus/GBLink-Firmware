#include "linkPlayer.hpp"

static struct LinkPlayerBlock g_linkPlayer = 
{
    .magic1 = "GameFreak inc.",
    .linkPlayer = 
    {
        .version = 0x4004,
        .lp_field_2 = 0x8000,
        .trainerId = 0x529E,
        .secretId = 0x1805,
        .name = {0xE3, 0xC4, 0xE0, 0xD9, 0xFF, 0x00, 0x00, 0x00},
        .progressFlags = 0xFF,
        .neverRead = 0x00,
        .progressFlagsCopy = 0x00,
        .gender = 0x00,
        .linkType = 0x1133,
        .id = 0x0000,
        .language = 0x0005
    },
    .magic2 = "GameFreak inc."
};

static struct LinkPlayerBlock g_linkPlayerCorrupt = 
{
    .magic1 = "GameFreak inc.",
    .linkPlayer = 
    {
        .version = 0x4004,
        .lp_field_2 = 0x8000,
        .trainerId = 0x529E,
        .secretId = 0x1805,
        .name = {0xE3, 0xC4, 0xE0, 0xD9, 0xFF, 0x00, 0x00, 0x00},
        .progressFlags = 0xFF,
        .neverRead = 0x00,
        .progressFlagsCopy = 0x00,
        .gender = 0x00,
        .linkType = 0x1133,
        .id = 0x0000,
        .language = 0x0005
    },
    .magic2 = "GameFreak pnc."
};

const struct LinkPlayerBlock* linkPLayer(uint32_t linkType)
{
    g_linkPlayer.linkPlayer.linkType = linkType;
    return &g_linkPlayer;
}

const struct LinkPlayerBlock* corruptedLinkPLayer(uint32_t linkType)
{
    g_linkPlayerCorrupt.linkPlayer.linkType = linkType;
    return &g_linkPlayerCorrupt;
}