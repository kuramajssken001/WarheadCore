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
#include "ScriptedEscortAI.h"
#include "shadow_labyrinth.h"

enum eEnums
{
    SAY_INTRO               = 0,
    SAY_AGGRO               = 1,
    SAY_HELP                = 2,
    SAY_SLAY                = 3,
    SAY_DEATH               = 4,

    SPELL_BANISH            = 30231,
    SPELL_CORROSIVE_ACID    = 33551,
    SPELL_FEAR              = 33547,
    SPELL_ENRAGE            = 34970,

    EVENT_SPELL_CORROSIVE   = 1,
    EVENT_SPELL_FEAR        = 2,
    EVENT_SPELL_ENRAGE      = 3
};

class boss_ambassador_hellmaw : public CreatureScript
{
public:
    boss_ambassador_hellmaw() : CreatureScript("boss_ambassador_hellmaw") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_ambassador_hellmawAI(creature);
    }

    struct boss_ambassador_hellmawAI : public npc_escortAI
    {
        boss_ambassador_hellmawAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        bool isBanished;

        void DoAction(int32 param)
        {
            if (param != 1)
                return;

            me->RemoveAurasDueToSpell(SPELL_BANISH);
            Talk(SAY_INTRO);
            Start(true, false, 0, NULL, false, true);
            isBanished = false;
        }

        void Reset()
        {
            events.Reset();
            isBanished = false;
            if (instance)
            {
                instance->SetData(TYPE_HELLMAW, NOT_STARTED);
                if (instance->GetData(TYPE_OVERSEER) != DONE)
                {
                    isBanished = true;
                    me->CastSpell(me, SPELL_BANISH, true);
                }
                else
                    Start(true, false, 0, NULL, false, true);
            }
        }

        void EnterCombat(Unit*)
        {
            if (isBanished)
                return;

            Talk(SAY_AGGRO);

            events.ScheduleEvent(EVENT_SPELL_CORROSIVE, 5s, 10s);
            events.ScheduleEvent(EVENT_SPELL_FEAR, 15s, 20s);

            if (IsHeroic())
                events.ScheduleEvent(EVENT_SPELL_ENRAGE, 3min);

            if (instance)
                instance->SetData(TYPE_HELLMAW, IN_PROGRESS);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (isBanished)
                return;
            npc_escortAI::MoveInLineOfSight(who);
        }

        void AttackStart(Unit* who)
        {
            if (isBanished)
                return;
            npc_escortAI::AttackStart(who);
        }

        void WaypointReached(uint32 /*waypointId*/)
        {
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && urand(0, 1))
                Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            if (instance)
                instance->SetData(TYPE_HELLMAW, DONE);
        }

        void UpdateAI(uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;

            if (isBanished)
            {
                EnterEvadeMode();
                return;
            }

            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_CORROSIVE:
                    me->CastSpell(me->GetVictim(), SPELL_CORROSIVE_ACID, false);
                    events.RepeatEvent(15s, 25s);
                    break;
                case EVENT_SPELL_FEAR:
                    me->CastSpell(me, SPELL_FEAR, false);
                    events.RepeatEvent(20s, 35s);
                    break;
                case EVENT_SPELL_ENRAGE:
                    me->CastSpell(me->GetVictim(), SPELL_ENRAGE, false);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_ambassador_hellmaw()
{
    new boss_ambassador_hellmaw();
}
