/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "Vehicle.h"
#include "CombatAI.h"
#include "Player.h"
#include "Pet.h"
#include "SpellInfo.h"
#include "CreatureTextMgr.h"
#include "SpellScript.h"

// Ours
enum qRedRocket
{
    SPELL_TORPEDO_EXPLODE                       = 49290,
    SPELL_ALLIANCE_TORPEDO_KILL_CREDIT          = 49510,
    SPELL_HORDE_TORPEDO_KILL_CREDIT             = 49340,

};

class npc_riding_the_red_rocket : public CreatureScript
{
public:
    npc_riding_the_red_rocket() : CreatureScript("npc_riding_the_red_rocket") { }

    struct npc_riding_the_red_rocketAI : public VehicleAI
    {
        npc_riding_the_red_rocketAI(Creature* creature) : VehicleAI(creature) {}

        void AttackStart(Unit* /*who*/) {}
        void EnterCombat(Unit* /*who*/) {}
        void EnterEvadeMode() {}

        void UpdateAI(uint32  /*diff*/)
        {
            if (me->GetDistance(2763, -1596, 0) < 20.0f) // Horde ship
            {
                me->CastSpell(me, SPELL_ALLIANCE_TORPEDO_KILL_CREDIT, true);
                if (Unit* owner = me->GetCharmerOrOwner())
                {
                    Position pos(me->GetHomePosition());
                    owner->ExitVehicle();
                    me->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), false);
                    owner->CastSpell(owner, SPELL_TORPEDO_EXPLODE, true);
                    owner->KnockbackFrom(owner->GetPositionX(), owner->GetPositionY(), 5.0f, 50.0f);
                }
            }
            else if (me->GetDistance(2545, -2242, 0) < 20.0f) // Alliance ship
            {
                me->CastSpell(me, SPELL_HORDE_TORPEDO_KILL_CREDIT, true);
                if (Unit* owner = me->GetCharmerOrOwner())
                {
                    Position pos(me->GetHomePosition());
                    owner->ExitVehicle();
                    me->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), false);
                    owner->CastSpell(owner, SPELL_TORPEDO_EXPLODE, true);
                    owner->KnockbackFrom(owner->GetPositionX(), owner->GetPositionY(), 5.0f, 50.0f);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_riding_the_red_rocketAI(creature);
    }
};


// Theirs

/*######
## Quest 12027: Mr. Floppy's Perilous Adventure
######*/

enum Floppy
{
    // Creature
    NPC_MRFLOPPY                = 26589,
    NPC_HUNGRY_WORG             = 26586,
    NPC_RAVENOUS_WORG           = 26590,   // RWORG
    NPC_EMILY                   = 26588,
    // Quest
    QUEST_PERILOUS_ADVENTURE    = 12027,
    // Spell
    SPELL_MRFLOPPY              = 47184,   // vehicle aura
    // Text
    SAY_WORGHAGGRO1             = 0, // Um... I think one of those wolves is back...
    SAY_WORGHAGGRO2             = 1, // He's going for Mr. Floppy!
    SAY_WORGRAGGRO3             = 2, // Oh, no! Look, it's another wolf, and it's a biiiiiig one!
    SAY_WORGRAGGRO4             = 3, // He's gonna eat Mr. Floppy! You gotta help Mr. Floppy! You just gotta!
    SAY_RANDOMAGGRO             = 4, // There's a big meanie attacking Mr. Floppy! Help!
    SAY_VICTORY1                = 5, // Let's get out of here before more wolves find us!
    SAY_VICTORY2                = 6, // Don't go toward the light, Mr. Floppy!
    SAY_VICTORY3                = 7, // Mr. Floppy, you're ok! Thank you so much for saving Mr. Floppy!
    SAY_VICTORY4                = 8, // I think I see the camp! We're almost home, Mr. Floppy! Let's go!
    TEXT_EMOTE_WP1              = 9, // Mr. Floppy revives
    TEXT_EMOTE_AGGRO            = 10, // The Ravenous Worg chomps down on Mr. Floppy
    SAY_QUEST_ACCEPT            = 11, // Are you ready, Mr. Floppy? Stay close to me and watch out for those wolves!
    SAY_QUEST_COMPLETE          = 12  // Thank you for helping me get back to the camp. Go tell Walter that I'm safe now!
};

// emily
class npc_emily : public CreatureScript
{
public:
    npc_emily() : CreatureScript("npc_emily") { }

    struct npc_emilyAI : public npc_escortAI
    {
        npc_emilyAI(Creature* creature) : npc_escortAI(creature) { }

        void JustSummoned(Creature* summoned)
        {
            if (Creature* Mrfloppy = GetClosestCreatureWithEntry(me, NPC_MRFLOPPY, 50.0f))
                summoned->AI()->AttackStart(Mrfloppy);
            else
                summoned->AI()->AttackStart(me->GetVictim());
        }

        void WaypointReached(uint32 waypointId)
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 9:
                    if (Creature* Mrfloppy = GetClosestCreatureWithEntry(me, NPC_MRFLOPPY, 100.0f))
                        _mrfloppyGUID = Mrfloppy->GetGUID();
                    break;
                case 10:
                    if (ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                    {
                        Talk(SAY_WORGHAGGRO1);
                        if (Creature* worg = me->SummonCreature(NPC_HUNGRY_WORG, me->GetPositionX() + 5, me->GetPositionY() + 2, me->GetPositionZ() + 1, 3.229f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
                            if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                            {
                                worg->SetReactState(REACT_AGGRESSIVE);
                                worg->GetAI()->AttackStart(Mrfloppy);
                            }
                    }
                    break;
                case 11:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                        Mrfloppy->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                    break;
                case 17:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                        Mrfloppy->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                    Talk(SAY_WORGRAGGRO3);
                    if (Creature* RWORG = me->SummonCreature(NPC_RAVENOUS_WORG, me->GetPositionX()+10, me->GetPositionY()+8, me->GetPositionZ()+2, 3.229f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 120000))
                    {
                        RWORG->SetReactState(REACT_PASSIVE);
                        RWORG->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        RWORG->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                        _RavenousworgGUID = RWORG->GetGUID();
                    }
                    break;
                case 18:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                    {
                        if (Creature* RWORG = ObjectAccessor::GetCreature(*me, _RavenousworgGUID))
                            RWORG->GetMotionMaster()->MovePoint(0, Mrfloppy->GetPositionX(), Mrfloppy->GetPositionY(), Mrfloppy->GetPositionZ());
                        me->AddAura(SPELL_MRFLOPPY, Mrfloppy);
                    }
                    break;
                case 19:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                    {
                        if (Mrfloppy->HasAura(SPELL_MRFLOPPY, 0))
                        {
                            if (Creature* RWORG = ObjectAccessor::GetCreature(*me, _RavenousworgGUID))
                                Mrfloppy->EnterVehicle(RWORG);
                        }
                    }
                    break;
                case 20:
                    if (Creature* RWORG = ObjectAccessor::GetCreature(*me, _RavenousworgGUID))
                        RWORG->HandleEmoteCommand(34);
                    break;
                case 21:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                    {
                        if (Creature* RWORG = ObjectAccessor::GetCreature(*me, _RavenousworgGUID))
                        {
                            Unit::Kill(RWORG, Mrfloppy);
                            Mrfloppy->ExitVehicle();
                            RWORG->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            RWORG->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                            RWORG->AI()->AttackStart(player);
                            Talk(SAY_VICTORY2);
                        }
                    }
                    break;
                case 22:
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                    {
                        if (Mrfloppy->isDead())
                        {
                            me->GetMotionMaster()->MovePoint(0, Mrfloppy->GetPositionX(), Mrfloppy->GetPositionY(), Mrfloppy->GetPositionZ());
                            Mrfloppy->setDeathState(ALIVE);
                            Mrfloppy->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                            Talk(SAY_VICTORY3);
                        }
                    }
                    break;
                case 24:
                    if (player)
                    {
                        player->GroupEventHappens(QUEST_PERILOUS_ADVENTURE, me);
                        Talk(SAY_QUEST_COMPLETE, player);
                    }
                    me->SetWalk(false);
                    break;
                case 25:
                    Talk(SAY_VICTORY4);
                    break;
                case 27:
                    me->DisappearAndDie();
                    if (Creature* Mrfloppy = ObjectAccessor::GetCreature(*me, _mrfloppyGUID))
                        Mrfloppy->DisappearAndDie();
                    break;
            }
        }

        void EnterCombat(Unit* /*Who*/)
        {
            Talk(SAY_RANDOMAGGRO);
        }

        void Reset()
        {
            _mrfloppyGUID     = 0;
            _RavenousworgGUID = 0;
        }

        private:
            uint64   _RavenousworgGUID;
            uint64   _mrfloppyGUID;
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_PERILOUS_ADVENTURE)
        {
            creature->AI()->Talk(SAY_QUEST_ACCEPT);
            if (Creature* Mrfloppy = GetClosestCreatureWithEntry(creature, NPC_MRFLOPPY, 180.0f))
                Mrfloppy->GetMotionMaster()->MoveFollow(creature, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

            if (npc_escortAI* pEscortAI = CAST_AI(npc_emily::npc_emilyAI, (creature->AI())))
                pEscortAI->Start(true, false, player->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_emilyAI(creature);
    }
};

// mrfloppy
class npc_mrfloppy : public CreatureScript
{
public:
    npc_mrfloppy() : CreatureScript("npc_mrfloppy") { }

    struct npc_mrfloppyAI : public ScriptedAI
    {
        npc_mrfloppyAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() { }

        void EnterCombat(Unit* Who)
        {
            if (Creature* Emily = GetClosestCreatureWithEntry(me, NPC_EMILY, 50.0f))
            {
                switch (Who->GetEntry())
                {
                    case NPC_HUNGRY_WORG:
                        Emily->AI()->Talk(SAY_WORGHAGGRO2);
                        break;
                    case NPC_RAVENOUS_WORG:
                        Emily->AI()->Talk(SAY_WORGRAGGRO4);
                        break;
                    default:
                        Emily->AI()->Talk(SAY_RANDOMAGGRO);
                }
            }
        }

        void EnterEvadeMode()
        {
            if (Creature* Emily = GetClosestCreatureWithEntry(me, NPC_EMILY, 50.0f))
                me->GetMotionMaster()->MoveFollow(Emily, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        }

        void MoveInLineOfSight(Unit* /*who*/) { }


        void UpdateAI(uint32 /*diff*/)
        {
            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mrfloppyAI(creature);
    }
};

// Ravenous Worg
class npc_ravenous_worg : public CreatureScript
{
public:
    npc_ravenous_worg() : CreatureScript("npc_ravenous_worg") { }

    struct npc_ravenous_worgAI : public CombatAI
    {
        npc_ravenous_worgAI(Creature* creature) : CombatAI(creature)
        {
            _pacified = false;
            _attack = false;
        }

        void AttackStart(Unit* who)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
                _pacified = true;

            if (_pacified && !me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
                _attack = true;

            if (_attack)
                CombatAI::AttackStart(who);
        }

        void UpdateAI(uint32 diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
                _pacified = true;

            if (_pacified && !me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
                _attack = true;

            CombatAI::UpdateAI(diff);
        }

        private:
            bool   _pacified;
            bool   _attack;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ravenous_worgAI(creature);
    }
};

/*######
## Quest 12227: Doing Your Duty
######*/

enum Outhouse
{
    // Sound
    SOUND_FEMALE                    = 12671,
    SOUND_MALE                      = 12670,
    // Spell
    SPELL_OUTHOUSE_GROANS           = 48382,
    SPELL_CAMERA_SHAKE              = 47533,
    SPELL_DUST_FIELD                = 48329
};

class npc_outhouse_bunny : public CreatureScript
{
public:
    npc_outhouse_bunny() : CreatureScript("npc_outhouse_bunny") { }

    struct npc_outhouse_bunnyAI : public ScriptedAI
    {
        npc_outhouse_bunnyAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            _counter = 0;
            _gender  = 0;
        }

        void SetData(uint32 Type, uint32 Data)
        {
            if (Type == 1)
                _gender = Data;
        }

        void SpellHit(Unit* Caster, const SpellInfo* Spell)
        {
             if (Spell->Id == SPELL_OUTHOUSE_GROANS)
             {
                ++_counter;
                if (_counter < 5)
                    DoCast(Caster, SPELL_CAMERA_SHAKE, true);
                else
                    _counter = 0;
                DoCast(me, SPELL_DUST_FIELD, true);
                switch (_gender)
                {
                    case GENDER_FEMALE:
                        DoPlaySoundToSet(me, SOUND_FEMALE);
                        break;

                    case GENDER_MALE:
                        DoPlaySoundToSet(me, SOUND_MALE);
                        break;
                }
            }
        }
        private:
            uint8 _counter;
            uint8 _gender;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_outhouse_bunnyAI(creature);
    }
};

// Tallhorn Stage

enum TallhornStage
{
    //Gameobject
    OBJECT_HAUNCH                   = 188665
};

class npc_tallhorn_stag : public CreatureScript
{
public:
    npc_tallhorn_stag() : CreatureScript("npc_tallhorn_stag") { }

    struct npc_tallhorn_stagAI : public ScriptedAI
    {
        npc_tallhorn_stagAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            _phase = 1;
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (_phase == 1)
            {
                if (me->FindNearestGameObject(OBJECT_HAUNCH, 2.0f))
                {
                    me->SetStandState(UNIT_STAND_STATE_DEAD);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    me->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);
                }
                _phase = 0;
            }
            DoMeleeAttackIfReady();
        }
        private:
            uint8 _phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tallhorn_stagAI(creature);
    }
};

// Amberpine Woodsman

enum AmberpineWoodsman
{
    // Creature
    NPC_TALLHORN_STAG                      = 26363
};

enum AmberpineWoodsmanEvents
{
    EVENT_WOODSMAN_1                       = 1,
    EVENT_WOODSMAN_2                       = 2
};

class npc_amberpine_woodsman : public CreatureScript
{
public:
    npc_amberpine_woodsman() : CreatureScript("npc_amberpine_woodsman") { }

    struct npc_amberpine_woodsmanAI : public ScriptedAI
    {
        npc_amberpine_woodsmanAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            if (me->FindNearestCreature(NPC_TALLHORN_STAG, 0.2f))
            {
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING);
            }
            else
                _events.ScheduleEvent(EVENT_WOODSMAN_1, 0);
        }

        void UpdateAI(uint32 diff)
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WOODSMAN_1:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_LOOT);
                        _events.ScheduleEvent(EVENT_WOODSMAN_2, 3000);
                        break;
                    case EVENT_WOODSMAN_2:
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_ATTACK1H);
                        _events.ScheduleEvent(EVENT_WOODSMAN_1, 4000);
                        break;
                    default:
                        break;
                }
            }
            UpdateVictim();
        }
        private:
            EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_amberpine_woodsmanAI(creature);
    }
};

/*######
## Quest 12288: Overwhelmed!
######*/

enum Skirmisher
{
    // Quest
    QUEST_OVERWHELMED           = 12288,
    // Spell
    SPELL_RENEW_SKIRMISHER      = 48812,
    SPELL_KILL_CREDIT           = 48813,
    // Text
    SAY_RANDOM                  = 0
};

class npc_wounded_skirmisher : public CreatureScript
{
public:
    npc_wounded_skirmisher() : CreatureScript("npc_wounded_skirmisher") { }

    struct npc_wounded_skirmisherAI : public ScriptedAI
    {
        npc_wounded_skirmisherAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            _despawnTimer = 5000;
            _playerGUID = 0;
        }

        void MovementInform(uint32, uint32 id)
        {
            if (id == 1)
                me->DespawnOrUnsummon(_despawnTimer);
        }

        void SpellHit(Unit* caster, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_RENEW_SKIRMISHER && caster->GetTypeId() == TYPEID_PLAYER
                && caster->ToPlayer()->GetQuestStatus(QUEST_OVERWHELMED) == QUEST_STATUS_INCOMPLETE)
            {
                DoCast(caster, SPELL_KILL_CREDIT);
                Talk(SAY_RANDOM, caster);
                if (me->IsStandState())
                    me->GetMotionMaster()->MovePoint(1, me->GetPositionX()+7, me->GetPositionY()+7, me->GetPositionZ());
                else
                {
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    me->DespawnOrUnsummon(_despawnTimer);
                }
            }
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
        private:
            uint64 _playerGUID;
            uint32 _despawnTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wounded_skirmisherAI(creature);
    }
};

/*Venture co. Straggler - when you cast Smoke Bomb, he will yell and run away*/
enum SmokeEmOut
{
    // Quest
    QUEST_SMOKE_EM_OUT_A                        = 12323,
    QUEST_SMOKE_EM_OUT_H                        = 12324,
    // Spell
    SPELL_SMOKE_BOMB                            = 49075,
    SPELL_CHOP                                  = 43410,
    SPELL_VENTURE_STRAGGLER_CREDIT              = 49093,
    // Text
    SAY_SEO                                     = 0
};

enum StragglerEvents
{
    EVENT_STRAGGLER_1                           = 1,
    EVENT_STRAGGLER_2                           = 2,
    EVENT_STRAGGLER_3                           = 3,
    EVENT_STRAGGLER_4                           = 4,
    EVENT_CHOP                                  = 5
};

class npc_venture_co_straggler : public CreatureScript
{
public:
    npc_venture_co_straggler() : CreatureScript("npc_venture_co_straggler") { }

    struct npc_venture_co_stragglerAI : public ScriptedAI
    {
        npc_venture_co_stragglerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            _playerGUID   = 0;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(uint32 diff)
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_STRAGGLER_1:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                             DoCast(player, SPELL_VENTURE_STRAGGLER_CREDIT);
                         me->GetMotionMaster()->MovePoint(0, me->GetPositionX()-7, me->GetPositionY()+7, me->GetPositionZ());
                         _events.ScheduleEvent(EVENT_STRAGGLER_2, 2500);
                         break;
                    case EVENT_STRAGGLER_2:
                         Talk(SAY_SEO);
                         me->GetMotionMaster()->MovePoint(0, me->GetPositionX()-7, me->GetPositionY()-5, me->GetPositionZ());
                         _events.ScheduleEvent(EVENT_STRAGGLER_3, 2500);
                         break;
                    case EVENT_STRAGGLER_3:
                        me->GetMotionMaster()->MovePoint(0, me->GetPositionX()-5, me->GetPositionY()-5, me->GetPositionZ());
                        _events.ScheduleEvent(EVENT_STRAGGLER_4, 2500);
                        break;
                    case EVENT_STRAGGLER_4:
                        me->DisappearAndDie();
                        break;
                    case EVENT_CHOP:
                        if (UpdateVictim())
                            DoCastVictim(SPELL_CHOP);
                        _events.ScheduleEvent(EVENT_CHOP, 10000, 12000);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void SpellHit(Unit* caster, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_SMOKE_BOMB && caster->GetTypeId() == TYPEID_PLAYER)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC);
                me->SetReactState(REACT_PASSIVE);
                me->CombatStop(false);
                _playerGUID = caster->GetGUID();
                _events.ScheduleEvent(EVENT_STRAGGLER_1, 3500);
            }
        }

        private:
            EventMap _events;
            uint64 _playerGUID;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_venture_co_stragglerAI(creature);
        }
};

/*######
## Quest: A Blade Fit For A Champion
######*/

enum LakeFrog
{
    // Creature
    NPC_LAKE_FROG                          = 33211,
    NPC_LAKE_FROG_QUEST                    = 33224,
    NPC_MAIDEN_OF_ASHWOOD_LAKE             = 33220,
    // Items
    ITEM_WARTS_B_GONE_LIP_BALM             = 44986,
    // Spells
    SPELL_WARTSBGONE_LIP_BALM              = 62574,
    SPELL_FROG_LOVE                        = 62537, // for 1 minute !
    SPELL_WARTS                            = 62581,
    SPELL_MAIDEN_OF_ASHWOOD_LAKE_TRANSFORM = 62550,
    SPELL_SUMMON_ASHWOOD_BRAND             = 62554,
    SPELL_FROG_KISS                        = 62536,
    // Text
    SAY_MAIDEN_0                           = 0,
    SAY_MAIDEN_1                           = 1
};

enum LakeFrogEvents
{
    EVENT_LAKEFROG_1                       = 1,
    EVENT_LAKEFROG_2                       = 2,
    EVENT_LAKEFROG_3                       = 3,
    EVENT_LAKEFROG_4                       = 4,
    EVENT_LAKEFROG_5                       = 5
};

class npc_lake_frog : public CreatureScript
{
public:
    npc_lake_frog() : CreatureScript("npc_lake_frog") { }

        struct npc_lake_frogAI : public ScriptedAI
        {
            npc_lake_frogAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _following = false;
                _runningScript = false;
                if (me->GetEntry() == NPC_LAKE_FROG_QUEST)
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            void UpdateAI(uint32 diff)
            {
                if (_following)
                    if (!me->HasAura(SPELL_FROG_LOVE))
                        me->DespawnOrUnsummon(1000);

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LAKEFROG_1:
                            DoCast(me, SPELL_MAIDEN_OF_ASHWOOD_LAKE_TRANSFORM);
                            me->SetEntry(NPC_MAIDEN_OF_ASHWOOD_LAKE);
                            _events.ScheduleEvent(EVENT_LAKEFROG_2, 2000);
                            break;
                        case EVENT_LAKEFROG_2:
                            Talk(SAY_MAIDEN_0);
                            _events.ScheduleEvent(EVENT_LAKEFROG_3, 3000);
                            break;
                        case EVENT_LAKEFROG_3:
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            _events.ScheduleEvent(EVENT_LAKEFROG_4, 25000);
                            break;
                        case EVENT_LAKEFROG_4:
                            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            _events.ScheduleEvent(EVENT_LAKEFROG_5, 2000);
                            break;
                        case EVENT_LAKEFROG_5:
                            Talk(SAY_MAIDEN_1);
                            me->DespawnOrUnsummon(4000);
                            break;
                        default:
                            break;
                    }
                }
            }

            void ReceiveEmote(Player* player, uint32 emote)
            {
                if (_following || _runningScript)
                    return;

                if (emote == TEXT_EMOTE_KISS && me->IsWithinDistInMap(player, 30.0f) && player->HasItemCount(ITEM_WARTS_B_GONE_LIP_BALM, 1, false))
                {
                    if (!player->HasAura(SPELL_WARTSBGONE_LIP_BALM))
                        player->AddAura(SPELL_WARTS, player);
                    else
                    {
                        DoCast(player, SPELL_FROG_KISS); // Removes SPELL_WARTSBGONE_LIP_BALM

                        if (me->GetEntry() == NPC_LAKE_FROG)
                        {
                            me->AddAura(SPELL_FROG_LOVE, me);
                            me->GetMotionMaster()->MoveFollow(player, 0.3f, frand(M_PI/2, M_PI + (M_PI/2)));
                            _following = true;
                        }
                        else if (me->GetEntry() == NPC_LAKE_FROG_QUEST)
                        {
                            me->GetMotionMaster()->MoveIdle();
                            me->SetFacingToObject(player);
                            _runningScript = true;
                            _events.ScheduleEvent(EVENT_LAKEFROG_1, 2000);
                        }
                    }
                }
            }

            void sGossipSelect(Player* player, uint32 /*sender*/, uint32 /*action*/)
            {
                DoCast(player, SPELL_SUMMON_ASHWOOD_BRAND);
            }

        private:
            EventMap _events;
            bool   _following;
            bool   _runningScript;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lake_frogAI(creature);
        }
};

enum ShredderDelivery
{
    NPC_BROKEN_DOWN_SHREDDER               = 27354
};

class spell_shredder_delivery : public SpellScriptLoader
{
    public:
        spell_shredder_delivery() : SpellScriptLoader("spell_shredder_delivery") { }

        class spell_shredder_delivery_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shredder_delivery_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (GetCaster()->ToCreature()->GetEntry() == NPC_BROKEN_DOWN_SHREDDER)
                    GetCaster()->ToCreature()->DespawnOrUnsummon();
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_shredder_delivery_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_shredder_delivery_SpellScript();
        }
};

enum InfectedWorgenBite
{
    SPELL_INFECTED_WORGEN_BITE = 53094,
    SPELL_WORGENS_CALL         = 53095
};

class spell_infected_worgen_bite : public SpellScriptLoader
{
    public:
        spell_infected_worgen_bite() : SpellScriptLoader("spell_infected_worgen_bite") { }
        
        class spell_infected_worgen_bite_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_infected_worgen_bite_AuraScript);
            
            void HandleAfterEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (target->GetTypeId() == TYPEID_PLAYER)
                    if (GetStackAmount() == GetSpellInfo()->StackAmount)
                    {
                        SetDuration(0);
                        target->CastSpell(target, SPELL_WORGENS_CALL, true);
                    }
            }
            
            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_infected_worgen_bite_AuraScript::HandleAfterEffectApply, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAPPLY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_infected_worgen_bite_AuraScript();
        }
};

void AddSC_grizzly_hills()
{
    // Ours
    new npc_riding_the_red_rocket();

    // Theirs
    new npc_emily();
    new npc_mrfloppy();
    new npc_ravenous_worg();
    new npc_outhouse_bunny();
    new npc_tallhorn_stag();
    new npc_amberpine_woodsman();
    new npc_wounded_skirmisher();
    new npc_venture_co_straggler();
    new npc_lake_frog();
    new spell_shredder_delivery();
    new spell_infected_worgen_bite();
}