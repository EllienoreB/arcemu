/*
 * ArcEmu MMORPG Server
 * Copyright (C) 2008-2023 <http://www.ArcEmu.org/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"
#include "QuestScriptRegisterer.hpp"
#include "PythonQuestScript.hpp"
#include "PythonQuestScriptFactory.hpp"

void QuestScriptRegisterer::visit( unsigned int questId, QuestFunctionTuple &tuple )
{
	if( !mgr->has_quest_script( questId ) )
	{
		PythonQuestScript *script = PythonQuestScriptFactory::createQuestScript( questId );
		mgr->register_quest_script( questId, script );
	}
}
