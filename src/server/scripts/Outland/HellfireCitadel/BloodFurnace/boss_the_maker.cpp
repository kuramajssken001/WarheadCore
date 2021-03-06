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
#include "blood_furnace.h"

enum eEnums
{
    SAY_AGGRO                   = 0,
    SAY_KILL                    = 1,
    SAY_DIE                     = 2,

    SPELL_ACID_SPRAY            = 38153,
    SPELL_EXPLODING_BREAKER     = 30925,
    SPELL_KNOCKDOWN             = 20276,
    SPELL_DOMINATION            = 25772,

    EVENT_SPELL_ACID                = 1,
    EVENT_SPELL_EXPLODING           = 2,
    EVENT_SPELL_DOMINATION          = 3,
    EVENT_SPELL_KNOCKDOWN           = 4,
};

class boss_the_maker : public CreatureScript
{
public:

    boss_the_maker() : CreatureScript("boss_the_maker")
    {
    }

    struct boss_the_makerAI : public ScriptedAI
    {
        boss_the_makerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.Reset();
            if (!instance)
                return;

            instance->SetData(DATA_THE_MAKER, NOT_STARTED);
            instance->HandleGameObject(instance->GetData64(DATA_DOOR2), true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            events.ScheduleEvent(EVENT_SPELL_ACID, 15s);
            events.ScheduleEvent(EVENT_SPELL_EXPLODING, 6s);
            events.ScheduleEvent(EVENT_SPELL_DOMINATION, 2min);
            events.ScheduleEvent(EVENT_SPELL_KNOCKDOWN, 10s);

            if (!instance)
                return;

            instance->SetData(DATA_THE_MAKER, IN_PROGRESS);
            instance->HandleGameObject(instance->GetData64(DATA_DOOR2), false);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && urand(0, 1))
                Talk(SAY_KILL);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DIE);

            if (!instance)
                return;

            instance->SetData(DATA_THE_MAKER, DONE);
            instance->HandleGameObject(instance->GetData64(DATA_DOOR2), true);
            instance->HandleGameObject(instance->GetData64(DATA_DOOR3), true);

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
                case EVENT_SPELL_ACID:
                    me->CastSpell(me->GetVictim(), SPELL_ACID_SPRAY, false);
                    events.RepeatEvent(15s, 23s);
                    break;
                case EVENT_SPELL_EXPLODING:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        me->CastSpell(target, SPELL_EXPLODING_BREAKER, false);
                    events.RepeatEvent(7s, 11s);
                    break;
                case EVENT_SPELL_DOMINATION:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        me->CastSpell(target, SPELL_DOMINATION, false);
                    events.RepeatEvent(2min);
                    break;
                case EVENT_SPELL_KNOCKDOWN:
                    me->CastSpell(me->GetVictim(), SPELL_KNOCKDOWN, false);
                    events.RepeatEvent(4s, 12s);
                    break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_the_makerAI(creature);
    }
};

void AddSC_boss_the_maker()
{
    new boss_the_maker();
}
