#include <plugin.h>

#include <map>

#include "AnimAssocDefinitions.h"

#include <CAnimManager.h>
#include <CFileLoader.h>
#include <CFileMgr.h>

//! Macro for passing a string var to *scanf_s function.
#define SCANF_S_STR(s) s, std::size(s)

static std::string TransformString(std::string string) {
    std::transform(string.begin(), string.end(), string.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return string;
}

std::map<std::string, AnimDescriptor*> AnimDescriptorNames = {
    { "WALKCYCLE", aStdAnimDescs },
    { "QUAD", aQuadDescs },
    { "BIKE", aBikesDescs },
    { "BIKESTD", aStdBikeAnimDescs},
    { "IDLE", aPlayerIdleAnimDescs },
    { "DOOR", aDoorDescs },
    { "WEAP", aWeaponDescs },
    { "WEAP1", aWeaponDescs1 },
    { "WEAP2", aWeaponDescs2 },
    { "THROW", aWeaponDescs3 },
    { "MELEE", aWeaponDescs4 },
    { "MELEE1", aWeaponDescs5 },
    { "CAR", aCarAnimDescs1 },
    { "CAR1", aLowCarAnimDiscs },
    { "CAR2", aCarAnimDescs2 },
    { "TRUCK", aTruckAnimDescs },
    { "DRIVEBY", aDbzAnimDescs },
    { "MEDIC", aMedicAnimDescs },
    { "BEACH", aBeachAnimDescs },
    { "SUNBATHE", aSunbatheAnimDescs },
    { "RIOT", aRiotAnimDescs },
    { "STRIP", aStripAnimDescs },
    { "GANGS", aGangsAnimDescs },
    { "ATTRACTOR", aAttractorsAnimDescs },
    { "SWIM", aSwimAnimDescs },
    { "FAT_TIRED", aFatTiredAnimDescs },
    { "HANDSIGNAL", aHandSignalAnimDescs },
    { "HANDSIGNAL_LEFT", aHandSignalLAnimDescs },
    { "HAND", aHandAnimDescs },
    { "CARRY", aCarryDescs },
    { "HOUSE", aIntHouseAnimDescs },
    { "OFFICE", aIntOfficeAnimDescs },
    { "SHOP", aIntShopAnimDescs },
    { "STEALTH", aStealthDescs },
};

using namespace plugin;

struct Main
{
    Main()
    {

        //Events::gameProcessEvent += [] {

            // SET POINTER PATCHES

            // CAnimationStyleDescriptor* __cdecl CAnimManager::GetAnimGroupName(int index)
            patch::SetPointer(0x4D3A2A + 1, New_ms_aAnimAssocDefinitions);

            // CAnimationStyleDescriptor *__cdecl CAnimManager::AddAnimAssocDefinition(const char *, const char *, int, unsigned int, void *)
            patch::SetPointer(0x4D3BAB + 2, New_ms_aAnimAssocDefinitions);
            patch::SetPointer(0x4D3BB3 + 2, New_ms_aAnimAssocDefinitions);
            patch::SetPointer(0x4D3BDB + 2, New_ms_aAnimAssocDefinitions);
            patch::SetPointer(0x4D3C70 + 2, New_ms_aAnimAssocDefinitions);

            // int __cdecl CAnimManager::CreateAnimAssocGroups()
            patch::SetPointer(0x4D3CDA + 1, &New_ms_aAnimAssocDefinitions->NumAnims);

            // char *__cdecl CAnimManager::GetAnimBlockName(int index)
            patch::SetPointer(0x4D3A3A + 1, New_ms_aAnimAssocDefinitions->BlockName);

            // signed int __cdecl CAnimManager::GetFirstAssocGroup(char *baseName)
            patch::SetPointer(0x4D39B9 + 1, New_ms_aAnimAssocDefinitions->BlockName);

            // CAnimManager::ReadAnimAssociationDefinitions()
            patch::ReplaceFunction(0x5BC910, ReadAnimAssociationDefinitions);

            // char __thiscall CPlayerPed::ProcessAnimGroups(CPlayerPed *playa)
            patch::ReplaceFunction(0x6098F0, ProcessCustomAnimGroups);
        //};
    }

    static void __fastcall ProcessCustomAnimGroups(CPlayerPed* player, int)
    {
        uint8_t requiredMotionGroup = player->m_nAnimGroup;

        requiredMotionGroup = 54;

        if (player->m_nAnimGroup != requiredMotionGroup)
        {
            player->m_nAnimGroup = requiredMotionGroup;
            player->ReApplyMoveAnims();
        }
    }

    static void __cdecl ReadAnimAssociationDefinitions()
    {
        char                      groupName[32], blockName[32], type[32];
        bool                      isAnimSection = false;
        AnimAssocDefinition*      def;
        //CAnimationStyleDescriptor def;
        uint32_t                  animCount;

        CFileMgr::SetDir("");
        const auto f = CFileMgr::OpenFile("DATA\\ANIMGRP.DAT", "rb");
        for (;;) {
            const auto l = CFileLoader::LoadLine(f);
            if (!l) {
                break;
            }
            if (!*l || *l == '#') {
                continue;
            }
            if (isAnimSection) {
                if (sscanf_s(l, "%s", SCANF_S_STR(groupName)) == 1) {
                    if (!memcmp(groupName, "end", 4)) {
                        isAnimSection = false;
                    }
                    else {
                        AddAnimToAssocDefinition(def, groupName);
                    }
                }
            }
            else 
            {
                assert(sscanf_s(l, "%s %s %s %d", SCANF_S_STR(groupName), SCANF_S_STR(blockName), SCANF_S_STR(type), &animCount) == 4);

                AnimDescriptor* AnimDsc = GetDescriptorForAnimGroup(type);

                //def = AddAnimAssocDefinition(groupName, blockName, MODEL_MALE01, animCount, aStdAnimDescs);

                def = AddAnimAssocDefinition(groupName, blockName, MODEL_MALE01, animCount, AnimDsc);
                isAnimSection = true;
            }
        }
        CFileMgr::CloseFile(f);
    }

    static AnimAssocDefinition* AddAnimAssocDefinition(const char* groupName, const char* blockName, uint32_t modelIndex, uint32_t animsCount, AnimDescriptor* descriptor)
    {
        return plugin::CallAndReturn<AnimAssocDefinition*, 0x4D3BA0, char const*, char const*, unsigned int, unsigned int, void*>
                                                                    (groupName, blockName, modelIndex, animsCount, descriptor);
        /*
        const auto def = &New_ms_aAnimAssocDefinitions[NUM_ANIM_ASSOC_GROUPS++];

        strcpy_s(def->GroupName, groupName);
        strcpy_s(def->BlockName, blockName);

        def->ModelIndex = modelIndex;
        def->NumAnims = animsCount;
        def->AnimDescr = descriptor;

        def->AnimNames = new const char* [animsCount]; // This is so incredibly retarded there are no words for it....
        const auto bufsz = AnimAssocDefinition::ANIM_NAME_BUF_SZ * animsCount;
        const auto buf = new char[bufsz];
        memset(buf, 0, bufsz);
        for (auto i = animsCount; i-- > 0;) {
            def->AnimNames[i] = buf + i * AnimAssocDefinition::ANIM_NAME_BUF_SZ;
        }

        return def;*/
    }

    static void* AddAnimToAssocDefinition(AnimAssocDefinition* def, const char* animName)
    {
        return plugin::CallAndReturn<void*, 0x4D3C80, void*, char const*>(def, animName);
        /*
        int32_t i = 0;
        for (; def->AnimNames[i][0]; i++) {
            assert(i < def->NumAnims);
        }
        // `const_cast` is fine here, because it's heap allocated [presumeably]
        strcpy_s(const_cast<char*>(def->AnimNames[i]), AnimAssocDefinition::ANIM_NAME_BUF_SZ, animName);*/
    }

    static AnimDescriptor* GetDescriptorForAnimGroup(char* DescName)
    {
        std::string CmpStr = DescName;
        CmpStr = TransformString(CmpStr);

        if (AnimDescriptorNames.contains(CmpStr))
            return AnimDescriptorNames[CmpStr];
        else
            return aStdAnimDescs;
    }

} gInstance;
