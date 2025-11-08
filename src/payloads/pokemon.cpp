#include "pokemon.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace detail
{
    template<typename T>
    T swap(std::span<const uint8_t> data)
    {
        T val = 0;
        auto val_bytes = std::as_writable_bytes(std::span{&val, 1});
        for (size_t i = 0; i < sizeof(T); i++)
        {
            val_bytes[i] = (std::byte)data[i];
        }
        return val;
    }

    template<typename T>
    size_t read(T& field, std::span<const uint8_t> data) {
        using fieldT = std::remove_reference_t<decltype(field)>;
        field = swap<fieldT>(data);
        return sizeof(fieldT);
    };

    party::Pokemon deserializePokemon(std::span<const uint8_t> input) {
        party::Pokemon result{};
        size_t offset = 0;
        
        // BoxPokemon fields
        offset += read(result.box.personality, input.subspan(offset));
        offset += read(result.box.otId, input.subspan(offset));

        std::memcpy(result.box.nickname, &input[offset], POKEMON_NAME_LENGTH);
        offset += POKEMON_NAME_LENGTH;

        offset += read(result.box.language, input.subspan(offset));

        uint8_t flags;
        offset += read(flags, input.subspan(offset));
        result.box.isBadEgg   = (flags >> 0) & 1;
        result.box.hasSpecies = (flags >> 1) & 1;
        result.box.isEgg      = (flags >> 2) & 1;
        result.box.blockBoxRS = (flags >> 3) & 1;
        result.box.unused     = (flags >> 4) & 0xF;

        std::memcpy(result.box.otName, &input[offset], PLAYER_NAME_LENGTH);
        offset += PLAYER_NAME_LENGTH;

        offset += read(result.box.markings, input.subspan(offset));
        offset += read(result.box.checksum, input.subspan(offset));

        offset += read(result.box.unknown, input.subspan(offset));

        // PokemonSubstruct0
        offset += read(result.box.secure.type0.species, input.subspan(offset));
        offset += read(result.box.secure.type0.heldItem, input.subspan(offset));
        offset += read(result.box.secure.type0.experience, input.subspan(offset));
        offset += read(result.box.secure.type0.ppBonuses, input.subspan(offset));
        offset += read(result.box.secure.type0.friendship, input.subspan(offset));
        offset += read(result.box.secure.type0.filler, input.subspan(offset));

        // PokemonSubstruct1
        offset += read(result.box.secure.type1.moves[0], input.subspan(offset));
        offset += read(result.box.secure.type1.moves[1], input.subspan(offset));
        offset += read(result.box.secure.type1.moves[2], input.subspan(offset));
        offset += read(result.box.secure.type1.moves[3], input.subspan(offset));
        offset += read(result.box.secure.type1.pp[0], input.subspan(offset));
        offset += read(result.box.secure.type1.pp[1], input.subspan(offset));
        offset += read(result.box.secure.type1.pp[2], input.subspan(offset));
        offset += read(result.box.secure.type1.pp[3], input.subspan(offset));

        // PokemonSubstruct2
        offset += read(result.box.secure.type2.hpEV, input.subspan(offset));
        offset += read(result.box.secure.type2.attackEV, input.subspan(offset));
        offset += read(result.box.secure.type2.defenseEV, input.subspan(offset));
        offset += read(result.box.secure.type2.speedEV, input.subspan(offset));
        offset += read(result.box.secure.type2.spAttackEV, input.subspan(offset));
        offset += read(result.box.secure.type2.spDefenseEV, input.subspan(offset));
        offset += read(result.box.secure.type2.cool, input.subspan(offset));
        offset += read(result.box.secure.type2.beauty, input.subspan(offset));
        offset += read(result.box.secure.type2.cute, input.subspan(offset));
        offset += read(result.box.secure.type2.smart, input.subspan(offset));
        offset += read(result.box.secure.type2.tough, input.subspan(offset));
        offset += read(result.box.secure.type2.sheen, input.subspan(offset));

        // PokemonSubstruct3
        offset += read(result.box.secure.type3.pokerus, input.subspan(offset));
        offset += read(result.box.secure.type3.metLocation, input.subspan(offset));

        uint16_t var16 = 0;
        offset += read(var16, input.subspan(offset));
        result.box.secure.type3.metLevel = (var16 >> 0) & 0x7F;
        result.box.secure.type3.metGame  = (var16 >> 7) & 0x0F;
        result.box.secure.type3.pokeball = (var16 >> 11) & 0x0F;
        result.box.secure.type3.otGender = (var16 >> 15) & 0x01;

        uint32_t var32 = 0;
        offset += read(var32, input.subspan(offset));
        result.box.secure.type3.hpIV        = (var32 >>  0) & 0x1F;
        result.box.secure.type3.attackIV    = (var32 >>  5) & 0x1F;
        result.box.secure.type3.defenseIV   = (var32 >> 10) & 0x1F;
        result.box.secure.type3.speedIV     = (var32 >> 15) & 0x1F;
        result.box.secure.type3.spAttackIV  = (var32 >> 20) & 0x1F;
        result.box.secure.type3.spDefenseIV = (var32 >> 25) & 0x1F;
        result.box.secure.type3.isEgg       = (var32 >> 30) & 0x01;
        result.box.secure.type3.abilityNum  = (var32 >> 31) & 0x01;

        offset += read(var32, input.subspan(offset));
        result.box.secure.type3.coolRibbon        = (var32 >> 0) & 0x07;
        result.box.secure.type3.beautyRibbon      = (var32 >> 3) & 0x07;
        result.box.secure.type3.cuteRibbon        = (var32 >> 6) & 0x07;
        result.box.secure.type3.smartRibbon       = (var32 >> 9) & 0x07;
        result.box.secure.type3.toughRibbon       = (var32 >> 12) & 0x07;
        result.box.secure.type3.championRibbon    = (var32 >> 15) & 0x01;
        result.box.secure.type3.winningRibbon     = (var32 >> 16) & 0x01;
        result.box.secure.type3.victoryRibbon     = (var32 >> 17) & 0x01;
        result.box.secure.type3.artistRibbon      = (var32 >> 18) & 0x01;
        result.box.secure.type3.effortRibbon      = (var32 >> 19) & 0x01;
        result.box.secure.type3.marineRibbon      = (var32 >> 20) & 0x01;
        result.box.secure.type3.landRibbon        = (var32 >> 21) & 0x01;
        result.box.secure.type3.skyRibbon         = (var32 >> 22) & 0x01;
        result.box.secure.type3.countryRibbon     = (var32 >> 23) & 0x01;
        result.box.secure.type3.nationalRibbon    = (var32 >> 24) & 0x01;
        result.box.secure.type3.earthRibbon       = (var32 >> 25) & 0x01;
        result.box.secure.type3.worldRibbon       = (var32 >> 26) & 0x01;
        result.box.secure.type3.unusedRibbons     = (var32 >> 27) & 0x0F;
        result.box.secure.type3.modernFatefulEncounter = (var32 >> 31) & 0x01;

        // Remaining visible fields in struct Pokemon
        offset += read(result.status, input.subspan(offset));
        offset += read(result.level, input.subspan(offset));
        offset += read(result.mail, input.subspan(offset));
        offset += read(result.hp, input.subspan(offset));
        offset += read(result.maxHP, input.subspan(offset));
        offset += read(result.attack, input.subspan(offset));
        offset += read(result.defense, input.subspan(offset));
        offset += read(result.speed, input.subspan(offset));
        offset += read(result.spAttack, input.subspan(offset));
        offset += read(result.spDefense, input.subspan(offset));

        return result;
    }
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

namespace party 
{
    const static std::array<uint8_t, 100> g_fillerPkmnArray = 
    {
        0x57, 0xe5, 0x88, 0x2a, 0x39, 0x30, 0x31, 0xd4, 0xc0, 0xc3, 
        0xc6, 0xc6, 0xbf, 0xcc, 0xff, 0x00, 0x00, 0x00, 0x02, 0x02, 
        0xc8, 0xdd, 0xe0, 0xe7, 0xff, 0xff, 0x00, 0x00, 0xfd, 0xd1, 
        0x00, 0x00, 0x7e, 0x14, 0xa0, 0x9c, 0xfc, 0x25, 0x99, 0xc7, 
        0x6e, 0xd5, 0xbd, 0xfe, 0x6e, 0xd5, 0xb9, 0xfe, 0x6e, 0xd5, 
        0xb9, 0xfe, 0x6e, 0xd5, 0xb9, 0xfe, 0x83, 0xd5, 0xb9, 0xfe, 
        0x6e, 0xd5, 0xb9, 0xfe, 0x76, 0xd5, 0xb9, 0xfe, 0xa7, 0xd5, 
        0xfd, 0xfe, 0x67, 0xe8, 0xb9, 0xfe, 0x6d, 0x93, 0xb9, 0xfe, 
        0x00, 0x00, 0x00, 0x00, 0x19, 0xFF, 0x3f, 0x00, 0x3f, 0x00, 
        0x2e, 0x00, 0x24, 0x00, 0x1d, 0x00, 0x2d, 0x00, 0x20, 0x00
    };

    static size_t g_receivedBytes = 0;
    static std::array<uint8_t, 600> g_party = {};

    static size_t g_partnerReceivedBytes = 0;
    static std::array<uint8_t, 600> g_partnerParty = {};

    static constexpr size_t chunkSize = 200;
    static constexpr size_t numberOfSlots = 6;

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    std::span<const uint8_t> getParty() { return g_party; }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    std::span<uint8_t> slot(uint8_t index) 
    { 
        return std::span(g_party).subspan(index * 100, 100); 
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void clearPartySlot(uint8_t index)
    {
        std::ranges::copy(g_fillerPkmnArray, slot(index).begin());
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    bool isPartySlotEmpty(uint8_t index)
    {
        return memcmp(g_fillerPkmnArray.data(), slot(index).data(), 100) == 0;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    int partyInit()
    {
        g_receivedBytes = 0;
        for (size_t i = 0; i < numberOfSlots; i++)
        {
            clearPartySlot(i);
        }
        return 0;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void partnerPartyInit()
    {
        g_partnerReceivedBytes = 0;
        std::fill(g_partnerParty.begin(), g_partnerParty.end(), 0x00);
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void partnerPartyConstruct(std::span<const uint8_t> data)
    {
        size_t nextThreshold = ((g_partnerReceivedBytes / chunkSize) + 1) * chunkSize;

        if (g_partnerReceivedBytes + data.size() > nextThreshold) 
        {
            data = data.first(nextThreshold - g_partnerReceivedBytes);
        }
        std::ranges::copy(data, std::span(g_partnerParty).subspan(g_partnerReceivedBytes).begin());
        g_partnerReceivedBytes += data.size();
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void tradePkmnAtIndex(uint8_t index)
    {
        std::ranges::copy(std::span(g_partnerParty).subspan(index * 100, 100), slot(index).begin());
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void usbReceivePkmFile(std::span<const uint8_t> data, void*)
    {
        static std::array<uint8_t, 0x64> encryptedPkm;
        std::ranges::copy(data, std::span(encryptedPkm).subspan(g_receivedBytes).begin());
        g_receivedBytes += data.size();
        if (g_receivedBytes >= 0x64)
        {
            for (size_t i = 0; i < numberOfSlots; i++)
            {
                if (isPartySlotEmpty(i))
                {
                    std::ranges::copy(encryptedPkm, slot(i).begin());
                    break;
                }
            }
            g_receivedBytes = 0;
        }
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
}