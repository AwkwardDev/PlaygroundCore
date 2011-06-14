/* Gunship Battle
 * Dev start : 29 / 05 / 2011
 * TODO List :
 * - [COMPLETE] Fix Below Zero to freeze only cannons.
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

struct NPCsPositions
{
    uint32 npcId; // `creature_template`.`entry`
    // The positions should be set offseted from the transport's position.
    Position positionPlayer; // Position if the creature is friendly with the players
    Position positionEnemy; // Position if the creature is on the enemy ship
};

const NPCsPositions npcPositions[]=
{
    {36948, {-472.596f, 2466.8701f, 190.7371f, 6.204f}, {0.0f, 0.0f, 0.0f, 0.0f}}, // Muradin, player ship (Alliance team)
};

enum Spells
{
    // Cannon
    SPELL_OVERHEAT                    = 69487, // Triggers spell #69488 every 0.25s.
    SPELL_CANNON_BLAST                = 69399,
    SPELL_INCINERATING_BLAST          = 69401,

    // Auras
    SPELL_ON_ORGRIMS_HAMMERS_DECK     = 70121,
    SPELL_ON_SKYBREAKERS_DECK         = 70120,

    // Achievement spell required target
    SPELL_ACHIEVEMENT                 = 72959,

    // Rampart of Skulls Shared NPCs spells
    SPELL_WRATH                       = 69968,
    SPELL_HEALING_TOUCH               = 69899,
    SPELL_REGROWTH                    = 69882,
    SPELL_REJUVENATION                = 69898,

    // Kor'kron Battle-mage & Skybreaker Sorcerer
    SPELL_BELOW_ZERO                  = 69705,

    // Experience spells
    SPELL_EXPERIENCED                 = 71188,
    SPELL_VETERAN                     = 71193,
    SPELL_ELITE                       = 71195,
    SPELL_DESPERATE_RESOLVE           = 69647,

    // Kor'kron Axethrower & Skybreaker Rifleman
    SPELL_HURL_AXE                    = 70161,
    SPELL_SHOOT                       = 70162,
};

enum Events
{
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

    // Rampart of Skulls NPCs Events
    EVENT_WRATH,
    EVENT_HEAL,

    // First Squad Assisted
    EVENT_FIRST_SQUAD_ASSISTED_1,
    EVENT_FIRST_SQUAD_ASSISTED_2,

    // Shared experience events
    EVENT_EXPERIENCED,
    EVENT_VETERAN,
    EVENT_ELITE,

    // Kor'kron Axethrower & Skybreaker Rifleman
    EVENT_HURL_AXE,
    EVENT_SHOOT,
};

enum Texts
{
    // Kor'kron Primalist says: Thank the spirits for you, brothers and sisters. 
    // The Skybreaker has already left. Quickly now, to Orgrim's Hammer! If you
    // leave soon, you may be able to catch them.
    TALK_FIRST_SQUAD_RESCUED_HORDE_0 = 0,
    // Kor'kron Invoker says: This should be helpin'ya!
    TALK_FIRST_SQUAD_RESCUED_HORDE_1 = 0,

    // Kor'kron Defender says: Aka'Magosh, brave warriors. The alliance is in great number here
    TALK_SECOND_SQUAD_RESCUED_HORDE_0 = 0,
    // Kor'kron Defender says: Captain Saurfang will be pleased to see you aboard Orgrim's Hammer. 
    // Make haste, we will secure the area until you are ready for take-off.
    TALK_SECOND_SQUAD_RESCUED_HORDE_1 = 1,

    // -- These two are left to do
    // A screeching cry pierces the air above! (Widescreen Yellow Emote)
    // A Spire Frostwyrm lands just before Orgrim's Hammer. (Chat message)
    // --

    // High Overlord Saurfang yells: Rise up, sons and daughters of the Horde! 
    // Today we battle a hated enemy of the Horde! LOK'TAR OGAR! Kor'kron, take us out!
    TALK_INTRO_HORDE_1                  = 0,
    // High Overlord Saurfang yells: What is that?! Something approaching in the distance!
    TALK_INTRO_HORDE_2                  = 1,
    // High Overlord Saurfang yells: ALLIANCE GUNSHIP! ALL HANDS ON DECK!
    TALK_INTRO_HORDE_3                  = 2,
    // Muradin Bronzebeard yells: Move yer jalopy or we'll blow it out of
    // the sky, orc! The Horde's got no business here!
    TALK_INTRO_HORDE_3                  = 3,

    // High Overlord Saurfang yells: You will know our business soon! KOR'KRON, ANNIHILATE THEM!
    TALK_INTRO_HORDE_4                  = 4,
};

/* Muradin event starter */
/* Todo: remove the spawning system from him and apply it the the squads. pInstance->CreateTransport() */
/* Also add a method to position the gunship depending on the encounter state : See m_Waypoints */
class npc_muradin_gunship : public CreatureScript
{
    public:
        npc_muradin_gunship() : CreatureScript("npc_muradin_gunship") { }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            InstanceScript* pInstance = pCreature->GetInstanceScript();
            if (pInstance && pInstance->GetBossState(DATA_GUNSHIP_EVENT) != DONE)
            {
                pPlayer->ADD_GOSSIP_ITEM(0, "My companions are all accounted for, Muradin. Let's go!", 631, 1001);
                pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
                return true;
            }

            return false;
        }

        bool OnGossipSelect(Player* player, Creature* pCreature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (action == 1001) // Make the boat move
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

                    // skybreaker->BuildStopMovePacket(tMap); // Nope, we're testin'
                }
                else
                    pCreature->MonsterYell("FUUUUU I'm not on my ship !", LANG_UNIVERSAL, 0);
            }

            return true;
        }

        private:
            InstanceScript* pInstance;
            Transport* playersBoat;
};

/* Player's transport script */
class transport_gunship : public TransportScript
{
    public:
        transport_gunship() : TransportScript("transport_gunship") { }

        void OnRelocate(Transport* transport, uint32 waypointId, uint32 mapId, float x, float y, float z)
        {
            sLog->outString("ICC::Gunship: Transport %s reached waypoint %u/%u. Position is X:%u, Y:%u, Z:%u.", GetName(), waypointId, transport->m_WayPoints.size(), x, y, z);
        }

        void OnAddPassenger(Transport* transport, Player* player)
        {
            if (InstanceScript* instance = transport->GetInstanceScript())
            {
                switch (instance->GetData(DATA_TEAM_IN_INSTANCE))
                {
                    case HORDE:
                        player->AddAura(SPELL_ON_ORGRIMS_HAMMERS_DECK, player);
                        break;
                    case ALLIANCE:
                        player->AddAura(SPELL_ON_SKYBREAKERS_DECK, player);
                        break;
                }
            }
        }

        void OnRemovePassenger(Transport* transport, Player* player)
        {
            // Simply ...
            player->RemoveAurasDueToSpell(SPELL_ON_ORGRIMS_HAMMERS_DECK);
            player->RemoveAurasDueToSpell(SPELL_ON_SKYBREAKERS_DECK);
        }
};

/* Zafod boombox */
class npc_zafod_boombox : public CreatureScript
{
    public:
        npc_zafod_boombox() : CreatureScript("npc_zafod_boombox") { }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pPlayer->GetItemCount(49278, false) == 0)
                pPlayer->ADD_GOSSIP_ITEM(0, "Yeah, I'm sure safety is your top priority. Give me a rocket pack.", 631, 1);
            pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        }

        bool OnGossipSelect(Player* player, Creature* pCreature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (action == 1)
            {
                // Seurity, this shouldn't happen. Maybe useless, I can't doublecheck atm if the item is unique.
                uint32 itemId = 49278;
                uint32 curItemCount = player->GetItemCount(itemId, false);
                if (curItemCount >= 1)
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
                    me->GetVehicleKit()->RemoveAllPassengers();
            }

            void UpdateAI(const uint32 diff)
            {
                if (Powers powerType = me->getPowerType())
                    if (me->GetPower(powerType) == me->GetMaxPower(powerType))
                        DoCast(me, SPELL_OVERHEAT);
            }
        };
};

/* Kro'kron Axethrower */
class npc_korkron_axethrower : public CreatureScript
{
    public:
        npc_korkron_axethrower() : CreatureScript("npc_korkron_axethrower") { }

        struct npc_korkron_axethrowerAI : public ScriptedAI
        {
            npc_korkron_axethrowerAI(Creature *creature) : ScriptedAI(creature) { }

            void Reset()
            {
                ScriptedAI::Reset();
                if (me->GetInstanceScript()->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE)
                {
                    events.ScheduleEvent(EVENT_EXPERIENCED, urand(19000, 21000)); // ~20 sec
                    events.ScheduleEvent(EVENT_VETERAN, urand(39000, 41000));     // ~40 sec
                    events.ScheduleEvent(EVENT_ELITE, urand(59000, 61000));       // ~60 sec
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (!me->HasUnitState(UNIT_STAT_CASTING))
                    DoCast(me->getVictim(), SPELL_HURL_AXE);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
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
            return new npc_korkron_axethrowerAI(pCreature);
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
                if (me->GetInstanceScript()->GetData(DATA_TEAM_IN_INSTANCE) == HORDE)
                {
                    events.ScheduleEvent(EVENT_EXPERIENCED, urand(19000, 21000)); // ~20 sec
                    events.ScheduleEvent(EVENT_VETERAN, urand(39000, 41000));     // ~40 sec
                    events.ScheduleEvent(EVENT_ELITE, urand(59000, 61000));       // ~60 sec
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (!me->HasUnitState(UNIT_STAT_CASTING))
                    DoCast(me->getVictim(), SPELL_SHOOT);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
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
/* Todo */

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
                events.ScheduleEvent(EVENT_WRATH, 10000); // TODO: Fix the timers
                events.ScheduleEvent(EVENT_HEAL, 20000); // TODO: Fix the timers
            }

            void UpdateAI(const uint32 diff)
            {
                if (!instance)
                    return;

                if (instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE)
                {
                    if (!instance->GetData(DATA_FIRST_SQUAD_ASSISTED)) // boolean, true = done
                    {
                        instance->SetData(DATA_FIRST_SQUAD_ASSISTED, true);
                        events.ScheduleEvent(EVENT_FIRST_SQUAD_ASSISTED_1, 100);
                        events.ScheduleEvent(EVENT_FIRST_SQUAD_ASSISTED_2, 15000); // TODO : fix the timer
                    }
                }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FIRST_SQUAD_ASSISTED_1:
                            Talk(TALK_FIRST_SQUAD_RESCUED_HORDE_0);
                            break;
                        case EVENT_FIRST_SQUAD_ASSISTED_2:
                            if (Creature* tempUnit = me->FindNearestCreature(NPC_KORKRON_INVOKER, 50.0f, true))
                                tempUnit->AI()->Talk(TALK_FIRST_SQUAD_RESCUED_HORDE_1);
                            break;
                        case EVENT_WRATH:
                            if (me->isInCombat())
                                if (!me->getVictim()->IsFriendlyTo(me))
                                    me->CastSpell(me->getVictim(), SPELL_WRATH, false);
                            events.ScheduleEvent(EVENT_WRATH, 10000);
                            break;
                        case EVENT_HEAL:
                            if (me->isInCombat())
                            {
                                std::list<Unit*> TargetList;
                                Unit* finalTarget = me;
                                Trinity::AnyFriendlyUnitInObjectRangeCheck checker(me, me, 30.0f);
                                Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, TargetList, checker);
                                me->VisitNearbyObject(30.0f, searcher);
                                for (std::list<Unit*>::iterator itr = TargetList.begin(); itr != TargetList.end(); ++itr)
                                    if ((*itr)->GetHealthPct() < finalTarget->GetHealthPct())
                                        finalTarget = *itr;

                                uint32 spellId = SPELL_HEALING_TOUCH;
                                uint32 healthPct = finalTarget->GetHealthPct();
                                if (healthPct > 15 && healthPct < 20)
                                    spellId = (urand (0, 1) ? SPELL_REGROWTH : SPELL_HEALING_TOUCH);
                                else if (healthPct >= 20 && healthPct < 40)
                                    spellId = SPELL_REGROWTH;
                                else if (healthPct > 40)
                                    spellId = (urand(0, 1) ? SPELL_REJUVENATION : SPELL_REGROWTH);

                                me->CastSpell(finalTarget, spellId, false);
                                events.ScheduleEvent(EVENT_HEAL, 20000);
                            }
                            break;
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

class spell_overheat : public SpellScriptLoader
{
    public:
        spell_overheat() : SpellScriptLoader("spell_overheat") { }

        class spell_overheat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_overheat_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                Unit* passenger = target->GetVehicle()->GetPassenger(1);
                if (passenger->GetTypeId() == TYPEID_PLAYER)
                {
                    passenger->ToPlayer()->AddSpellCooldown(SPELL_CANNON_BLAST, 0, time(NULL) + 3000);
                    passenger->ToPlayer()->AddSpellCooldown(SPELL_INCINERATING_BLAST, 0, time(NULL) + 3000);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_overheat_AuraScript::OnApply, EFFECT_0, SPELL_EFFECT_APPLY_AURA, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_overheat_AuraScript();
        }
};

void AddSC_boss_gunship_battle()
{
    new transport_gunship();
    new npc_muradin_gunship();
    new npc_zafod_boombox();
    new npc_gunship_cannon();
    new npc_korkron_axethrower();
    new npc_skybreaker_rifleman();
    new spell_overheat();
}