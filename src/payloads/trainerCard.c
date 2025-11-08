#include "trainerCard.h"

static const struct TrainerCard g_templateTrainerCard = 
{
    .rse = {
        .gender = 0,
        .stars = 4,
        .hasPokedex = true,
        .caughtAllHoenn = true,
        .hasAllPaintings = true,
        .hofDebutHours = 10,
        .hofDebutMinutes = 15,
        .hofDebutSeconds = 23,
        .caughtMonsCount = 386,
        .trainerId =  0x529E,
        .playTimeHours = 95,
        .playTimeMinutes = 10,
        .linkBattleWins = 30,
        .linkBattleLosses = 0,
        .battleTowerWins = 120,
        .battleTowerStraightWins = 100,
        .contestsWithFriends = 0,
        .pokeblocksWithFriends = 0,
        .pokemonTrades = 386,
        .money = 103345,
        .easyChatProfile = {((4 << 9) | 0xf), ((8 << 9) | 0x23), 0xFFFF, 0xFFFF},
        .playerName = {0xC8, 0xDD, 0xE0, 0xE7, 0xFF, 0x00, 0x00, 0x00}
    },
    .version = 4,
    .hasAllFrontierSymbols = false,
    .berryCrushPoints = 10,
    .unionRoomNum = 8500,
    .berriesPicked = 60,
    .jumpsInRow = 100,
    .shouldDrawStickers = true,
    .hasAllMons = true,
    .monIconTint = 1,
    .facilityClass = 0,
    .stickers = {1, 2, 3},
    .monSpecies = {94, 6, 82, 212, 406, 303}
};

const struct TrainerCard* trainerCardPlaceholder()
{
    return &g_templateTrainerCard;
}