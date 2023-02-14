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

#ifndef GOFUNCTION_TUPLE_H
#define GOFUNCTION_TUPLE_H

#include "PythonGOEventTypes.hpp"

class GOFunctionTuple
{
public:

	GOFunctionTuple()
	{
		clearFunctions();
	}

	void clearFunctions()
	{
		for( int i = 0; i < PYTHON_GO_EVENT_COUNT; i++ )
		{
			functions[ i ] = NULL;
		}
	}

	bool hasFunction( unsigned long eventType ) const
	{
		if( functions[ eventType ] != NULL )
			return true;
		else
			return false;
	}

	void setFunction( unsigned long eventType, void* function )
	{
		functions[ eventType ] = function;
	}

	void* getFunction( unsigned long eventType ) const
	{
		return functions[ eventType ];
	}

private:
	void* functions[ PYTHON_GO_EVENT_COUNT ];
};

#endif