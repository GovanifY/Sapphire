#include <boost/format.hpp>

#include <common/Common.h>
#include <common/Network/CommonNetwork.h>
#include <common/Exd/ExdDataGenerated.h>
#include <common/Network/GamePacketNew.h>
#include <common/Network/PacketContainer.h>
#include <common/Logging/Logger.h>

#include "Network/GameConnection.h"
#include "Network/PacketWrappers/ServerNoticePacket.h"
#include "Network/PacketWrappers/ActorControlPacket142.h"
#include "Network/PacketWrappers/ActorControlPacket143.h"
#include "Network/PacketWrappers/ActorControlPacket144.h"
#include "Network/PacketWrappers/MoveActorPacket.h"

#include "Network/PacketWrappers/PlayerStateFlagsPacket.h"

#include "DebugCommand/DebugCommandHandler.h"

#include "Actor/Player.h"

#include "Action/Action.h"
#include "Action/ActionCast.h"
#include "Action/ActionMount.h"

#include "Script/ScriptMgr.h"

#include "Session.h"
#include "Forwards.h"
#include "Framework.h"

extern Core::Framework g_framework;

using namespace Core::Common;
using namespace Core::Network::Packets;
using namespace Core::Network::Packets::Server;

void Core::Network::GameConnection::skillHandler( const Packets::GamePacket& inPacket,
                                                  Entity::Player& player )
{
    uint8_t type = inPacket.getValAt< uint32_t >( 0x21 );

    uint32_t action = inPacket.getValAt< uint32_t >( 0x24 );
    uint32_t useCount = inPacket.getValAt< uint32_t >( 0x28 );

    uint64_t targetId = inPacket.getValAt< uint64_t >( 0x30 );

    player.sendDebug( "Skill type:" + std::to_string( type ) );

    switch( type )
    {
    case Common::SkillType::Normal:

    if( action < 1000000 ) // normal action
    {
        std::string actionIdStr = boost::str( boost::format( "%|04X|" ) % action );
        player.sendDebug( "---------------------------------------" );
        player.sendDebug( "ActionHandler ( " + actionIdStr + " | " +
                          g_framework.getExdDataGen().get< Core::Data::Action >( action )->name +
                          " | " + std::to_string( targetId ) + " )" );

        player.queuePacket( ActorControlPacket142( player.getId(), ActorControlType::ActionStart, 0x01, action ) );

        if( action == 5 )
        {
            auto currentAction = player.getCurrentAction();

            // we should always have an action here, if not there is a bug
            assert( currentAction );
            currentAction->onStart();
        }
        else
        {
            Core::Entity::ActorPtr targetActor = player.getAsPlayer();
            if( targetId != player.getId() )
            {
                targetActor = player.lookupTargetById( targetId );
            }

            if( !player.actionHasCastTime( action ) )
            {
                g_framework.getScriptMgr().onCastFinish( player, targetActor, action );
            }
            else
            {
                auto pActionCast = Action::make_ActionCast( player.getAsPlayer(), targetActor, action );
                player.setCurrentAction( pActionCast );
                player.sendDebug( "setCurrentAction()" );
                player.getCurrentAction()->onStart();
            }
        }
    }
    else if( action < 2000000 ) // craft action
    {

    }
    else if( action < 3000000 ) // item action
    {
        auto info = g_framework.getExdDataGen().get< Core::Data::EventItem >( action );
        if( info )
        {
            g_framework.getLogger().debug( info->name );
            g_framework.getScriptMgr().onEventItem( player, action, info->quest, info->castTime, targetId );
        }
    }
    else if( action > 3000000 ) // unknown
    {

    }

    break;

    case Common::SkillType::MountSkill:

    player.sendDebug( "Request mount " + std::to_string( action ) );

    auto pActionMount = Action::make_ActionMount( player.getAsPlayer(), action );
    player.setCurrentAction( pActionMount );
    player.sendDebug( "setCurrentAction()" );
    player.getCurrentAction()->onStart();
    
    break;

    }

}
