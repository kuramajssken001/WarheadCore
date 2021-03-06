/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "blood_furnace.h"

enum eKelidan
{
    SAY_WAKE                    = 0,
    SAY_ADD_AGGRO               = 1,
    SAY_KILL                    = 2,
    SAY_NOVA                    = 3,
    SAY_DIE                     = 4,

    // Keldian spells
    SPELL_CORRUPTION            = 30938,
    SPELL_EVOCATION             = 30935,
    SPELL_FIRE_NOVA             = 33132,
    SPELL_SHADOW_BOLT_VOLLEY    = 28599,
    SPELL_BURNING_NOVA          = 30940,
    SPELL_VORTEX                = 37370,

    // Channelers spells
    SPELL_SHADOW_BOLT               = 12739,
    SPELL_SHADOW_BOLT_H             = 15472,
    SPELL_MARK_OF_SHADOW            = 30937,
    SPELL_CHANNELING                = 39123,

    // Events
    EVENT_SPELL_VOLLEY              = 1,
    EVENT_SPELL_CORRUPTION          = 2,
    EVENT_SPELL_BURNING_NOVA        = 3,
    EVENT_SPELL_FIRE_NOVA           = 4,
    EVENT_SPELL_SHADOW_BOLT         = 5,
    EVENT_SPELL_MARK                = 6,

    // Actions
    ACTION_CHANNELER_ENGAGED        = 1,
    ACTION_CHANNELER_DIED           = 2,
};

const float ShadowmoonChannelers[5][4] =
{
    {302.0f, -87.0f, -24.4f, 0.157f},
    {321.0f, -63.5f, -24.6f, 4.887f},
    {346.0f, -74.5f, -24.6f, 3.595f},
    {344.0f, -103.5f, -24.5f, 2.356f},
    {316.0f, -109.0f, -24.6f, 1.257f}
};

class boss_kelidan_the_breaker : public CreatureScript
{
public:

    boss_kelidan_the_breaker() : CreatureScript("boss_kelidan_the_breaker")
    {
    }

    struct boss_kelidan_the_breakerAI : public ScriptedAI
    {
        boss_kelidan_the_breakerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            memset(&channelers, 0, sizeof(channelers));
        }

        InstanceScript* instance;
        EventMap events;
        uint64 channelers[5];
        uint32 checkTimer;
        bool addYell;

        void Reset()
        {
            addYell = false;
            checkTimer = 5000;

            events.Reset();
            ApplyImmunities(true);
            SummonChannelers();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
            if (instance)
                instance->SetData(DATA_KELIDAN, NOT_STARTED);
        }

        void EnterCombat(Unit*  /*who*/)
        {
            events.ScheduleEvent(EVENT_SPELL_VOLLEY, 1s);
            events.ScheduleEvent(EVENT_SPELL_CORRUPTION, 5s);
            events.ScheduleEvent(EVENT_SPELL_BURNING_NOVA, 15s);

            me->InterruptNonMeleeSpells(false);
            Talk(SAY_WAKE);

            if (instance)
                instance->SetData(DATA_KELIDAN, IN_PROGRESS);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            if (urand(0, 1))
                Talk(SAY_KILL);
        }

        void DoAction(int32 param)
        {
            if (param == ACTION_CHANNELER_ENGAGED)
            {
                if (!addYell)
                {
                    addYell = true;
                    Talk(SAY_ADD_AGGRO);

                    for (uint8 i = 0; i < 5; ++i)
                    {
                        Creature* channeler = ObjectAccessor::GetCreature(*me, channelers[i]);
                        if (channeler && !channeler->IsInCombat())
                            channeler->SetInCombatWithZone();
                    }
                }
            }
            else if (param == ACTION_CHANNELER_DIED)
            {
                for (uint8 i = 0; i < 5; ++i)
                {
                    Creature* channeler = ObjectAccessor::GetCreature(*me, channelers[i]);
                    if (channeler && channeler->IsAlive())
                        return;
                }
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE);
                if (Unit* target = me->SelectNearestPlayer(100.0f))
                    AttackStart(target);
            }
        }

        void CheckChannelers()
        {
            if (addYell)
            {
                if (!SelectTargetFromPlayerList(100.0f))
                    EnterEvadeMode();
                return;
            }

            SummonChannelers();
            for (uint8 i = 0; i < 5; ++i)
            {
                Creature* channeler = ObjectAccessor::GetCreature(*me, channelers[i]);
                if (channeler && !channeler->HasUnitState(UNIT_STATE_CASTING) && !channeler->IsInCombat())
                {
                    Creature* target = ObjectAccessor::GetCreature(*me, channelers[(i + 2) % 5]);
                    if (target)
                        channeler->CastSpell(target, SPELL_CHANNELING, false);
                }
            }
        }

        void SummonChannelers()
        {
            for (uint8 i = 0; i < 5; ++i)
            {
                Creature* channeler = ObjectAccessor::GetCreature(*me, channelers[i]);
                if (channeler && channeler->isDead())
                {
                    channeler->DespawnOrUnsummon(1);
                    channeler = nullptr;
                }
                if (!channeler)
                    channeler = me->SummonCreature(NPC_CHANNELER, ShadowmoonChannelers[i][0], ShadowmoonChannelers[i][1], ShadowmoonChannelers[i][2], ShadowmoonChannelers[i][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 300000);

                channelers[i] = channeler ? channeler->GetGUID() : 0;
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DIE);
            if (instance)
            {
                // Xinef: load grid with start doors
                me->GetMap()->LoadGrid(0, -111.0f);
                instance->SetData(DATA_KELIDAN, DONE);
                instance->HandleGameObject(instance->GetData64(DATA_DOOR1), true);
                instance->HandleGameObject(instance->GetData64(DATA_DOOR6), true);
            }
        }

        void ApplyImmunities(bool apply)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISTRACT, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SHACKLE, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DAZE, apply);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
            {
                checkTimer += diff;
                if (checkTimer >= 5000)
                {
                    checkTimer = 0;
                    CheckChannelers();
                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                        me->CastSpell(me, SPELL_EVOCATION, false);
                }
                return;
            }

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_VOLLEY:
                    me->CastSpell(me, SPELL_SHADOW_BOLT_VOLLEY, false);
                    events.RepeatEvent(8s, 13s);
                    break;
                case EVENT_SPELL_CORRUPTION:
                    me->CastSpell(me, SPELL_CORRUPTION, false);
                    events.RepeatEvent(30s, 50s);
                    break;
                case EVENT_SPELL_BURNING_NOVA:
                    Talk(SAY_NOVA);

                    ApplyImmunities(false);
                    me->AddAura(SPELL_BURNING_NOVA, me);
                    ApplyImmunities(true);

                    if (IsHeroic())
                        DoTeleportAll(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());

                    events.DelayEvents(6s, 0);
                    events.RepeatEvent(25s, 32s);
                    events.ScheduleEvent(EVENT_SPELL_FIRE_NOVA, 5s);
                    break;
                case EVENT_SPELL_FIRE_NOVA:
                    me->CastSpell(me, SPELL_FIRE_NOVA, true);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_kelidan_the_breakerAI(creature);
    }
};

class npc_shadowmoon_channeler : public CreatureScript
{
public:

    npc_shadowmoon_channeler() : CreatureScript("npc_shadowmoon_channeler") {}

    struct npc_shadowmoon_channelerAI : public ScriptedAI
    {
        npc_shadowmoon_channelerAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset()
        {
            events.Reset();
        }

        Creature* GetKelidan()
        {
            if (me->GetInstanceScript())
                return ObjectAccessor::GetCreature(*me, me->GetInstanceScript()->GetData64(DATA_KELIDAN));
            return nullptr;
        }

        void EnterCombat(Unit*  /*who*/)
        {
            if (Creature* kelidan = GetKelidan())
                kelidan->AI()->DoAction(ACTION_CHANNELER_ENGAGED);

            me->InterruptNonMeleeSpells(false);
            events.ScheduleEvent(EVENT_SPELL_SHADOW_BOLT, 1500ms, 3500ms);
            events.ScheduleEvent(EVENT_SPELL_MARK, 5s, 6500ms);
        }

        void JustDied(Unit*  /*killer*/)
        {
            if (Creature* kelidan = GetKelidan())
                kelidan->AI()->DoAction(ACTION_CHANNELER_DIED);
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_SHADOW_BOLT:
                    me->CastSpell(me->GetVictim(), IsHeroic() ? SPELL_SHADOW_BOLT_H : SPELL_SHADOW_BOLT, false);
                    events.RepeatEvent(6s, 7500ms);
                    break;
                case EVENT_SPELL_MARK:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        me->CastSpell(target, SPELL_MARK_OF_SHADOW, false);
                    events.RepeatEvent(16s, 17500ms);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_shadowmoon_channelerAI(creature);
    }
};

void AddSC_boss_kelidan_the_breaker()
{
    new boss_kelidan_the_breaker();
    new npc_shadowmoon_channeler();
}
