import arcemu
from arcemu import Unit

def npc_onDamageTaken( unit, event, attacker, amount ):
	unit.sendChatMessage( arcemu.CHAT_MSG_MONSTER_SAY, arcemu.LANG_UNIVERSAL, "I am damaged for " + str( amount ) )
	
	
arcemu.RegisterUnitEvent( 113, arcemu.CREATURE_EVENT_ON_DAMAGE_TAKEN, npc_onDamageTaken )