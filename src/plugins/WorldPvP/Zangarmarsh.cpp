/*
 * ArcEmu MMORPG Server
 * Copyright (C) 2008-2024 <http://www.ArcEmu.org/>
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

#include "plugin.h"

#define ZONE_ZANGARMARSH 3521

#define ZM_BEACON_SCAN_UPDATE_FREQ (2*1000)
#define ZM_BEACON_SCAN_RANGE 100.0f
#define ZM_BEACON_CAPTURE_RANGE 50.0f
#define ZM_BEACON_CAPTURE_PROGRESS_TICK 10

#define ZM_BEACON_CAPTURE_THRESHOLD_ALLIANCE 100
#define ZM_BEACON_CAPTURE_THRESHOLD_HORDE 0
#define ZM_BEACON_CAPTURE_THRESHOLD_NEUTRAL_HI 65
#define ZM_BEACON_CAPTURE_THRESHOLD_NEUTRAL_LO 35

#define ZM_BEACON_OWNER_NEUTRAL 2

enum Beacons
{
	ZM_BEACON_WEST  = 0,
	ZM_BEACON_EAST  = 1,
	ZM_BEACON_COUNT = 2
};

static const char* beaconNames[ ZM_BEACON_COUNT ] = 
{
	"West Beacon",
	"East Beacon"
};

static uint32 beaconOwners[ ZM_BEACON_COUNT ] = { ZM_BEACON_OWNER_NEUTRAL, ZM_BEACON_OWNER_NEUTRAL };

static uint32 beaconCaptureProgress[ ZM_BEACON_COUNT ] = { 50, 50 };

enum ZangarmarshGOs
{
	GO_ZM_BANNER_BEACON_WEST        = 182522,
	GO_ZM_BANNER_BEACON_EAST        = 182523,

	GO_ZM_BANNER_GRAVEYARD_ALLIANCE = 182527,
	GO_ZM_BANNER_GRAVEYARD_HORDE    = 182528,
	GO_ZM_BANNER_GRAVEYARD_NEUTRAL  = 182529
};

enum ZMWorldStates
{
	WORLDSTATE_ZM_PROGRESS_UI_A                 = 2527,
	WORLDSTATE_ZM_PROGRESS_A                    = 2528,
	WORLDSTATE_ZM_PROGRESS_A_NEUTRAL            = 2529,

	WORLDSTATE_ZM_PROGRESS_UI_B                 = 2533,
	WORLDSTATE_ZM_PROGRESS_B                    = 2534,
	WORLDSTATE_ZM_PROGRESS_B_NEUTRAL            = 2535,

	WORLDSTATE_ZM_BEACON_WEST_TOP_ALLIANCE      = 2555,
	WORLDSTATE_ZM_BEACON_WEST_TOP_HORDE         = 2556,
	WORLDSTATE_ZM_BEACON_WEST_TOP_NEUTRAL       = 2557,

	WORLDSTATE_ZM_BEACON_EAST_TOP_ALLIANCE      = 2558,
	WORLDSTATE_ZM_BEACON_EAST_TOP_HORDE         = 2559,
	WORLDSTATE_ZM_BEACON_EAST_TOP_NEUTRAL       = 2560,

	WORLDSTATE_ZM_BEACON_WEST_MAP_ALLIANCE      = 2644,
	WORLDSTATE_ZM_BEACON_WEST_MAP_HORDE         = 2645,
	WORLDSTATE_ZM_BEACON_WEST_MAP_NEUTRAL       = 2646,

	WORLDSTATE_ZM_GRAVEYARD_NEUTRAL             = 2647,
	WORLDSTATE_ZM_GRAVEYARD_ALLIANCE            = 2648,
	WORLDSTATE_ZM_GRAVEYARD_HORDE               = 2649,

	WORLDSTATE_ZM_BEACON_EAST_MAP_ALLIANCE      = 2650,
	WORLDSTATE_ZM_BEACON_EAST_MAP_HORDE         = 2651,
	WORLDSTATE_ZM_BEACON_EAST_MAP_NEUTRAL       = 2652,

	WORLDSTATE_ZM_FIELD_SCOUT_ALLIANCE_ACTIVE   = 2655,
	WORLDSTATE_ZM_FIELD_SCOUT_ALLIANCE_INACTIVE = 2656,
	WORLDSTATE_ZM_FIELD_SCOUT_HORDE_ACTIVE      = 2657,
	WORLDSTATE_ZM_FIELD_SCOUT_HORDE_INACTIVE    = 2658
};

static uint32 beaconTopWorldStates[ ZM_BEACON_COUNT ][ 3 ] = 
{
	{ WORLDSTATE_ZM_BEACON_WEST_TOP_ALLIANCE, WORLDSTATE_ZM_BEACON_WEST_TOP_HORDE, WORLDSTATE_ZM_BEACON_WEST_TOP_NEUTRAL },
	{ WORLDSTATE_ZM_BEACON_EAST_TOP_ALLIANCE, WORLDSTATE_ZM_BEACON_EAST_TOP_HORDE, WORLDSTATE_ZM_BEACON_EAST_TOP_NEUTRAL }
};

static uint32 beaconMapWorldStates[ ZM_BEACON_COUNT ][ 3 ] = 
{
	{ WORLDSTATE_ZM_BEACON_WEST_MAP_ALLIANCE, WORLDSTATE_ZM_BEACON_WEST_MAP_HORDE, WORLDSTATE_ZM_BEACON_WEST_MAP_NEUTRAL },
	{ WORLDSTATE_ZM_BEACON_EAST_MAP_ALLIANCE, WORLDSTATE_ZM_BEACON_EAST_MAP_HORDE, WORLDSTATE_ZM_BEACON_EAST_MAP_NEUTRAL }
};

class ZangarmarshPvP
{
private:
	MapMgr *mgr;

public:
	ZangarmarshPvP()
	{
		mgr = NULL;
	}

	void setMapMgr( MapMgr *mgr )
	{
		this->mgr = mgr;
	}

	void broadcastBeaconCaptureMessage( uint32 beaconId )
	{
		std::string faction;
		if( beaconOwners[ beaconId ] == TEAM_ALLIANCE )
		{
			faction.assign( "The alliance" );
		}
		else
		{
			faction.assign( "The horde" );
		}
		
		std::stringstream ss;
		ss << faction;
		ss << " has captured the " << beaconNames[ beaconId ] << "!";
		
		WorldPacket *packet = sChatHandler.FillMessageData( CHAT_MSG_SYSTEM, LANG_UNIVERSAL, ss.str().c_str(), 0, 0 );
		if( packet != NULL )
		{
			mgr->SendPacketToPlayersInZone( ZONE_ZANGARMARSH, packet );
			delete packet;
		}
	}

	void onBeaconCaptured( uint32 beaconId )
	{
		broadcastBeaconCaptureMessage( beaconId );
	}

	void onBeaconOwnerChange( uint32 beaconId, uint32 lastOwner )
	{
		WorldStatesHandler &handler = mgr->GetWorldStatesHandler();

		switch( beaconOwners[ beaconId ] )
		{
			case TEAM_ALLIANCE:
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_ALLIANCE ], 1 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_HORDE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 0 );

				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_ALLIANCE ], 1 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_HORDE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 0 );
				break;

			case TEAM_HORDE:
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_ALLIANCE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_HORDE ], 1 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 0 );

				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_ALLIANCE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_HORDE ], 1 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 0 );
				break;

			case ZM_BEACON_OWNER_NEUTRAL:
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_ALLIANCE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ TEAM_HORDE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconTopWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 1 );

				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_ALLIANCE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ TEAM_HORDE ], 0 );
				handler.SetWorldStateForZone( ZONE_ZANGARMARSH, beaconMapWorldStates[ beaconId ][ ZM_BEACON_OWNER_NEUTRAL ], 1 );
				break;
		}

		if( beaconOwners[ beaconId ] < ZM_BEACON_OWNER_NEUTRAL )
		{
			onBeaconCaptured( beaconId );
		}
	}
};

ZangarmarshPvP pvp;

class ZangarmarshBeaconBannerAI : public GameObjectAIScript
{
public:
	uint32 beaconId;

	ZangarmarshBeaconBannerAI( GameObject *go ) : GameObjectAIScript( go )
	{
		beaconId = std::numeric_limits<uint32>::max();
	}

	ADD_GAMEOBJECT_FACTORY_FUNCTION( ZangarmarshBeaconBannerAI );

	void onOwnerChange( uint32 lastOwner )
	{
		pvp.onBeaconOwnerChange( beaconId, lastOwner );
	}

	/// Calculate the current capture progress based on player counts
	void calculateProgress( uint32 alliancePlayers, uint32 hordePlayers )
	{
		int32 delta = 0;
		if( alliancePlayers > hordePlayers )
		{
			delta = 1;
		}
		else
		if( hordePlayers > alliancePlayers )
		{
			delta = -1;
		}

		delta *= ZM_BEACON_CAPTURE_PROGRESS_TICK;

		int32 progress = beaconCaptureProgress[ beaconId ];

		if( ( ( progress < 100 ) && ( delta > 0 ) ) ||
			( ( progress > 0 ) && ( delta < 0 ) ) )
		{
			progress += delta;
			progress = Math::clamp< int32 >( progress, 0, 100 );
			beaconCaptureProgress[ beaconId ] = progress;
		}
	}

	/// Calculate the current owner based on the current progress
	/// Returns true on owner change
	bool calculateOwner()
	{
		bool ownerChanged = false;
		if( beaconCaptureProgress[ beaconId ] == ZM_BEACON_CAPTURE_THRESHOLD_ALLIANCE )
		{
			if( beaconOwners[ beaconId ] != TEAM_ALLIANCE )
			{
				ownerChanged = true;
				beaconOwners[ beaconId ] = TEAM_ALLIANCE;
			}
		}
		else
		if( beaconCaptureProgress[ beaconId ] <= ZM_BEACON_CAPTURE_THRESHOLD_HORDE )
		{
			if( beaconOwners[ beaconId ] != TEAM_HORDE )
			{
				ownerChanged = true;
				beaconOwners[ beaconId ] = TEAM_HORDE;
			}
		}
		else
		if( ( beaconCaptureProgress[ beaconId ] <= ZM_BEACON_CAPTURE_THRESHOLD_NEUTRAL_HI ) && ( beaconCaptureProgress[ beaconId ] >= ZM_BEACON_CAPTURE_THRESHOLD_NEUTRAL_LO ) )
		{
			if( beaconOwners[ beaconId ] != ZM_BEACON_OWNER_NEUTRAL )
			{
				ownerChanged = true;
				beaconOwners[ beaconId ] = ZM_BEACON_OWNER_NEUTRAL;
			}
		}

		return ownerChanged;
	}

	void OnSpawn()
	{
		if( _gameobject->GetMapId() != MAP_OUTLAND && _gameobject->GetZoneId() != ZONE_ZANGARMARSH )
		{
			return;
		}

		switch( _gameobject->GetInfo()->ID )
		{
			case GO_ZM_BANNER_BEACON_WEST:
				beaconId = ZM_BEACON_WEST;
				break;

			case GO_ZM_BANNER_BEACON_EAST:
				beaconId = ZM_BEACON_EAST;
				break;
		}

		if( beaconId >= ZM_BEACON_COUNT )
		{
			return;
		}

		RegisterAIUpdateEvent( ZM_BEACON_SCAN_UPDATE_FREQ );
	}

	void AIUpdate()
	{
		std::set< Player* > playersInRange;

		uint32 alliancePlayers = 0;
		uint32 hordePlayers = 0;

		/// Find and count players in range
		std::set< Object* > players = _gameobject->GetInRangePlayers();
		for( std::set< Object* >::const_iterator itr = players.begin(); itr != players.end(); ++itr )
		{
			Player *player = static_cast< Player* >( *itr );

			float d = _gameobject->CalcDistance( player );

			/// If player is outside the "scan range" then we shouldn't even care about him
			/// We need this because the forts are too close to each others, so a player can be in range of more than 1 of them
			if( d > ZM_BEACON_SCAN_RANGE )
			{
				continue;
			}

			if( d > ZM_BEACON_CAPTURE_RANGE )
			{
				/// If player is outside the capture range, turn off the capture UI
				Messenger::SendWorldStateUpdate( player, WORLDSTATE_ZM_PROGRESS_UI_A, 0 );
			}
			else
			{
				if( player->isAlive() && !player->IsInvisible() && !player->IsStealth() && player->IsPvPFlagged() )
				{
					if( player->GetTeam() == TEAM_ALLIANCE )
					{
						alliancePlayers++;
					}
					else
					{
						hordePlayers++;
					}
				}

				playersInRange.insert( player );
			}
		}

		if( playersInRange.empty() )
		{
			/// No player in range, no point in calculating
			return;
		}

		calculateProgress( alliancePlayers, hordePlayers );
		uint32 lastOwner = beaconOwners[ beaconId ];
		bool ownerChanged = calculateOwner();

		/// Send progress to players fighting for control
		for( std::set< Player* >::const_iterator itr = playersInRange.begin(); itr != playersInRange.end(); ++itr )
		{
			Player *player = *itr;

			Messenger::SendWorldStateUpdate( player, WORLDSTATE_ZM_PROGRESS_UI_A, 1 );
			Messenger::SendWorldStateUpdate( player, WORLDSTATE_ZM_PROGRESS_A, beaconCaptureProgress[ beaconId ] );
		}

		if( ownerChanged )
		{
			onOwnerChange( lastOwner );
		}

		playersInRange.clear();
	}

};

void setupZangarmarsh( ScriptMgr *mgr )
{
	MapMgr *mapMgr = sInstanceMgr.GetMapMgr( MAP_OUTLAND );
	pvp.setMapMgr( mapMgr );

	mgr->register_gameobject_script( GO_ZM_BANNER_BEACON_WEST, &ZangarmarshBeaconBannerAI::Create );
	mgr->register_gameobject_script( GO_ZM_BANNER_BEACON_EAST, &ZangarmarshBeaconBannerAI::Create );
}