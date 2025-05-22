#include <zephyr/kernel.h>

struct TrainerCardRSE
{
    /*0x00*/ uint8_t gender;
    /*0x01*/ uint8_t stars;
    /*0x02*/ bool hasPokedex;
    /*0x03*/ bool caughtAllHoenn;
    /*0x04*/ bool hasAllPaintings;
    /*0x06*/ uint16_t hofDebutHours;
    /*0x08*/ uint16_t hofDebutMinutes;
    /*0x0A*/ uint16_t hofDebutSeconds;
    /*0x0C*/ uint16_t caughtMonsCount;
    /*0x0E*/ uint16_t trainerId;
    /*0x10*/ uint16_t playTimeHours;
    /*0x12*/ uint16_t playTimeMinutes;
    /*0x14*/ uint16_t linkBattleWins;
    /*0x16*/ uint16_t linkBattleLosses;
    /*0x18*/ uint16_t battleTowerWins;
    /*0x1A*/ uint16_t battleTowerStraightWins;
    /*0x1C*/ uint16_t contestsWithFriends;
    /*0x1E*/ uint16_t pokeblocksWithFriends;
    /*0x20*/ uint16_t pokemonTrades;
    /*0x24*/ uint32_t money;
    /*0x28*/ uint16_t easyChatProfile[4];
    /*0x30*/ uint8_t playerName[8];
};

struct TrainerCard
{
    struct TrainerCardRSE rse;
    /*0x38*/ uint8_t version;
    /*0x3A*/ uint16_t hasAllFrontierSymbols;
    /*0x3C*/ uint32_t berryCrushPoints;
    /*0x40*/ uint32_t unionRoomNum;
    /*0x44*/ uint32_t berriesPicked;
    /*0x48*/ uint32_t jumpsInRow;
    /*0x4C*/ bool shouldDrawStickers;
    /*0x4D*/ bool hasAllMons;
    /*0x4E*/ uint8_t monIconTint;
    /*0x4F*/ uint8_t facilityClass;
    /*0x50*/ uint8_t stickers[3];
    /*0x54*/ uint16_t monSpecies[6];
};

const struct TrainerCard* trainerCardPlaceholder();