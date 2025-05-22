#include "trainerCard.h"

static const struct TrainerCard g_templateTrainerCard = 
{
    .rse = {
        .gender = 0,
        .stars = 4,
        .hasPokedex = true,
        .caughtAllHoenn = true,
        .hasAllPaintings = true,
        .hofDebutHours = 999,
        .hofDebutMinutes = 59,
        .hofDebutSeconds = 59,
        .caughtMonsCount = 200,
        .trainerId =  0x529E,
        .playTimeHours = 999,
        .playTimeMinutes = 59,
        .linkBattleWins = 5535,
        .linkBattleLosses = 5535,
        .battleTowerWins = 5535,
        .battleTowerStraightWins = 5535,
        .contestsWithFriends = 55555,
        .pokeblocksWithFriends = 44444,
        .pokemonTrades = 33333,
        .money = 999999,
        .easyChatProfile = {0, 0, 0, 0},
        .playerName = {0xE3, 0xC4, 0xE0, 0xD9, 0xFF, 0x00, 0x00, 0x00}
    },
    .version = 4,
    .hasAllFrontierSymbols = false,
    .berryCrushPoints = 5555,
    .unionRoomNum = 8500,
    .berriesPicked = 5456,
    .jumpsInRow = 6300,
    .shouldDrawStickers = true,
    .hasAllMons = true,
    .monIconTint = 1,
    .facilityClass = 0,
    .stickers = {1, 2, 3},
    .monSpecies = {1, 2, 3, 4, 5, 6}
};

const struct TrainerCard* trainerCardPlaceholder()
{
    return &g_templateTrainerCard;
}