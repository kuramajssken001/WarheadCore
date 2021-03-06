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

#ifndef __SAVINGSYSTEM_H
#define __SAVINGSYSTEM_H

#include "Common.h"

// to evenly distribute saving players to db

class SavingSystemMgr
{
public:
    static void Update(uint32 diff);

    static uint32 GetSavingCurrentValue(); // modified only during single thread
    static uint32 GetSavingMaxValue(); // modified only during single thread
    static void IncreaseSavingCurrentValue(uint32 inc); // used and modified only during single thread
    static uint32 IncreaseSavingMaxValue(uint32 inc);
    static void InsertToSavingSkipListIfNeeded(uint32 id);

protected:
    static uint32 m_savingCurrentValue;
    static uint32 m_savingMaxValueAssigned;
    static uint32 m_savingDiffSum;
    static std::list<uint32> m_savingSkipList;
    static ACE_Thread_Mutex _savingLock;
};

#endif
