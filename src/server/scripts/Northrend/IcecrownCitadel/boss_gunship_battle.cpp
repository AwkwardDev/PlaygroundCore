/* Gunship Battle
 * Dev start : 29 / 05 / 2011
 * TODO List :
 * - [COMPLETE] Fix Below Zero to freeze only cannons. Mages are done with SAI
     -> http://www.trinitycore.org/f/topic/3269-complete-gunship-battle-sai/page__gopid__19479#entry19479
 * - Get approximative positions for every NPC, coupling Horde with Alliance to gain performance
 * - Encounter frames : the enemy one is the first one. They both are supposed to be friendly to 
        the player for him not to dot them, nor AoE, and should not be selectable.
        Hencemore, they appear when the enemy ship is boarding yours.
 * - Riflemen are spawned directly on the other side of the space separating Players VS NPCs. 4 of them
 * - Mortar soldiers and Rocketeers are spawned on the extremity of the ship. This can be clearly seen from videos. 2 of them.
 * - Rocket boots have an AoE damage when landing. Heroic only ?
 * - The targeting mark for rockets are cast when that spell is also cast, not when it hits the ground.
 * - When players win, the enemy zeppelin escapes VERY quickly
 * Notes:
 * - Rocketeers seem to be able to cast on their own ship. For their spell, take a look at SPELL_ROCKET_ARTILLERY_TARGET_ALLIANCE and SPELL_ROCKET_ARTILLERY_TARGET_HORDE (Script effect, SpellEffect.cpp, EffectScriptEffect(EffIndex index))
 */

#include "ScriptPCH.h"
#include "icecrown_citadel.h"
#include "MapManager.h"
#include "Transport.h"

const Position npcPositions[] = 
{
    {-472.596f, 2466.8701f, 190.7371f, 6.204f}, // Muradin / Saurfang, in the ship of the players
};

enum Spells
{
    AURA_ON_OGRIM_HAMMER        = 70121,
    AURA_ON_SKYBREAKERS_DECK    = 70120,
    SPELL_OVERHEAT                = 69487,

    // Canon de la cannonière ( 2 en 10, 4 en 25)
    SPELL_CANNON_BLAST            = 69400,
    SPELL_INCINERATING_BLAST    = 69402,

    // High Overlord Saurcroc / Muradin Bronzebeard
    SPELL_CLEAVE                = 15284,
    SPELL_TASTE_FOR_BLOOD        = 69634,
    SPELL_RENDING_THROW            = 70309,

    // All troops
    SPELL_BURNING_PITCH            = 71335,
    // From WowWiki :
    // All enemy NPCs except the Commanders (Muradin / Saurfang) and the Battle-Mage/Sorceror gain 
    // experience the longer they are left alive, starting from Regular and progressing to Experienced, 
    // then Veteran, and finally Elite. Depending on rank, damage and attack speed are increased. 
    SPELL_EXPERIENCED            = 71188, // ~20 sec
    SPELL_VETERAN                = 71193, // ~40 sec
    SPELL_ELITE                    = 71195, // ~60 sec
    
    // Skybreaker Marine / Kor'kron Reaver / Skybreaker Sergeant / Kor'kron Sergeant 
    SPELL_DESPERATE_RESOLVE_10N    = 69647, // HP < 20%
    SPELL_DESPERATE_RESOLVE_10H    = 72537, // --------
    SPELL_DESPERATE_RESOLVE_25N    = 72536, // --------
    SPELL_DESPERATE_RESOLVE_25H = 72538, // --------

    // Kor'kron Sergeant & Skybreaker Sergeant
    SPELL_BLADESTORM            = 69652,
    SPELL_WOUNDING_STRIKE_10N    = 69651,
    SPELL_WOUNDING_STRIKE_10H    = 72569,
    SPELL_WOUNDING_STRIKE_25N    = 72570,
    SPELL_WOUNDING_STRIKE_25H    = 72571,

    // Kor'kron Axethrower & Skybreaker Rifleman
    SPELL_SHOOT_10N                = 70162,
    SPELL_SHOOT_10H                = 72567,
    SPELL_SHOOT_25N                = 72566,
    SPELL_SHOOT_25H                = 72568,
    SPELL_HURL_AXE_10N            = 70161,
    SPELL_HURL_AXE_10H            = 72540,
    SPELL_HURL_AXE_25N            = 72539,
    SPELL_HURL_AXE_25H            = 72541,
    
    // Skybreaker Sorcerer / Kor'kron Battle-Mage 
    SPELL_BELOW_ZERO            = 69705,
    
    // Skybreaker Mortar Soldier / Kor'kron Rocketeer 
    SPELL_ROCKET_ARTILLERY        = 69679,
    SPELL_ROCKET_ARTILLERY_TARGET_ALLIANCE = 69678, // Requires Target Aura Spell (70120) Sur le pont du Brise-ciel
    SPELL_ROCKET_ARTILLERY_TARGET_HORDE       = 70609, // Requires Target Aura Spell (70121) Sur le pont du Marteau d'Orgrim
    SPELL_EXPLOSION                = 69680, // Triggered by the previous one when it hits the ground
    SPELL_TARGET_MARKER            = 71371, // Triggered by 69679 when it is CAST

    // Achievement
    SPELL_ACHIEVEMENT            = 72959, // You must be a tagrte for this spell to get the achievement
};

enum ExperienceEvents
{
    // Shared AI Events
    EVENT_EXPERIENCED            = 0,
    EVENT_VETERAN,
    EVENT_ELITE,

    // Muradin / Saurfang
    EVENT_CLEAVE,
    EVENT_TASTE_FOR_BLOOD,
    EVENT_RENDING_THROW,
    EVENT_NEW_RIFLEMEN_AXETHROWER,
    EVENT_BOARDING_PLAYERS_SHIP,
    EVENT_NEW_MORTAR_TEAM_SPAWNED_ROCKETEERS,
    EVENT_NEW_MAGE,

    EVENT_INTRO_HORDE_0, // High Overlord Saurfang yells: Rise up, sons and daughters of the Horde! Today we battle a hated enemy of the Horde! LOK'TAR OGAR! Kor'kron, take us out!
    EVENT_INTRO_HORDE_1, // High Overlord Saurfang yells: What is that?! Something approaching in the distance! 
    EVENT_INTRO_HORDE_2, // High Overlord Saurfang yells: ALLIANCE GUNSHIP! ALL HANDS ON DECK!
    EVENT_INTRO_HORDE_3, // Muradin Bronzebeard yells: Move yer jalopy or we'll blow it out of the sky, orc! The Horde's got no business here!
    EVENT_INTRO_HORDE_4, // High Overlord Saurfang yells: You will know our business soon! KOR'KRON, ANNIHILATE THEM!

    EVENT_INTRO_ALLIANCE_0, // Muradin Bronzebeard yells: Fire up the engines! We got a meetin' with destiny, lads!
    EVENT_INTRO_ALLIANCE_1, // Muradin Bronzebeard yells: Hold on to yer hats!
    EVENT_INTRO_ALLIANCE_2, // Muradin Bronzebeard yells: What in the world is that? Grab me spyglass, crewman!
    EVENT_INTRO_ALLIANCE_3, // Muradin Bronzebeard yells: By me own beard! HORDE SAILIN' IN FAST 'N HOT!
    EVENT_INTRO_ALLIANCE_4, // Muradin Bronzebeard yells: EVASIVE ACTION! MAN THE GUNS!
    EVENT_INTRO_ALLIANCE_5, // Muradin Bronzebeard yells: Cowardly dogs! Ye blindsided us!
    EVENT_INTRO_ALLIANCE_6, // High Overlord Saurfang yells: This is not your battle, dwarf. Back down or we will be forced to destroy your ship.
    EVENT_INTRO_ALLIANCE_7, // Muradin Bronzebeard yells: Not me battle? I dunnae who ye﻿ think ye are, mister, but I got a score to settle with Arthas and yer not gettin' in me way! FIRE ALL GUNS! FIRE! FIRE!

    // Skybreaker Marine & Kor'kron Reaver & Skybreaker Sergeant & Kor'kron Sergeant 
    EVENT_DESPERATE_RESOLVE,

    // Kor'kron Sergeant & Skybreaker Sergeant
    EVENT_BLADESTORM,
    EVENT_WOUNDING_STRIKE,

    // Kor'kron Axethrower
    EVENT_SHOOT,
    // Skybreaker Rifleman
    EVENT_HURL_AXE,

    // Skybreaker Mortar Soldier & Kor'kron Rocketeer
    EVENT_ROCKET_ARTILLERY,
};

#define AURA_BATTLE_FURY_HELPER            RAID_MODE<uint32>(72306, 72307, 72306, 72307)
#define SPELL_WOUNDING_STRIKE_HELPER    RAID_MODE<uint32>(69651, 72569, 72570, 72571)
#define SPELL_DESPERATE_RESOLVE_HELPER  RAID_MODE<uint32>(69647, 72537, 72536, 72538)
#define SPELL_SHOOT_HELPER                RAID_MODE<uint32>(70162, 72567, 72566, 72568)
#define SPELL_HURL_AXE_HELPER            RAID_MODE<uint32>(70161, 72540, 72539, 72541)

// Texts - Horde
#define SAY_KORKRON_FIRST_SQUAD_0       "Thank the spirits for you, brothers and sisters. The Skybreaker has already left. Quickly now, to Orgrim's Hammer! If you leave soon, you may be able to catch them."
#define SAY_KORKRON_FIRST_SQUAD_1       "This should be helpin'ya!"
#define SAY_KORKRON_SECOND_SQUAD_0      "Aka'Magosh, brave warriors. The alliance is in great number here."
#define SAY_KORKRON_SECOND_SQUAD_1      "Captain Saurfang will be pleased to see you aboard Orgrim's Hammer. Make haste, we will secure the area until you are ready for take-off."
#define YELL_EVENT_BEGIN_HORDE_HIGH_OVERLORD_SAURFANG_0 "Rise up, sons and daughters of the Horde! Today we battle a hated enemy of the Horde! LOK'TAR OGAR! Kor'kron, take us out!"
#define YELL_EVENT_BEGIN_HORDE_HIGH_OVERLORD_SAURFANG_1 "What is that?! Something approaching in the distance!"
#define YELL_EVENT_BEGIN_HORDE_HIGH_OVERLORD_SAURFANG_2 "ALLIANCE GUNSHIP! ALL HANDS ON DECK!"
#define YELL_EVENT_BEGIN_HORDE_MURADIN_BRONZEBEARD_0    "Move yer jalopy or we'll blow it out of the sky, orc! The Horde's got no business here!"
#define YELL_EVENT_BEGIN_HORDE_HIGH_OVERLORD_SAURFANG_3         "You will know our business soon! KOR'KRON, ANNIHILATE THEM!"
#define YELL_BOARDING_ORGRIM_S_HAMMER_MURADIN_BRONZEBEARD_0     "Marines, Sergeants, attack!"
#define YELL_BOARDING_ORGRIM_S_HAMMER_HIGH_OVERLORD_SAURFANG_0  "You DARE board my ship? Your death will come swiftly."
#define YELL_NEW_RIFLEMEN_MURADIN       "Riflemen, shoot faster!"
#define YELL_NEW_MORTAR_TEAM_MURADIN    "Mortar team, reload!"
#define YELL_NEW_MAGE_MURADIN           "We're taking hull damage, get a sorcerer out here to shut down those cannons!"
#define YELL_HORDE_VICTORY_SAURFANG     "The Alliance falter. Onward to the Lich King!"
#define YELL_HORDE_DEFEAT_SAURFANG      "Damage control! Put those fires out! You haven't seen the last of the Horde!"

// Texts - Alliance
#define SAY_SKYBREAKER_FIRST_SQUAD_0    "Thank goodness you arrived when you did, heroes. Orgrim's Hammer has already left. Quickly now, to The Skybreaker! If you leave soon, you may be able to catch them."
#define SAY_SKYBREAKER_FIRST_SQUAD_1    "This ought to help!"
#define SAY_SKYBREAKER_SECOND_SQUAD_0   "You have my thanks. We were outnumbered until you arrived."
#define SAY_SKYBREAKER_SECOND_SQUAD_1   "Captain Muradin is waiting aboard The Skybreaker. We'll secure the area until you are ready for take off."
#define SAY_SKYBREAKER_SECOND_SQUAD_2   "Skybreaker infantry, hold position!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_0         "Fire up the engines! We got a meetin' with destiny, lads!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_1         "Hold on to yer hats!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_2         "What in the world is that? Grab me spyglass, crewman!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_3         "By me own beard! HORDE SAILIN' IN FAST 'N HOT!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_4         "EVASIVE ACTION! MAN THE GUNS!"
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_5         "Cowardly dogs! Ye blindsided us!"
#define YELL_EVENT_BEGIN_ALLIANCE_HIGH_OVERLORD_SAURFANG_0      "This is not your battle, dwarf. Back down or we will be forced to destroy your ship."
#define YELL_EVENT_BEGIN_ALLIANCE_MURADIN_BRONZEBEARD_6         "Not me battle? I dunnae who ye? think ye are, mister, but I got a score to settle with Arthas and yer not gettin' in me way! FIRE ALL GUNS! FIRE! FIRE!"
#define YELL_BOARDING_THE_SKYBREAKER_HIGH_OVERLORD_SAURFANG_0   "Reavers, Sergeants, attack!"
#define YELL_BOARDING_THE_SKYBREAKER_MURADIN_BRONZEBEARD_0      "What's this then?! Ye won't be takin' this son o' Ironforge's vessel without a fight!"
#define YELL_NEW_AXETHROWERS_HIGH_OVERLORD_SAURFANG_0   "Axethrowers, hurl faster!"
#define YELL_NEW_ROCKETEERS_HIGH_OVERLORD_SAURFANG_0    "Rocketeers, reload!"
#define YELL_NEW_BATTLE_MAGE_HIGH_OVERLORD_SAURFANG_0   "We're taking hull damage, get a battle-mage out here to shut down those cannons!"
#define YELL_ALLIANCE_VICTORY_MURADIN_BRONZEBEARD_0        "Don't say I didn't warn ya, scoundrels! Onward, brothers and sisters!"
#define YELL_ALLIANCE_DEFEAT_MURADIN_BRONZEBEARD_0        "Captain Bartlett, get us out of here! We're taken too much damage to stay afloat!"

/* Muradin event starter */
/* Todo: remove the spawning system from him and apply it the the squads */
class npc_muradin_gunship : public CreatureScript
{
    public:
        npc_muradin_gunship() : CreatureScript("npc_muradin_gunship") { }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            InstanceScript* pInstance = pCreature->GetInstanceScript();
            if (pInstance && pInstance->GetBossState(DATA_GUNSHIP_EVENT) != DONE)
            {
                if (!pCreature->GetTransport())
                    pPlayer->ADD_GOSSIP_ITEM(0, "Call the Skybreaker", 631, 1);
                else
                    pPlayer->ADD_GOSSIP_ITEM(0, "My companions are all accounted for, Muradin. Let's go!", 631, 2);

                pPlayer->ADD_GOSSIP_ITEM(0, "Despawn the Skybreaker", 631, 3);
                pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
                return true;
            }

            return false;
        }

        bool OnGossipSelect(Player* player, Creature* pCreature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (action == 1) // Call the boat
            {
                uint32 goEntry = 201811; // Horde: 201812
                uint32 period = 51584;

                Transport* playersBoat = instance->CreateTransport(goEntry, 51584);
                playersBoat->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                playersBoat->SetGoState(GO_STATE_READY);
                // Add it to the grid in case TC uses it to track transports
                Map* tMap = player->GetMap();
                playersBoat->SetMap(tMap);
                playersBoat->AddToWorld();

                // Transmit creation packet to all players on the map
                for (Map::PlayerList::const_iterator itr = tMap->GetPlayers().begin(); itr != tMap->GetPlayers().end(); ++itr)
                    if (Player* pPlayer = itr->getSource())
                    {
                        UpdateData transData;
                        playersBoat->BuildCreateUpdateBlockForPlayer(&transData, pPlayer);
                        WorldPacket packet;
                        transData.BuildPacket(&packet);
                        pPlayer->SendDirectMessage(&packet);
                    }
                
                sMapMgr->m_Transports.insert(playersBoat);
                playersBoat->Update(1);
                playersBoat->BuildStopMovePacket(tMap);
            }
            else if (action == 2) // Make the boat move
            {
                if (Transport* playersBoat = pCreature->GetTransport())
                {    
                    Map* tMap = player->GetMap();
                    pInstance->SetData(DATA_GUNSHIP_EVENT, IN_PROGRESS);
                    playersBoat->BuildStartMovePacket(tMap);
                    playersBoat->SetGoState(GO_STATE_ACTIVE);
                    playersBoat->Update(0);
                    playersBoat->SetUInt32Value(GAMEOBJECT_DYNAMIC, 0x10830010); // Seen in sniffs
                    playersBoat->SetFloatValue(GAMEOBJECT_PARENTROTATION + 3, 1.0f);
                   
                    // Transmit movement packet
                    for (Map::PlayerList::const_iterator itr = tMap->GetPlayers().begin(); itr != tMap->GetPlayers().end(); ++itr)
                        if (Player* pPlayer = itr->getSource())
                        {
                            UpdateData transData;
                            playersBoat->BuildCreateUpdateBlockForPlayer(&transData, pPlayer);
                            WorldPacket packet;
                            transData.BuildPacket(&packet);
                            pPlayer->GetSession()->SendPacket(&packet);
                        }

                    // skybreaker->BuildStopMovePacket(tMap);
                }    
            }
            // Despawn the skybreaker (debug gossip option) because when the gunship is ended, the players' boat stays 
            // but the adversary's one despawns when it reaches end of the waypoint.
            else if (action == 3)
            {
                Transport* playersBoat = pCreature->GetTransport();
                if (!playersBoat)
                    return true;

                sMapMgr->m_Transports.erase(playersBoat);
                for (Transport::PlayerSet::const_iterator itr = playersBoat->GetPassengers().begin(); itr != playersBoat->GetPassengers().end(); ++itr)
                {
                    playersBoat->RemovePassenger(*itr);
                    if (Player* plr = *itr)
                        plr->SetTransport(NULL);
                }
                // Uncomment and fix this when NPC passengers will be added
                /*
                for (Transport::CreatureSet::iterator itr = playersBoat->m_NPCPassengerSet.begin(); itr != playersBoat->m_NPCPassengerSet.end();)
                    if (Creature *npc = *(itr++))
                        npc->AddObjectToRemoveList();

                skybreaker->m_NPCPassengerSet.clear();
                */
                UpdateData transData;
                playersBoat->BuildOutOfRangeUpdateBlock(&transData);
                WorldPacket out_packet;
                transData.BuildPacket(&out_packet);

                for (Map::PlayerList::const_iterator itr = playersBoat->GetMap()->GetPlayers().begin(); itr != playersBoat->GetMap()->GetPlayers().end(); ++itr)
                    if (playersBoat != itr->getSource()->GetTransport())
                        itr->getSource()->SendDirectMessage(&out_packet);

                playersBoat->Delete();
                playersBoat = NULL;
            }

            return true;
        }

        private:
            InstanceScript* pInstance;
            uint64 transportLowGuid;
            Transport* playersBoat;
};

/* Zafod boombox */
class npc_zafod_boombox : public CreatureScript
{
    public:
        npc_zafod_boombox() : CreatureScript("npc_zafod_boombox") { }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            pPlayer->ADD_GOSSIP_ITEM(0, "Yeah, I'm sure safety is your top priority. Give me a rocket pack.", 631, 1);
            pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        }

        bool OnGossipSelect(Player* player, Creature* pCreature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (action == 1)
            {
                uint32 itemId = 49278;
                uint32 curItemCount = player->GetItemCount(itemId, false);
                if (curItemCount == 1)
                {
                    pCreature->MonsterWhisper("You already have my rocket pack !", player->GetGUIDLow());
                    return false;
                }

                ItemPosCountVec dest;
                uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, 1);
                if (msg == EQUIP_ERR_OK)
                {
                    Item* item = player->StoreNewItem(dest, itemId, true);
                    player->SendNewItem(item, 1, true, false);
                }
            }

            return true;
        }
};

/* Gunship Cannon (Horde / Ally) */
class npc_gunship_cannon : public CreatureScript
{
    public:
        npc_gunship_cannon() : CreatureScript("npc_gunship_cannon") { }

        struct npc_gunship_cannonAI : public ScriptedAI
        {
            npc_gunship_cannonAI(Creature *creature) : ScriptedAI(creature) { }

            void SpellHit(Unit* caster, SpellEntry const* spellEntry)
            {
                if (spellEntry->Id == SPELL_BELOW_ZERO)
                {
                    me->GetVehicleKit()->RemoveAllPassengers();
                }
            }

            void UpdateAI(const uint32 diff)
            {
                //if (Powers powerType = me->getPowerType())
                //    if (me->GetPower(powerType) == me->GetMaxPower(powerType))
                        
            }
        };
};

/* Kro'kron Axethrower */
class npc_gunship_axethrower : public CreatureScript
{
    public:
        npc_gunship_axethrower() : CreatureScript("npc_gunship_axethrower") { }

        struct npc_gunship_axethrowerAI : public ScriptedAI
        {
            npc_gunship_axethrowerAI(Creature *creature) : ScriptedAI(creature) { }

            void Reset()
            {
                ScriptedAI::Reset();
                events.ScheduleEvent(EVENT_HURL_AXE, 2000);
                events.ScheduleEvent(EVENT_EXPERIENCED, urand(19, 21) * 1000); // ~20 sec
                events.ScheduleEvent(EVENT_VETERAN, urand(39, 41) * 1000);     // ~40 sec
                events.ScheduleEvent(EVENT_ELITE, urand(59, 61) * 1000);       // ~60 sec
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HURL_AXE:
                            DoCast(me->getVictim(), SPELL_HURL_AXE_HELPER);
                            events.ScheduleEvent(EVENT_HURL_AXE, 2000);
                            break;
                        case EVENT_EXPERIENCED:
                            DoCast(me, SPELL_EXPERIENCED);
                            break;
                        case EVENT_VETERAN:
                            DoCast(me, SPELL_VETERAN);
                            break;
                        case EVENT_ELITE:
                            DoCast(me, SPELL_ELITE);
                            break;
                        default:
                            break;
                    }
                }
            }

            private:
                EventMap events;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_gunship_axethrowerAI(pCreature);
        }
};

/* Skybreaker Rifleman */
class npc_skybreaker_rifleman : public CreatureScript
{
    public:
        npc_skybreaker_rifleman() : CreatureScript("npc_skybreaker_rifleman") { }

        struct npc_skybreaker_riflemanAI : public ScriptedAI
        {
            npc_skybreaker_riflemanAI(Creature *creature) : ScriptedAI(creature) { }

            void Reset()
            {
                ScriptedAI::Reset();
                events.ScheduleEvent(EVENT_SHOOT, 2000);
                events.ScheduleEvent(EVENT_EXPERIENCED, urand(19, 21) * 1000); // ~20 sec
                events.ScheduleEvent(EVENT_VETERAN, urand(39, 41) * 1000);     // ~40 sec
                events.ScheduleEvent(EVENT_ELITE, urand(59, 61) * 1000);       // ~60 sec
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HURL_AXE:
                            DoCast(me->getVictim(), SPELL_SHOOT_HELPER);
                            events.ScheduleEvent(EVENT_SHOOT, 2000);
                            break;
                        case EVENT_EXPERIENCED:
                            DoCast(me, SPELL_EXPERIENCED);
                            break;
                        case EVENT_VETERAN:
                            DoCast(me, SPELL_VETERAN);
                            break;
                        case EVENT_ELITE:
                            DoCast(me, SPELL_ELITE);
                            break;
                        default:
                            break;
                    }
                }
            }

            private:
                EventMap events;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_skybreaker_riflemanAI(pCreature);
        }
};

/* Skybreaker Sorcerer & Kor'kron Battle-Mage */
/* Those two are made with SAI along with SPELL_BELOW_ZERO */

/* Kor'kron Primalist */
class npc_korkron_primalist: public CreatureScript
{
    public:
        npc_korkron_primalist() : CreatureScript("npc_korkron_primalist") { }

        struct npc_korkron_primalistAI : public ScriptedAI
        {
            npc_korkron_primalistAI(Creature *creature) : ScriptedAI(creature) 
            {
                instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                ScriptedAI::Reset();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!instance)
                    return;
                
                if (!instance->GetData(DATA_FIRST_SQUAD_ASSISTED)) // boolean, true = done
                {
                    if (instance->GetData(DATA_RAMPART_TRASH_COUNT) >= REQUIRED_TRASH_FIRST_SQUAD)
                    {
                        instance->SetData(DATA_FIRST_SQUAD_ASSISTED, true);
                        me->MonsterYell(SAY_KORKRON_FIRST_SQUAD_0, LANG_UNIVERSAL, 0);
                    }
                }
            }

            private:
                EventMap events;
                InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_korkron_primalistAI(pCreature);
        }
};


void AddSC_boss_gunship_battle()
{
    // new transport_gunship();
    new npc_muradin_gunship();
    new npc_zafod_boombox();
    new npc_gunship_cannon();
    new npc_korkron_axethrower();
    new npc_skybreaker_rifleman();
}

 /* SQL REQUEST PATCH */

/*
-- ScriptNames Updates
UPDATE `creature_template` SET `ScriptName` = 'npc_muradin_gunship' WHERE `entry` = 36948;
UPDATE `creature_template` SET `ScriptName` = 'npc_saurfang_gunship' WHERE `entry` = 36939;
UPDATE `creature_template` SET `ScriptName` = 'npc_zafod_boombox' WHERE `entry` = 37184;
UPDATE `creature_template` SET `ScriptName` = 'npc_gunship_cannon' WHERE `entry` = 36838;
UPDATE `creature_template` SET `ScriptName` = 'npc_gunship_cannon' WHERE `entry` = 36839;
UPDATE `creature_template` SET `ScriptName` = 'npc_korkron_axethrower' WHERE `entry` = 36968;
UPDATE `creature_template` SET `ScriptName` = 'npc_skybreaker_rifleman' WHERE `entry` = 36969;

-- Linked spells
DELETE FROM `spell_linked_spell` WHERE `spell_trigger` = 71193 AND `spell_effect` = -71188;
DELETE FROM `spell_linked_spell` WHERE `spell_trigger` = 71195 AND `spell_effect` = -71193;
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) VALUES
(71193, -71188, 0, 'Gunship battle - Veteran removes Experimented'),
(71195, -71193, 0, 'Gunship battle - Elite removes Veteran');

-- Add spell conditions for 69705
SET @SPELL := 69705;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=13 AND `SourceEntry`=@SPELL;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(13,0,@SPELL,0,18,1,36838,0,0,'','Gunship Battle - Spell 69705 (Below Zero) target creature 36838'),
(13,0,@SPELL,0,18,1,36839,0,0,'','Gunship Battle - Spell 69705 (Below Zero) target creature 36839');

-- Battle Standards Spells
DELETE FROM `creature_template_addon` WHERE `entry` IN (37044, 37041);
SET @KORKRON := 37044; -- Kor'kron Battle Standard
SET @SKYBREAKER := 37041; -- Skybreaker Battle Standard
SET @HSPELL := 69809;
SET @ASPELL := 69808;
UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry` IN (@KORKRON, @SKYBREAKER);
DELETE FROM `smart_scripts` WHERE `source_type`=0 AND `entryorguid` IN (@KORKRON, @SKYBREAKER);
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(@KORKRON,0,0,0,1,0,100,1,1000,1000,1000,1000,11,@HSPELL,2,0,0,0,0,1,0,0,0,0,0,0,0, 'Kor''kron Battle Standard - On spawn & reset - Cast spell 69809'),
(@SKYBREAKER,0,0,0,1,0,100,1,1000,1000,1000,1000,11,@ASPELL,2,0,0,0,0,1,0,0,0,0,0,0,0, 'Skybreaker Battle Standard - On spawn & reset - Cast spell 69808');

-- Some tools
instance->SendEncounterUnit(ENCOUNTER_FRAME_ADD, keleseth);
Add an encounter frame
*/