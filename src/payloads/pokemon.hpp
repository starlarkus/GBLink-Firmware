#include <zephyr/kernel.h>
#include <cstdint>
#include <span>
#include <cstring>

#pragma once

#define POKEMON_NAME_LENGTH 10
#define PLAYER_NAME_LENGTH 7

namespace party
{
    struct PokemonSubstruct0
    {
        uint16_t species;
        uint16_t heldItem;
        uint32_t experience;
        uint8_t ppBonuses;
        uint8_t friendship;
        uint16_t filler;
    };

    struct PokemonSubstruct1
    {
        uint16_t moves[4];
        uint8_t pp[4];
    };

    struct PokemonSubstruct2
    {
        uint8_t hpEV;
        uint8_t attackEV;
        uint8_t defenseEV;
        uint8_t speedEV;
        uint8_t spAttackEV;
        uint8_t spDefenseEV;
        uint8_t cool;
        uint8_t beauty;
        uint8_t cute;
        uint8_t smart;
        uint8_t tough;
        uint8_t sheen;
    };

    struct PokemonSubstruct3
    {
    /* 0x00 */ uint8_t pokerus;
    /* 0x01 */ uint8_t metLocation;

    /* 0x02 */ uint16_t metLevel:7;
    /* 0x02 */ uint16_t metGame:4;
    /* 0x03 */ uint16_t pokeball:4;
    /* 0x03 */ uint16_t otGender:1;

    /* 0x04 */ uint32_t hpIV:5;
    /* 0x04 */ uint32_t attackIV:5;
    /* 0x05 */ uint32_t defenseIV:5;
    /* 0x05 */ uint32_t speedIV:5;
    /* 0x05 */ uint32_t spAttackIV:5;
    /* 0x06 */ uint32_t spDefenseIV:5;
    /* 0x07 */ uint32_t isEgg:1;
    /* 0x07 */ uint32_t abilityNum:1;

    /* 0x08 */ uint32_t coolRibbon:3;               // Stores the highest contest rank achieved in the Cool category.
    /* 0x08 */ uint32_t beautyRibbon:3;             // Stores the highest contest rank achieved in the Beauty category.
    /* 0x08 */ uint32_t cuteRibbon:3;               // Stores the highest contest rank achieved in the Cute category.
    /* 0x09 */ uint32_t smartRibbon:3;              // Stores the highest contest rank achieved in the Smart category.
    /* 0x09 */ uint32_t toughRibbon:3;              // Stores the highest contest rank achieved in the Tough category.
    /* 0x09 */ uint32_t championRibbon:1;           // Given when defeating the Champion. Because both RSE and FRLG use it, later generations don't specify from which region it comes from.
    /* 0x0A */ uint32_t winningRibbon:1;            // Given at the Battle Tower's Level 50 challenge by winning a set of seven battles that extends the current streak to 56 or more.
    /* 0x0A */ uint32_t victoryRibbon:1;            // Given at the Battle Tower's Level 100 challenge by winning a set of seven battles that extends the current streak to 56 or more.
    /* 0x0A */ uint32_t artistRibbon:1;             // Given at the Contest Hall by winning a Master Rank contest with at least 800 points, and agreeing to have the Pokémon's portrait placed in the museum after being offered.
    /* 0x0A */ uint32_t effortRibbon:1;             // Given at Slateport's market to Pokémon with maximum EVs.
    /* 0x0A */ uint32_t marineRibbon:1;             // Never distributed.
    /* 0x0A */ uint32_t landRibbon:1;               // Never distributed.
    /* 0x0A */ uint32_t skyRibbon:1;                // Never distributed.
    /* 0x0A */ uint32_t countryRibbon:1;            // Distributed during Pokémon Festa '04 and '05 to tournament winners.
    /* 0x0B */ uint32_t nationalRibbon:1;           // Given to purified Shadow Pokémon in Colosseum/XD.
    /* 0x0B */ uint32_t earthRibbon:1;              // Given to teams that have beaten Mt. Battle's 100-battle challenge in Colosseum/XD.
    /* 0x0B */ uint32_t worldRibbon:1;              // Distributed during Pokémon Festa '04 and '05 to tournament winners.
    /* 0x0B */ uint32_t unusedRibbons:4;            // Discarded in Gen 4.

    // The functionality of this bit changed in FRLG:
    // In RS, this bit does nothing, is never set, & is accidentally unset when hatching Eggs.
    // In FRLG & Emerald, this controls Mew & Deoxys obedience and whether they can be traded.
    // If set, a Pokémon is a fateful encounter in FRLG's summary screen if hatched & for all Pokémon in Gen 4+ summary screens.
    // Set for in-game event island legendaries, events distributed after a certain date, & Pokémon from XD: Gale of Darkness.
    // Not to be confused with METLOC_FATEFUL_ENCOUNTER.
    /* 0x0B */ uint32_t modernFatefulEncounter:1;
    };

    struct PokemonSubstruct
    {
        struct PokemonSubstruct0 type0;
        struct PokemonSubstruct1 type1;
        struct PokemonSubstruct2 type2;
        struct PokemonSubstruct3 type3;
    };

    struct BoxPokemon
    {
        uint32_t personality;
        uint32_t otId;
        uint8_t nickname[POKEMON_NAME_LENGTH];
        uint8_t language;
        uint8_t isBadEgg:1;
        uint8_t hasSpecies:1;
        uint8_t isEgg:1;
        uint8_t blockBoxRS:1; // Unused, but Pokémon Box Ruby & Sapphire will refuse to deposit a Pokémon with this flag set
        uint8_t unused:4;
        uint8_t otName[PLAYER_NAME_LENGTH];
        uint8_t markings;
        uint16_t checksum;
        uint16_t unknown;

        PokemonSubstruct secure;
    };

    struct Pokemon
    {
        struct BoxPokemon box;
        uint32_t status;
        uint8_t level;
        uint8_t mail;
        uint16_t hp;
        uint16_t maxHP;
        uint16_t attack;
        uint16_t defense;
        uint16_t speed;
        uint16_t spAttack;
        uint16_t spDefense;
    };

    int partyInit();
    
    void partnerPartyInit();

    void partnerPartyConstruct(std::span<const uint8_t> data);

    void clearPartySlot(uint8_t index);

    void usbReceivePkmFile(std::span<const uint8_t> data, void*);

    std::span<const uint8_t> getParty();

    int partySize();

    void tradePkmnAtIndex(uint8_t index);

}

