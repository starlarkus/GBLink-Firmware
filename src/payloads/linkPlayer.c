#include "linkPlayer.h"

static struct LinkPlayerBlock g_linkPlayer = 
{
    .magic1 = "GameFreak inc.",
    .linkPlayer = 
    {
        .version = 0x4004,
        .lp_field_2 = 0x8000,
        .trainerId = 0x529E,
        .secretId = 0x1805,
        .name = {0xC8, 0xDD, 0xE0, 0xE7, 0xFF, 0x00, 0x00, 0x00},
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

const struct LinkPlayerBlock* linkPLayer(uint32_t linkType)
{
    g_linkPlayer.linkPlayer.linkType = linkType;
    return &g_linkPlayer;
}
