/***************************************************************************
 *  Copyright 2007-2009 Huy Phan  <huyphan@playxiangqi.com>                *
 *                      Bharatendra Boddu (bharathendra at yahoo dot com)  *
 *                                                                         * 
 *  This file is part of HOXChess.                                         *
 *                                                                         *
 *  HOXChess is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  HOXChess is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with HOXChess.  If not, see <http://www.gnu.org/licenses/>.      *
 ***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// Name:            hoxMyPlayer.cpp
// Created:         10/28/2007
//
// Description:     The new "advanced" LOCAL Player.
/////////////////////////////////////////////////////////////////////////////

#include "hoxMyPlayer.h"
#include "hoxSocketConnection.h"
#include "hoxNetworkAPI.h"
#include "hoxSite.h"
#include "hoxUtil.h"

#include <wx/tokenzr.h>

IMPLEMENT_DYNAMIC_CLASS(hoxMyPlayer, hoxLocalPlayer)

BEGIN_EVENT_TABLE(hoxMyPlayer, hoxLocalPlayer)
    EVT_COMMAND(hoxREQUEST_PLAYER_DATA, hoxEVT_CONNECTION_RESPONSE, hoxMyPlayer::OnConnectionResponse_PlayerData)
    EVT_COMMAND(wxID_ANY, hoxEVT_CONNECTION_RESPONSE, hoxMyPlayer::OnConnectionResponse)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
// hoxMyPlayer
//-----------------------------------------------------------------------------

hoxMyPlayer::hoxMyPlayer( const wxString& name,
                          hoxPlayerType   type,
                          int             score )
            : hoxLocalPlayer( name, type, score )
            , m_bLoginSuccess( false )
{ 
}

void
hoxMyPlayer::Start()
{
    wxASSERT_MSG(m_site, "Site must be set first");
    hoxServerAddress address = m_site->GetAddress();
    hoxConnection_APtr connection( new hoxSocketConnection( address, this ) );
    this->SetConnection( connection );
}

void 
hoxMyPlayer::OnConnectionResponse_PlayerData( wxCommandEvent& event )
{
    hoxResult result = hoxRC_OK;
    const hoxResponse_APtr apResponse( wxDynamicCast(event.GetEventObject(), hoxResponse) );
    hoxTable_SPtr  pTable;

    /* Handle error-code. */

    if ( apResponse->code != hoxRC_OK )
    {
        wxLogDebug("%s: *INFO* Received response-code [%s].", 
            __FUNCTION__, hoxUtil::ResultToStr(apResponse->code));

        /* Close the connection and logout.
         */
        if ( m_bLoginSuccess )
        {
            this->LeaveAllTables();
            this->DisconnectFromServer();
            m_site->OnShutdownReadyFromLocalPlayer();
            wxLogDebug("%s: END (exception).", __FUNCTION__);
            return;  // *** Exit immediately.
        }
    }

    /* Handle other type of data. */

    hoxCommand  command;

    result = hoxNetworkAPI::ParseCommand( apResponse->data, command );
    if ( result != hoxRC_OK )
    {
        wxLogError("%s: Failed to parse command-data.", __FUNCTION__);
        return;
    }

    const wxString sType    = hoxUtil::RequestTypeToString(command.type);
    const wxString sCode    = command.parameters["code"];
    const wxString sTableId = command.parameters["tid"];
    const wxString sContent = command.parameters["content"];

    wxLogDebug("%s: Received a command [%s].", __FUNCTION__, sType.c_str());

    /* Lookup Table if the Table-Id is provided. */
    if ( ! sTableId.empty() )
    {
        pTable = m_site->FindTable( sTableId );
        if ( pTable.get() == NULL )
        {
            wxLogDebug("%s: *INFO* Table [%s] not found.", __FUNCTION__, sTableId.c_str());
            // *** Still allow to continue!
        }
    }

    /* Handle the error-code. */
    if ( sCode != "0" ) // failed?
    {
        const wxString sMessage = "Request " + sType + " failed with code = " + sCode;
        wxLogDebug("%s: %s.", __FUNCTION__, sMessage.c_str());

        if ( pTable.get() != NULL )
        {
            // Post the error on the Board.
            pTable->PostBoardMessage( sMessage );
        }
        // *** Allow to continue
    }

    switch ( command.type )
    {
        case hoxREQUEST_LOGIN:
		{
            _HandleResponseEvent_LOGIN( sCode, sContent, apResponse );
            break;
        }
        case hoxREQUEST_LOGOUT:
        {
            _HandleResponseEvent_LOGOUT( sContent, apResponse );
            break;
        }
        case hoxREQUEST_LIST:
        {
            std::auto_ptr<hoxNetworkTableInfoList> pTableList( new hoxNetworkTableInfoList );
		    result = _ParseNetworkTables( sContent,
					                      *pTableList );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: *WARN* Failed to parse LIST's response [%s].", 
				    __FUNCTION__, sContent.c_str());
			    apResponse->code = result;
		    }
	        m_site->DisplayListOfTables( *pTableList );
            break;
        }
        case hoxREQUEST_I_TABLE:
        {
		    std::auto_ptr<hoxNetworkTableInfo> pTableInfo( new hoxNetworkTableInfo() );
		    result = hoxNetworkAPI::ParseOneNetworkTable( sContent,
													      *pTableInfo );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: *WARN* Failed to parse [%s]'s event [%s].", 
				    __FUNCTION__, sType.c_str(), sContent.c_str());
                break;
		    }
		    m_site->JoinLocalPlayerToTable( *pTableInfo );
		    break;
        }
        case hoxREQUEST_LEAVE:
        {
            hoxPlayer* leavePlayer = NULL;

		    result = _ParsePlayerLeaveEvent( sContent,
										     pTable, leavePlayer );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Table/Player not found. LEAVE's event [%s] ignored.", 
				    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Player [%s] left Table [%s].", __FUNCTION__, 
                leavePlayer->GetId().c_str(), pTable->GetId().c_str());
            pTable->OnLeave_FromNetwork( leavePlayer );
            break;
        }
        case hoxREQUEST_UPDATE:
        {
            hoxPlayer*    player = NULL;
            bool          bRatedGame = true;
            hoxTimeInfo   newTimeInfo;

		    result = _ParseTableUpdateEvent( sContent,
									         pTable, player, bRatedGame, newTimeInfo );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse [%s]'s event [%s].",
                    __FUNCTION__, sType.c_str(), sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Player [%s] updated Timers to [%s] in Table [%s].", __FUNCTION__, 
                player->GetId().c_str(), hoxUtil::TimeInfoToString(newTimeInfo).c_str(),
                pTable->GetId().c_str());
            pTable->OnUpdate_FromPlayer( player,
                                         bRatedGame ? hoxGAME_TYPE_RATED : hoxGAME_TYPE_NONRATED,
                                         newTimeInfo );
            break;
        }
        case hoxREQUEST_E_JOIN:
        {
            wxString      tableId;
            wxString      playerId;
            int           nPlayerScore = 0;
            hoxColor joinColor  = hoxCOLOR_NONE; // Default = observer.

		    result = _ParsePlayerJoinEvent( sContent,
									        tableId, playerId, nPlayerScore, joinColor );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse E_JOIN's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Player [%s] joined Table [%s] as [%d].", __FUNCTION__, 
                playerId.c_str(), tableId.c_str(), joinColor);
            result = m_site->OnPlayerJoined( tableId, 
                                             playerId, 
                                             nPlayerScore,
                                             joinColor );
            if ( result != hoxRC_OK )
            {
                wxLogDebug("%s: *ERROR* Failed to ask table to join as color [%d].", 
                    __FUNCTION__, joinColor);
                break;
            }
            break;
        }
        case hoxREQUEST_MSG:
        {
            wxString      playerId;  // Who sent the message?
            wxString      message;

		    result = _ParsePlayerMsgEvent( sContent,
									       pTable, playerId, message );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse MSG's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Player [%s] sent msg [%s] in Table [%s].", __FUNCTION__, 
                playerId.c_str(), message.c_str(), pTable->GetId().c_str());
            pTable->OnMessage_FromNetwork( playerId, message );
            break;
        }
        case hoxREQUEST_MOVE:
        {
            hoxPlayer*    movePlayer = NULL;
            wxString      sMove;

		    result = _ParsePlayerMoveEvent( sContent,
									        pTable, movePlayer, sMove );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse MOVE's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Player [%s] sent move [%s] in Table [%s].", __FUNCTION__, 
                movePlayer->GetId().c_str(), sMove.c_str(), pTable->GetId().c_str());
            pTable->OnMove_FromNetwork( movePlayer, sMove );
            break;
        }
        case hoxREQUEST_DRAW:
        {
            hoxPlayer*    offerPlayer = NULL;

		    result = _ParsePlayerDrawEvent( sContent,
									        pTable, offerPlayer );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse DRAW's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Inform table of player [%s] offering Draw-Request.", 
                __FUNCTION__, offerPlayer->GetId().c_str());
            pTable->OnDrawRequest_FromNetwork( offerPlayer );
            break;
        }
        case hoxREQUEST_RESET:
        {
		    result = _ParsePlayerResetEvent( sContent,
									         pTable );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Table not found. RESET's event [%s] ignored.", 
				    __FUNCTION__, sContent.c_str());
                break;
		    }

		    wxLogDebug("%s: Received RESET's event [%s].", __FUNCTION__, sContent.c_str());
            pTable->OnGameReset_FromNetwork();
            break;
        }
        case hoxREQUEST_E_END:
        {
            hoxGameStatus gameStatus = hoxGAME_STATUS_UNKNOWN;
            wxString      sReason;

		    result = _ParsePlayerEndEvent( sContent,
									       pTable, gameStatus, sReason );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Table not found. E_END's event [%s] ignored.", 
				    __FUNCTION__, sContent.c_str());
                break;
		    }

            wxLogDebug("%s: The game has ended. Status = [%s]. Reason = [%s]",
                __FUNCTION__, hoxUtil::GameStatusToString( gameStatus ).c_str(), sReason.c_str());
            pTable->OnGameOver_FromNetwork( gameStatus );
            break;
        }
        case hoxREQUEST_E_SCORE:
        {
            hoxPlayer*    player = NULL;
            int           nScore = 0;

		    result = _ParsePlayerScoreEvent( sContent,
									         pTable, player, nScore );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse E_SCORE's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Inform table [%s] of player [%s] new Score [%d].", 
                __FUNCTION__, pTable->GetId().c_str(), player->GetId().c_str(), nScore);
            player->SetScore( nScore );
            pTable->OnScore_FromNetwork( player );
            break;
        }
        case hoxREQUEST_I_MOVES:
        {
            hoxStringList moves;

		    result = _ParsePastMovesEvent( sContent,
									       pTable, moves );
		    if ( result != hoxRC_OK )
		    {
			    wxLogDebug("%s: Failed to parse I_MOVES's event [%s].",
                    __FUNCTION__, sContent.c_str());
                break;
		    }
            wxLogDebug("%s: Inform Table [%s] of past Moves [%s].", __FUNCTION__, 
                pTable->GetId().c_str(), sContent.c_str());
            pTable->OnPastMoves_FromNetwork( this, moves );
            break;
        }
        case hoxREQUEST_INVITE:
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // *** NOTE: The HOXServer does not support INVITATION yet.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            const wxString sMessage =
                wxString::Format("*INVITE: [%s] (NOT YET SUPPORTED!)", sContent.c_str());
            if ( pTable )
            {
                pTable->PostBoardMessage( sMessage );
            }
            else
            {
                ::wxMessageBox( sMessage, _("Invitation from Player"),
                                wxOK | wxICON_INFORMATION );
            }
            break;
        }
        case hoxREQUEST_PLAYER_INFO:
        {
            hoxPlayerStats  playerStats;
		    _ParsePlayerInfoEvent( sContent, playerStats );

            pTable = this->GetActiveTable();

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // *** NOTE: The HOXServer does not support player's statistics yet.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            const wxString sMessage = wxString::Format("*INFO: %s %d W%d D%d L%d",
                playerStats.id.c_str(), playerStats.score,
                playerStats.wins, playerStats.draws, playerStats.losses);
            if ( pTable )
            {
                pTable->PostBoardMessage( sMessage );
            }
            else
            {
                ::wxMessageBox( sMessage, _("Information of Player"),
                                wxOK | wxICON_INFORMATION );
            }
            break;
        }
        default:
        {
		    wxLogDebug("%s: *WARN* Unexpected command [%s].", __FUNCTION__, sType.c_str());
        }
    } // switch()

    wxLogDebug("%s: END.", __FUNCTION__);
}

void 
hoxMyPlayer::OnConnectionResponse( wxCommandEvent& event )
{
    wxLogDebug("%s: ENTER.", __FUNCTION__);

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     * NOTE: Currently, this function exists to handle errors only.
     *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    const hoxResponse_APtr apResponse( wxDynamicCast(event.GetEventObject(), hoxResponse) );

    const wxString sType = hoxUtil::RequestTypeToString(apResponse->type);
    const wxString sContent = apResponse->content;

    switch ( apResponse->type )
    {
        case hoxREQUEST_LOGIN:
		{
            if ( apResponse->code != hoxRC_OK )  // error?
            {
                wxLogDebug("%s: *WARN* Failed to login. Error = [%s].", 
                    __FUNCTION__, sContent.c_str());
                _OnLoginFailure( apResponse );
            }
            break;
        }
        default:
        {
            wxLogDebug("%s: *WARN* Unsupported Request [%s].", __FUNCTION__, sType.c_str());
        }
    }

    wxLogDebug("%s: END.", __FUNCTION__);
}

void 
hoxMyPlayer::_HandleResponseEvent_LOGIN( const wxString&         sCode,
                                         const wxString&         sContent,
                                         const hoxResponse_APtr& apResponse )
{
    /* Error handling. */

    if ( sCode != "0" )  // error?
    {
        wxLogDebug("%s: *WARN* Received LOGIN error = [%s: %s].", 
            __FUNCTION__, sCode.c_str(), sContent.c_str());
        apResponse->code = hoxRC_ERR;
        _OnLoginFailure( apResponse );
        return;
    }

    /* Handle data. */

    wxString  sPlayerId;
	int       nPlayerScore = 0;

    _ParsePlayerLoginEvent( sContent,
						    sPlayerId, nPlayerScore );

    if ( sPlayerId == this->GetId() )
    {
        m_bLoginSuccess = true;  // *** Record this LOGIN event.

        wxLogDebug("%s: Set my score = [%d].", __FUNCTION__, nPlayerScore);
        this->SetScore( nPlayerScore );
        
        apResponse->code = hoxRC_OK;
        m_site->OnResponse_LOGIN( apResponse );
    }
    else
    {
        wxLogDebug("%s: Received LOGIN from other [%s %d].",
            __FUNCTION__, sPlayerId.c_str(), nPlayerScore);
    }

    /* Inform the Site. */
    m_site->OnPlayerLoggedIn( sPlayerId, nPlayerScore );
}

void 
hoxMyPlayer::_HandleResponseEvent_LOGOUT( const wxString&         sContent,
                                          const hoxResponse_APtr& apResponse )
{
    const wxString sPlayerId = sContent.BeforeFirst('\n');
    wxLogDebug("%s: Received LOGOUT from [%s].", __FUNCTION__, sPlayerId.c_str());
    m_site->OnPlayerLoggedOut( sPlayerId );
}

void
hoxMyPlayer::_OnLoginFailure( const hoxResponse_APtr& apResponse )
{
    wxLogDebug("%s: ENTER.", __FUNCTION__);
    m_site->OnResponse_LOGIN( apResponse );

    /* NOTE:
     *   PlayXiangqi server automatically closes the connection
     *   after a login-failure.
     */
    m_site->OnShutdownReadyFromLocalPlayer();
}

void
hoxMyPlayer::_ParsePlayerLoginEvent( const wxString& sContent,
                                     wxString&       playerId,
                                     int&            nPlayerScore )
{
    playerId     = "";
    nPlayerScore = 0;

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: playerId     = token;  break;
            case 1: nPlayerScore = ::atoi( token.c_str() ); break; 
			default: /* Ignore the rest. */ break;
		}
	}		
}

hoxResult
hoxMyPlayer::_ParseNetworkTables( const wxString&          responseStr,
                                   hoxNetworkTableInfoList& tableList )
{
    hoxResult  result = hoxRC_ERR;

    wxLogDebug("%s: ENTER.", __FUNCTION__);

	wxStringTokenizer tkz( responseStr, "\n" );
    hoxNetworkTableInfo tableInfo;

    tableList.clear();
   
	while ( tkz.HasMoreTokens() )
	{
        wxString token = tkz.GetNextToken();

        hoxNetworkAPI::ParseOneNetworkTable(token, tableInfo);
		tableList.push_back( tableInfo );
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerLeaveEvent( const wxString& sContent,
                                     hoxTable_SPtr&  pTable,
                                     hoxPlayer*&     player )
{
    const wxString tableId = sContent.BeforeFirst(';');
    const wxString playerId = sContent.AfterFirst(';');

    pTable.reset();
    player = NULL;

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Lookup Player. */
    player = this->GetSite()->FindPlayer( playerId );
    if ( player == NULL ) 
    {
        wxLogDebug("%s: Player [%s] not found.", __FUNCTION__, playerId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParseTableUpdateEvent( const wxString& sContent,
                                     hoxTable_SPtr&  pTable,
                                     hoxPlayer*&     player,
                                     bool&           bRatedGame,
                                     hoxTimeInfo&    newTimeInfo )
{
    wxString tableId;
    wxString playerId;

    pTable.reset();
    player = NULL;
    bRatedGame = true;

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId = token;  break;
			case 1: playerId = token;  break;
            case 2: bRatedGame = (token == "1");  break;
            case 3: newTimeInfo = hoxUtil::StringToTimeInfo(token); break; 
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Lookup Player. */
    player = this->GetSite()->FindPlayer( playerId );
    if ( player == NULL ) 
    {
        wxLogDebug("%s: Player [%s] not found.", __FUNCTION__, playerId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerJoinEvent( const wxString& sContent,
                                    wxString&       tableId,
                                    wxString&       playerId,
                                    int&            nPlayerScore,
                                    hoxColor&       color)
{
    nPlayerScore = 0;
    color        = hoxCOLOR_NONE; // Default = observer.

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId = token;  break;
			case 1: playerId = token;  break;
            case 2: nPlayerScore = ::atoi( token.c_str() ); break; 
            case 3: color = hoxUtil::StringToColor( token ); break;
			default: /* Ignore the rest. */ break;
		}
	}		

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerMsgEvent( const wxString& sContent,
                                   hoxTable_SPtr&  pTable,
                                   wxString&       playerId,
                                   wxString&       message )
{
    wxString tableId;

    pTable.reset();
    playerId = "";
    message  = "";

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId  = token;  break;
			case 1: playerId = token;  break;
            case 2: message  = token;  break; 
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerMoveEvent( const wxString& sContent,
                                    hoxTable_SPtr&  pTable,
                                    hoxPlayer*&     player,
                                    wxString&       sMove )
{
    wxString tableId;
    wxString playerId;

    pTable.reset();
    player  = NULL;
    sMove   = "";

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId  = token;  break;
			case 1: playerId = token;  break;
            case 2: sMove    = token;  break; 
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Lookup Player. */
    player = this->GetSite()->FindPlayer( playerId );
    if ( player == NULL ) 
    {
        wxLogDebug("%s: Player [%s] not found.", __FUNCTION__, playerId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerDrawEvent( const wxString& sContent,
                                    hoxTable_SPtr&  pTable,
                                    hoxPlayer*&     player )
{
    wxString tableId;
    wxString playerId;

    pTable.reset();
    player  = NULL;

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId    = token;  break;
            case 1: playerId   = token;  break;
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Lookup Player. */
    player = this->GetSite()->FindPlayer( playerId );
    if ( player == NULL ) 
    {
        wxLogDebug("%s: Player [%s] not found.", __FUNCTION__, playerId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerEndEvent( const wxString& sContent,
                                   hoxTable_SPtr&  pTable,
                                   hoxGameStatus&  gameStatus,
                                   wxString&       sReason )
{
    wxString tableId;
    wxString sStatus;  // Game-status.

    pTable.reset();
    gameStatus = hoxGAME_STATUS_UNKNOWN;
    sReason    = "";

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId  = token;  break;
            case 1: sStatus  = token;  break;
            case 2: sReason  = token;  break; 
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Convert game-status from the string ... */
    gameStatus = hoxUtil::StringToGameStatus( sStatus );

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerResetEvent( const wxString& sContent,
                                     hoxTable_SPtr&  pTable )
{
    wxString tableId;

    pTable.reset();

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId  = token;  break;
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePlayerScoreEvent( const wxString& sContent,
                                     hoxTable_SPtr&  pTable,
                                     hoxPlayer*&     player,
                                     int&            nScore )
{
    wxString tableId;
    wxString playerId;

    pTable.reset();
    player  = NULL;
    nScore  = 0;

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId    = token;  break;
            case 1: playerId   = token;  break;
            case 2: nScore     = ::atoi(token.c_str());  break;
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Lookup Player. */
    player = this->GetSite()->FindPlayer( playerId );
    if ( player == NULL ) 
    {
        wxLogDebug("%s: Player [%s] not found.", __FUNCTION__, playerId.c_str());
        return hoxRC_NOT_FOUND;
    }

	return hoxRC_OK;
}

hoxResult
hoxMyPlayer::_ParsePastMovesEvent( const wxString& sContent,
                                   hoxTable_SPtr&  pTable,
                                   hoxStringList&  moves )
{
    wxString tableId;
    wxString sMoves;

    pTable.reset();
    moves.clear();

    /* Parse the input string. */

	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: tableId    = token;  break;
            case 1: sMoves     = token;  break;
			default: /* Ignore the rest. */ break;
		}
	}		

    /* Lookup Table. */
    pTable = this->GetSite()->FindTable( tableId );
    if ( pTable.get() == NULL )
    {
        wxLogDebug("%s: Table [%s] not found.", __FUNCTION__, tableId.c_str());
        return hoxRC_NOT_FOUND;
    }

    /* Parse for the list of Moves. */
    _ParseMovesString( sMoves, moves );

	return hoxRC_OK;
}

void
hoxMyPlayer::_ParsePlayerInfoEvent( const wxString& sContent,
                                    hoxPlayerStats& playerStats )
{
	// ... Do not return empty tokens
	wxStringTokenizer tkz( sContent, ";", wxTOKEN_STRTOK );
	int tokenPosition = 0;
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
		switch ( tokenPosition++ )
		{
			case 0: playerStats.id = token;  break;
            case 1: playerStats.score = ::atoi( token.c_str() ); break;
			default: /* Ignore the rest. */ break;
		}
	}		
}

hoxResult
hoxMyPlayer::_ParseMovesString( const wxString& sMoves,
                                hoxStringList&  moves )
{
	// ... Do not return empty tokens
	wxStringTokenizer tkz( sMoves, "/", wxTOKEN_STRTOK );
	wxString token;

	while ( tkz.HasMoreTokens() )
	{
		token = tkz.GetNextToken();
        moves.push_back( token );
	}		

	return hoxRC_OK;
}

/************************* END OF FILE ***************************************/
