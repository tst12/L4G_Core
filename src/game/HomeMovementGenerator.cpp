/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "WorldPacket.h"

#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

void HomeMovementGenerator<Creature>::Initialize(Creature & owner)
{
    owner.addUnitState(UNIT_STAT_IGNORE_ATTACKERS);

    _setTargetLocation(owner);
}

void HomeMovementGenerator<Creature>::Reset(Creature &)
{
}

void HomeMovementGenerator<Creature>::_setTargetLocation(Creature & owner)
{
    if (owner.hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    Movement::MoveSplineInit init(owner);
    float x, y, z, o;

    // at apply we can select more nice return points base at current move generator
    owner.GetHomePosition(x, y, z, o);

    if (!_path)
        _path = new PathFinder(&owner);

    bool result = _path->calculate(x, y, z, true);

    init.SetFacing(o);

    if (!result || _path->getPathType() & PATHFIND_NOPATH)
    {
        init.MoveTo(x, y, z);
        init.SetWalk(false);
        init.Launch();

        arrived = false;
        owner.clearUnitState(UNIT_STAT_ALL_STATE);
        return;
    }
    
    init.MovebyPath(_path->getPath());
    init.SetWalk(false);
    init.Launch();

    arrived = false;
    owner.clearUnitState(UNIT_STAT_ALL_STATE);
}

bool HomeMovementGenerator<Creature>::Update(Creature &owner, const uint32& time_diff)
{
    arrived = owner.movespline->Finalized();
    return !arrived;
}

void HomeMovementGenerator<Creature>::Finalize(Creature& owner)
{
    owner.clearUnitState(UNIT_STAT_IGNORE_ATTACKERS);

    if (arrived)
    {
        owner.SetWalk(true);
        owner.LoadCreaturesAddon(true);

        owner.SetNoSearchAssistance(false);
        owner.UpdateSpeed(MOVE_RUN, false);

        owner.AI()->JustReachedHome();

        BasicEvent* event = new RestoreReactState(owner);
        if (owner.GetEvents()->HasEventOfType(event))
        {
            delete event;
            return;
        }

        owner.AddEvent(event, sWorld.getConfig(CONFIG_CREATURE_RESTORE_STATE));
    }
}
