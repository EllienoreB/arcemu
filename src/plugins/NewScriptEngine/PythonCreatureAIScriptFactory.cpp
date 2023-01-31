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

#include "PythonCreatureAIScriptFactory.hpp"

#include "PythonCreatureAIScript.hpp"

#include "FunctionRegistry.hpp"


class FactoryManagedPythonCreatureAIScript : public PythonCreatureAIScript
{
public:
	FactoryManagedPythonCreatureAIScript( Creature* src, CreatureFunctionTuple &tuple ) : PythonCreatureAIScript( src, tuple ){}
	
	~FactoryManagedPythonCreatureAIScript()
	{
		PythonCreatureAIScriptFactory::removeScript( _unit->GetProto()->Id );
	}
};


HM_NAMESPACE::HM_HASH_MAP< uint32, CreatureAIScript* > PythonCreatureAIScriptFactory::createdScripts;

CreatureAIScript* PythonCreatureAIScriptFactory::createScript( Creature* src )
{
	uint32 id = src->GetProto()->Id;
	PythonCreatureAIScript* script = NULL;
	
	CreatureFunctionTuple* tuple = FunctionRegistry::getCreatureEventFunctions( id );
	if( tuple != NULL )
	{
		script = new FactoryManagedPythonCreatureAIScript( src, *tuple );
	}
	else
	{
		/// This shouldn't happen
		CreatureFunctionTuple empty;
		script = new FactoryManagedPythonCreatureAIScript( src, empty );
	}

	createdScripts.insert( std::pair< uint32,CreatureAIScript* >( id, script ) );

	return script;
}

void PythonCreatureAIScriptFactory::onReload()
{
	HM_NAMESPACE::HM_HASH_MAP< uint32, CreatureAIScript* >::iterator itr = createdScripts.begin();
	while( itr != createdScripts.end() )
	{
		PythonCreatureAIScript *script = (PythonCreatureAIScript*)itr->second;

		CreatureFunctionTuple* tuple = FunctionRegistry::getCreatureEventFunctions( itr->first );
		if( tuple != NULL )
		{
			script->setFunctions( *tuple );
		}
		else
		{
			/// Removed script from the script files
			CreatureFunctionTuple empty;
			script->setFunctions( empty );
		}
		++itr;
	}
}

void PythonCreatureAIScriptFactory::onShutdown()
{
	createdScripts.clear();
}

void PythonCreatureAIScriptFactory::removeScript( uint32 goId )
{
	HM_NAMESPACE::HM_HASH_MAP< uint32, CreatureAIScript* >::iterator itr = createdScripts.find( goId );
	if( itr != createdScripts.end() )
	{
		createdScripts.erase( itr );
	}
}
