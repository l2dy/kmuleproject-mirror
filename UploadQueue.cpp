//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "UploadQueue.h"
#include "Packets.h"
#include "KnownFile.h"
#include "ListenSocket.h"
#include "Exceptions.h"
#include "UploadBandwidthThrottler.h"
#include "ClientList.h"
#include "LastCommonRouteFinder.h"
#include "DownloadQueue.h"
#include "FriendList.h"
#include "Statistics.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "ClientCredits.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "SearchDlg.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "Log.h"
#include "./Mod/ClientAnalyzer.h" //>>> WiZaRd::ClientAnalyzer

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT counter, sec, statsave;
static UINT s_uSaveStatistics = 0;
static UINT igraph, istats, i2Secs;

#define HIGHSPEED_UPLOADRATE_START 500*1024
#define HIGHSPEED_UPLOADRATE_END   300*1024

CUploadQueue::CUploadQueue()
{
    h_timer = NULL;
    datarate = 0;
    counter=0;
    successfullupcount = 0;
    failedupcount = 0;
    totaluploadtime = 0;
    m_nLastStartUpload = 0;
    statsave=0;
    i2Secs=0;
    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;
    m_MaxActiveClients = 0;
    m_MaxActiveClientsShortTime = 0;

    m_lastCalculatedDataRateTick = 0;
    m_avarage_dr_sum = 0;
    friendDatarate = 0;

    m_dwLastResortedUploadSlots = 0;
    m_hHighSpeedUploadTimer = NULL;
    m_bStatisticsWaitingListDirty = true;
    m_uiPaybackCount = 0; //>>> WiZaRd::Payback First
//>>> WiZaRd::ZZUL Upload [ZZ]
	m_dwLastCheckedForHighPrioClient = 0;

	m_dwLastCalculatedAverageCombinedFilePrioAndCredit = 0;
	m_fAverageCombinedFilePrioAndCredit = 0;
//<<< WiZaRd::ZZUL Upload [ZZ]
}

void	CUploadQueue::StartTimer()
{
    if(h_timer == NULL)
    {
        VERIFY( (h_timer = SetTimer(0,0,100,UploadTimer)) != NULL );
        if (thePrefs.GetVerbose() && !h_timer)
            AddDebugLogLine(true, L"Failed to create 'upload queue' timer - %s", GetErrorMessage(GetLastError()));
    }
    else
        ASSERT(0);
}

CUploadQueue::~CUploadQueue()
{
    if (h_timer)
        KillTimer(0,h_timer);
    if (m_hHighSpeedUploadTimer)
        UseHighSpeedUploadTimer(false);
}

//>>> WiZaRd::Payback First
INT64	GetUpDownDiff(const CUpDownClient* client)
{
    if(client->Credits() == NULL)
        return _I64_MIN;
    return  client->Credits()->GetDownloadedTotal()-client->Credits()->GetUploadedTotal();
}

BOOL IsPayback(const CUpDownClient* client)
{
    return (!client->GetRequestFile() || client->GetDownloadState() > DS_ONQUEUE)
           && GetUpDownDiff(client) > (SESSIONMAXTRANS/2);
}

BOOL SecondIsBetterClient(const bool bPaybackCheck, const CUpDownClient* first, const CUpDownClient* second, const UINT firstscore, const UINT secondscore)
{
    if(second == NULL)
        return FALSE;
    if(first == NULL)
        return TRUE;

    if(bPaybackCheck)
    {
        const BOOL bPay1 = IsPayback(first);
        const BOOL bPay2 = IsPayback(second);
        if(bPay1 && !bPay2)
            return FALSE;	//first is better
        if(!bPay1 && bPay2)
            return TRUE;	//second is better
        else if(bPay1 && bPay2)
            return GetUpDownDiff(second) > GetUpDownDiff(first); //second or first?
        //flow over
    }

    return (secondscore > firstscore);
}
//<<< WiZaRd::Payback First

/**
 * Find the highest ranking client in the waiting queue, and return it.
 *
 * Low id client are ranked as lowest possible, unless they are currently connected.
 * A low id client that is not connected, but would have been ranked highest if it
 * had been connected, gets a flag set. This flag means that the client should be
 * allowed to get an upload slot immediately once it connects.
 *
 * @return address of the highest ranking client.
 */
//>>> WiZaRd::ZZUL Upload [ZZ]
CUpDownClient* CUploadQueue::FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient)
//CUpDownClient* CUploadQueue::FindBestClientInQueue()
//<<< WiZaRd::ZZUL Upload [ZZ]
{
//>>> WiZaRd::PowerShare
    CUpDownClient* toadd = NULL;
    CUpDownClient* toaddlow = NULL;
    UINT	bestscore = 0;
    UINT  bestlowscore = 0;

    CUpDownClient* friendpos = NULL;
    CUpDownClient* toaddPS = NULL;
    CUpDownClient* toaddlowPS = NULL;
    UINT	bestscorePS = 0;
    UINT	bestlowscorePS = 0;
    uint8	highestPSPrio = 0;

//>>> WiZaRd::Payback First
    bool bPaybackCheck = m_lPaybackList.IsEmpty()
                         || m_uiPaybackCount == 5;;
    /*switch(thePrefs.GetPaybackFirst())
    {
    	default:
    	case 0: //off
    		break;

    	case 1: //on - one slot
    		bPaybackCheck = m_lPaybackList.IsEmpty() ? true : false;
    		break;

    	case 2: //on - once every X slots
    		bPaybackCheck = m_uiPaybackCount == 5;
    		break;

    	case 3: //auto - TODO?
    		bPaybackCheck = m_lPaybackList.IsEmpty()
    			|| m_uiPaybackCount == 5;
    		break;
    }*/
//<<< WiZaRd::Payback First

    POSITION pos1, pos2;
    for (pos1 = waitinglist.GetHeadPosition(); ( pos2 = pos1 ) != NULL;)
    {
        waitinglist.GetNext(pos1);
        CUpDownClient* cur_client =	waitinglist.GetAt(pos2);
        CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
        //While we are going through this list.. Lets check if a client appears to have left the network..
        ASSERT ( cur_client->GetLastUpRequest() );
//>>> WiZaRd::FiX unfair client treatment [http://forum.emule-project.net/index.php?showtopic=116699]
        //if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !file )
        if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME))
//<<< WiZaRd::FiX unfair client treatment [http://forum.emule-project.net/index.php?showtopic=116699]
        {
            //This client has either not been seen in a long time, or we no longer share the file he wanted anymore..
            //cur_client->ClearWaitStartTime(); //>>> WiZaRd::ZZUL Upload [ZZ]
            RemoveFromWaitingQueue(pos2,true);
            continue;
        }

//>>> WiZaRd::FiX unfair client treatment [http://forum.emule-project.net/index.php?showtopic=116699]
        if(file == NULL)
            continue;
//<<< WiZaRd::FiX unfair client treatment [http://forum.emule-project.net/index.php?showtopic=116699]
//>>> WiZaRd::Startup Flood Prevention
        //Requested by James R. Bath
        //http://forum.emule-project.net/index.php?showtopic=101181
        if(::GetTickCount() - cur_client->GetWaitStartTime() < SEC2MS(30)) //30secs are enough
            continue;
//<<< WiZaRd::Startup Flood Prevention

        if(cur_client->GetFriendSlot()
                && (!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected())))
            friendpos = cur_client;
        if(friendpos)
            continue; //no need to go on...

        // finished clearing
        const UINT cur_score = cur_client->GetScore(false);

        if (file->IsPowerShared())
        {
            uint8 curprio = file->GetUpPriority()+1;
            if(curprio >= 5)
                curprio = 0;
            if(curprio > highestPSPrio
//>>> WiZaRd::Payback First
//					|| (curprio == highestPSPrio && cur_score > bestscorePS))
                    || (curprio == highestPSPrio && SecondIsBetterClient(bPaybackCheck, toaddPS, cur_client, bestscorePS, cur_score)))
//<<< WiZaRd::Payback First
            {
                //if our upload tries failed at least 3 times in a row we will only grant a slot while being connected already
                //that way, we hopefully ensure a successful upload session
                if(cur_client->GetAntiLeechData()
                        && cur_client->GetAntiLeechData()->ShouldntUploadForBadSessions())
                {
                    if(cur_client->socket && cur_client->socket->IsConnected())
                    {
                        highestPSPrio = curprio;
                        if(curprio > highestPSPrio)
                        {
                            bestscorePS = 0;
                            bestlowscorePS = 0;
                        }

                        bestscorePS = cur_score;
                        toaddPS = cur_client;
                    }
                    continue;
                }

                highestPSPrio = curprio;
                if(curprio > highestPSPrio)
                {
                    bestscorePS = 0;
                    bestlowscorePS = 0;
                }

                // cur_client is more worthy than current best client that is ready to go (connected).
                if(!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected()))
                {
                    // this client is a HighID or a lowID client that is ready to go (connected)
                    // and it is more worthy
                    bestscorePS = cur_score;
                    toaddPS = cur_client;
                }
//>>> WiZaRd::ZZUL Upload [ZZ]
                //else if(!cur_client->m_bAddNextConnect)
				else if(allowLowIdAddNextConnectToBeSet && !cur_client->m_bAddNextConnect)
//<<< WiZaRd::ZZUL Upload [ZZ]
                {
                    // this client is a lowID client that is not ready to go (not connected)

                    // now that we know this client is not ready to go, compare it to the best not ready client
                    // the best not ready client may be better than the best ready client, so we need to check
                    // against that client
//>>> WiZaRd::Payback First
                    if(SecondIsBetterClient(bPaybackCheck, toaddlowPS, cur_client, bestlowscorePS, cur_score))
//						if (cur_score > bestlowscorePS)
//<<< WiZaRd::Payback First
                    {
                        // it is more worthy, keep it
                        bestlowscorePS = cur_score;
                        toaddlowPS = cur_client;
                    }
                }
            }
        }
        else
        {
//>>> WiZaRd::Payback First
            if(SecondIsBetterClient(bPaybackCheck, toadd, cur_client, bestscore, cur_score))
//				if (cur_score > bestscore)
//<<< WiZaRd::Payback First
            {
                //if our upload tries failed at least 3 times in a row we will only grant a slot while being connected already
                //that way, we hopefully ensure a successful upload session
                if(cur_client->GetAntiLeechData()
                        && cur_client->GetAntiLeechData()->ShouldntUploadForBadSessions())
                {
                    if(cur_client->socket && cur_client->socket->IsConnected())
                    {
                        bestscore = cur_score;
                        toadd = cur_client;
                    }
                    continue;
                }

                // cur_client is more worthy than current best client that is ready to go (connected).
                if(!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected()))
                {
                    // this client is a HighID or a lowID client that is ready to go (connected)
                    // and it is more worthy
                    bestscore = cur_score;
                    toadd = cur_client;
                    //newclient = waitinglist.GetAt(toadd);
                }
//>>> WiZaRd::ZZUL Upload [ZZ]
                //else if(!cur_client->m_bAddNextConnect)
				else if(allowLowIdAddNextConnectToBeSet && !cur_client->m_bAddNextConnect)
//<<< WiZaRd::ZZUL Upload [ZZ]	
                {
                    // this client is a lowID client that is not ready to go (not connected)

                    // now that we know this client is not ready to go, compare it to the best not ready client
                    // the best not ready client may be better than the best ready client, so we need to check
                    // against that client
//>>> WiZaRd::Payback First
                    if(SecondIsBetterClient(bPaybackCheck, toaddlow, cur_client, bestlowscore, cur_score))
//						if (cur_score > bestlowscore)
//<<< WiZaRd::Payback First
                    {
                        // it is more worthy, keep it
                        bestlowscore = cur_score;
                        toaddlow = cur_client;
                        //lowclient = waitinglist.GetAt(toaddlow);
                    }
                }
            }
        }
    }
//>>> WiZaRd::ZZUL Upload [ZZ]	
	// TODO: apply ZZUL restrictions
	if(allowLowIdAddNextConnectToBeSet)
//<<< WiZaRd::ZZUL Upload [ZZ]	
	{
//>>> WiZaRd::Payback First
//		if (bestlowscorePS > bestscorePS && toaddlowPS)
		if (toaddlowPS && SecondIsBetterClient(bPaybackCheck, toaddPS, toaddlowPS, bestscorePS, bestlowscorePS))
//<<< WiZaRd::ZZUL Upload [ZZ]
//<<< WiZaRd::Payback First
			toaddlowPS->m_bAddNextConnect = true;
//>>> WiZaRd::Payback First
//		else if (bestlowscore > bestscore && toaddlow)
		else if (toaddlow && SecondIsBetterClient(bPaybackCheck, toadd, toaddlow, bestscore, bestlowscore))
//<<< WiZaRd::Payback First
			toaddlow->m_bAddNextConnect = true;
	}
    if(friendpos)
        return friendpos;
    if(toaddPS)
        return toaddPS;
    return toadd;
}

//>>> WiZaRd::PowerShare //>>> WiZaRd::SlotFocus
uint8 GetClass(const CUpDownClient* client)
{
    if(client->GetFriendSlot())
        return 0;
    if(client->IsPowerShared())
        return 1;
    return 2;
}
//<<< WiZaRd::PowerShare //<<< WiZaRd::SlotFocus
//>>> WiZaRd::ZZUL Upload [ZZ]
// void CUploadQueue::InsertInUploadingList(CUpDownClient* newclient)
// {
// //>>> WiZaRd::Payback First
//     if(IsPayback(newclient))
//         m_lPaybackList.AddTail(newclient);
// //<<< WiZaRd::Payback First
// 
// //>>> WiZaRd::PowerShare //>>> WiZaRd::SlotFocus
// //>>> USC
//     POSITION insertPosition = NULL;
//     UINT posCounter = uploadinglist.GetCount();
//     const uint8 classID = GetClass(newclient);
//     const UINT score = newclient->GetScore(false, newclient->IsDownloading());
// 
//     for(POSITION pos = uploadinglist.GetTailPosition(); pos;)
//     {
//         CUpDownClient* uploadingClient = uploadinglist.GetAt(pos);
//         const uint8 thisClassID = GetClass(uploadingClient);
// 
//         //check if the uploadingClient is "better" than the newclient and break - or update the insertPosition accordingly
//         if(thisClassID < classID) //lower is better
//             break; //quit it
// 
//         if(thisClassID == classID)
//         {
// //>>> WiZaRd::Drop Blocking Sockets [Xman?]
// 			//try to put "good" clients to the top... "good" means that they do not block too much :)
// 			//put scheduled clients at the end of the uploadlist
// 			//if(newclient->IsScheduled() && !uploadingClient->IsScheduled())
// 			//	break; //quit it
// 			//OR if he is blocking too much because that may indicate that the client (currently) cannot take enough
// 			if(uploadingClient->socket
// 				&& newclient->socket
// 				//remove the 1min limit because slots will jump too much if we keep this
// 				//				&& uploadingClient->GetUpStartTimeDelay() > MIN2MS(1)
// 				//				&& newclient->GetUpStartTimeDelay() > MIN2MS(1)
// 				&& uploadingClient->GetUpStartTimeDelay() > SEC2MS(15)
// 				&& newclient->GetUpStartTimeDelay() > SEC2MS(15)
// 				&& uploadingClient->socket->GetOverallBlockingRatio() <= newclient->socket->GetOverallBlockingRatio())
// 				break; //quit it
// //<<< WiZaRd::Drop Blocking Sockets [Xman?]
// 
// 			const UINT thisScore = uploadingClient->GetScore(false, true);
// 			if(thisScore > score) //higher is better
// 			    break; //quit it
//         }
//         		
//         uploadingClient->SetSlotNumber(posCounter+1);
//         insertPosition = pos; //the new client is "better" than the current client - save the position of the last "worse" client
//         uploadinglist.GetPrev(pos);
//         --posCounter;
//     }
// 
//     //Lets make sure any client that is added to the list has this flag reset!
//     newclient->m_bAddNextConnect = false;
// 
//     theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->GetFileUploadSocket());
//     if(insertPosition)
//         uploadinglist.InsertBefore(insertPosition, newclient); // add it at found pos
//     else
//         uploadinglist.AddTail(newclient); // add it last
//     newclient->SetSlotNumber(posCounter+1);
//     /*
//     	//Lets make sure any client that is added to the list has this flag reset!
//     	newclient->m_bAddNextConnect = false;
//         // Add it last
//         theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket());
//     	uploadinglist.AddTail(newclient);
//         newclient->SetSlotNumber(uploadinglist.GetCount());
//     */
// //<<< WiZaRd::PowerShare //<<< WiZaRd::SlotFocus
// }

/*
bool CUploadQueue::AddUpNextClient(LPCTSTR pszReason, CUpDownClient* directadd)
{
    CUpDownClient* newclient = NULL;
    // select next client or use given client
    if (!directadd)
    {
        newclient = FindBestClientInQueue();

        if(newclient)
        {
            RemoveFromWaitingQueue(newclient, true);
            theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
        }
    }
    else
        newclient = directadd;

    if(newclient == NULL)
        return false;

    if (IsDownloading(newclient))
        return false;

//>>> WiZaRd::Optimization
    CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
    if (reqfile == NULL)
    {
        if(thePrefs.GetLogUlDlEvents())
            AddDebugLogLine(false, L"Tried to add client %s to upload list but reqfile wasn't found - prevented failed UL session!", newclient->DbgGetClientInfo());
        return false;
    }
//<<< WiZaRd::Optimization

    if(pszReason && thePrefs.GetLogUlDlEvents())
        AddDebugLogLine(false, _T("Adding client to upload list: %s Client: %s"), pszReason, newclient->DbgGetClientInfo());

    if(directadd == NULL && reqfile->GetFileSize() < theApp.uploadqueue->GetSmallFileSize()) //>>> WiZaRd::Small File Slot
        ASSERT( false );

    // tell the client that we are now ready to upload
    if (!newclient->socket || !newclient->socket->IsConnected())
    {
        newclient->SetUploadState(US_CONNECTING);
        if (!newclient->TryToConnect(true))
            return false;
    }
    else
    {
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__AcceptUploadReq", newclient);
        Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
        theStats.AddUpDataOverheadFileRequest(packet->size);
        newclient->SendPacket(packet, true);
        newclient->SetUploadState(US_UPLOADING);
    }
    newclient->SetUpStartTime();
    newclient->ResetSessionUp();

    InsertInUploadingList(newclient);

    m_nLastStartUpload = ::GetTickCount();

    // statistic
    reqfile->statistic.AddAccepted();

    theApp.emuledlg->transferwnd->GetUploadList()->AddClient(newclient);

    return true;
}
*/
//<<< WiZaRd::ZZUL Upload [ZZ]

void CUploadQueue::UpdateActiveClientsInfo(DWORD curTick)
{
    // Save number of active clients for statistics
    UINT tempHighest = theApp.uploadBandwidthThrottler->GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset();

    if(thePrefs.GetLogUlDlEvents() && theApp.uploadBandwidthThrottler->GetStandardListSize() > (UINT)uploadinglist.GetSize())
    {
        // debug info, will remove this when I'm done.
        //AddDebugLogLine(false, _T("UploadQueue: Error! Throttler has more slots than UploadQueue! Throttler: %i UploadQueue: %i Tick: %i"), theApp.uploadBandwidthThrottler->GetStandardListSize(), uploadinglist.GetSize(), ::GetTickCount());
    }

    if(tempHighest > (UINT)uploadinglist.GetSize()+1)
    {
        tempHighest = uploadinglist.GetSize()+1;
    }

    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = tempHighest;

    // save some data about number of fully active clients
    UINT tempMaxRemoved = 0;
//>>> WiZaRd::ZZUL Upload [ZZ]
    //while(!activeClients_tick_list.IsEmpty() && !activeClients_list.IsEmpty() && curTick-activeClients_tick_list.GetHead() > 20*1000)
	while(!activeClients_tick_list.IsEmpty() && !activeClients_list.IsEmpty() && curTick-activeClients_tick_list.GetHead() > 2*60*1000)
//<<< WiZaRd::ZZUL Upload [ZZ]
    {
        activeClients_tick_list.RemoveHead();
        UINT removed = activeClients_list.RemoveHead();

        if(removed > tempMaxRemoved)
        {
            tempMaxRemoved = removed;
        }
    }

    activeClients_list.AddTail(m_iHighestNumberOfFullyActivatedSlotsSinceLastCall);
    activeClients_tick_list.AddTail(curTick);

    if(activeClients_tick_list.GetSize() > 1)
    {
        UINT tempMaxActiveClients = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        UINT tempMaxActiveClientsShortTime = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        POSITION activeClientsTickPos = activeClients_tick_list.GetTailPosition();
        POSITION activeClientsListPos = activeClients_list.GetTailPosition();
        while(activeClientsListPos != NULL && (tempMaxRemoved > tempMaxActiveClients && tempMaxRemoved >= m_MaxActiveClients || curTick - activeClients_tick_list.GetAt(activeClientsTickPos) < 10 * 1000))
        {
            DWORD activeClientsTickSnapshot = activeClients_tick_list.GetAt(activeClientsTickPos);
            UINT activeClientsSnapshot = activeClients_list.GetAt(activeClientsListPos);

            if(activeClientsSnapshot > tempMaxActiveClients)
            {
                tempMaxActiveClients = activeClientsSnapshot;
            }

            if(activeClientsSnapshot > tempMaxActiveClientsShortTime && curTick - activeClientsTickSnapshot < 10 * 1000)
            {
                tempMaxActiveClientsShortTime = activeClientsSnapshot;
            }

            activeClients_tick_list.GetPrev(activeClientsTickPos);
            activeClients_list.GetPrev(activeClientsListPos);
        }

        if(tempMaxRemoved >= m_MaxActiveClients || tempMaxActiveClients > m_MaxActiveClients)
        {
            m_MaxActiveClients = tempMaxActiveClients;
        }

        m_MaxActiveClientsShortTime = tempMaxActiveClientsShortTime;
    }
    else
    {
        m_MaxActiveClients = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        m_MaxActiveClientsShortTime = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
    }
}

/**
 * Maintenance method for the uploading slots. It adds and removes clients to the
 * uploading list. It also makes sure that all the uploading slots' Sockets always have
 * enough packets in their queues, etc.
 *
 * This method is called approximately once every 100 milliseconds.
 */
void CUploadQueue::Process()
{

    DWORD curTick = ::GetTickCount();

    UpdateActiveClientsInfo(curTick);

//>>> WiZaRd::ZZUL Upload [ZZ]
	CheckForHighPrioClient();

	if(::GetTickCount()-m_nLastStartUpload > SEC2MS(20) && GetEffectiveUploadListCount() > 0 && GetEffectiveUploadListCount() > m_MaxActiveClientsShortTime+GetWantedNumberOfTrickleUploads() && AcceptNewClient(GetEffectiveUploadListCount()-1) == false) 
	{
		// we need to close a trickle slot and put it back first on the queue
		POSITION lastpos = uploadinglist.GetTailPosition();

		CUpDownClient* lastClient = NULL;
		if(lastpos != NULL)
			lastClient = uploadinglist.GetAt(lastpos);

		if(lastClient != NULL && !lastClient->IsScheduledForRemoval() /*lastClient->GetUpStartTimeDelay() > 3*1000*/) 
		{
			// There's too many open uploads (propably due to the user changing
			// the upload limit to a lower value). Remove the last opened upload and put
			// it back on the waitinglist. When it is put back, it get
			// to keep its waiting time. This means it is likely to soon be
			// choosen for upload again.

			// Remove from upload list.
			ScheduleRemovalFromUploadQueue(lastClient, _T("Too many upload slots opened for current ul speed"), GetResString(IDS_UPLOAD_TOO_MANY_SLOTS), true /*, true*/);

			// add to queue again.
			// the client is allowed to keep its waiting position in the queue, since it was pre-empted
			//AddClientToQueue(lastClient,true, true);

			m_nLastStartUpload = ::GetTickCount();
		}
	} 
	else 
//<<< WiZaRd::ZZUL Upload [ZZ]
    if (ForceNewClient())
    {
        // There's not enough open uploads. Open another one.
        AddUpNextClient(_T("Not enough open upload slots for current ul speed"));
    }

    ReSortUploadSlots(); //>>> WiZaRd::PowerShare //>>> WiZaRd::SlotFocus

//>>> WiZaRd::Drop Blocking Sockets [Xman?]
	//count the blocked send to remove such clients if needed
	//there are many clients out there, which can't take a high slot speed (often less than 1kbs)
	//in worst case out upload is full of them and because no new slots are opened
	//our overall upload decreases
	//why should we keep such bad clients? we keep it only if we have enough slots left
	//if our slot-max is reached, we drop the most blocking client
	float maxblock = 0.0f;
	CUpDownClient* blockclient = NULL;
//<<< WiZaRd::Drop Blocking Sockets [Xman?]

    // The loop that feeds the upload slots with data.
    POSITION pos = uploadinglist.GetHeadPosition();
    while(pos != NULL)
    {
        // Get the client. Note! Also updates pos as a side effect.
        CUpDownClient* cur_client = uploadinglist.GetNext(pos);
        if (thePrefs.m_iDbgHeap >= 2)
            ASSERT_VALID(cur_client);
        //It seems chatting or friend slots can get stuck at times in upload.. This needs looked into..
        if (!cur_client->socket)
        {
            RemoveFromUploadQueue(cur_client, _T("Uploading to client without socket? (CUploadQueue::Process)"));
            if(cur_client->Disconnected(_T("CUploadQueue::Process")))
                delete cur_client;
        }
        else
        {
//>>> WiZaRd::ZZUL Upload [ZZ]
			if(!cur_client->IsScheduledForRemoval() || ::GetTickCount()-m_nLastStartUpload <= SEC2MS(11) && cur_client->GetSlotNumber() <= GetActiveUploadsCount()+ 2 || ::GetTickCount()-m_nLastStartUpload <= SEC2MS(1) && cur_client->GetSlotNumber() <= GetActiveUploadsCount()+ 10 || ::GetTickCount()-m_nLastStartUpload <= 150 || !cur_client->GetScheduledRemovalLimboComplete() || pos != NULL || cur_client->GetSlotNumber() <= GetActiveUploadsCount() || ForceNewClient(true))
//<<< WiZaRd::ZZUL Upload [ZZ]
			{
//>>> WiZaRd::Drop Blocking Sockets [Xman?]
				//wait until we have some data...
				if(cur_client->GetUpStartTimeDelay() > MIN2MS(1))
				{
					// WiZaRd - ToDo: why should we check for the speed? The blocking values are more interesting for us
					if(cur_client->GetSessionUp()/cur_client->GetUpStartTimeDelay()*1000.0f < UPLOAD_LOWEST_VALUE) //at least xkbps
					{
						const float cur_block = cur_client->socket->GetOverallBlockingRatio();
						if(cur_block > maxblock)
						{
							maxblock = cur_block;
							blockclient = cur_client;
						}
					}
				}
//<<< WiZaRd::Drop Blocking Sockets [Xman?]
				cur_client->SendBlockData();
			}
//>>> WiZaRd::ZZUL Upload [ZZ]
			else 
			{
				bool keepWaitingTime = cur_client->GetScheduledUploadShouldKeepWaitingTime();
				RemoveFromUploadQueue(cur_client, (CString)_T("Scheduled for removal: ") + cur_client->GetScheduledRemovalDebugReason(), true, keepWaitingTime);
				AddClientToQueue(cur_client, true, keepWaitingTime);
				m_nLastStartUpload = ::GetTickCount()-SEC2MS(9);
			}
//<<< WiZaRd::ZZUL Upload [ZZ]
        }
    }

//>>> WiZaRd::Drop Blocking Sockets [Xman?]
	if(thePrefs.DropBlockingSockets())
	{
		const UINT MaxSpeed = theApp.lastCommonRouteFinder->GetUpload();
		const UINT curUploadSlots = (UINT)uploadinglist.GetCount();
		if(blockclient 
			/*curUploadSlots+2 >= MAX_UP_CLIENTS_ALLOWED*/
//>>> WiZaRd::ZZUL Upload [ZZ]
			//&& (curUploadSlots>= MAX_UP_CLIENTS_ALLOWED
			&& (curUploadSlots >= (datarate/UPLOAD_CHECK_CLIENT_DR_DFLT)
//<<< WiZaRd::ZZUL Upload [ZZ]
			|| curUploadSlots * UPLOAD_LOWEST_VALUE >= MaxSpeed) //hard coded final limit
			)
		{
			//search a socket we should remove
			if(blockclient->socket->GetOverallBlockingRatio() > thePrefs.GetMaxBlockRate()	//95% of all send were blocked
				&& blockclient->socket->GetBlockingRatio() > thePrefs.GetMaxBlockRate20())	//96% the last 20 seconds
			{
				if(RemoveFromUploadQueue(blockclient, L"Blocking", true, true))
				{
					theApp.QueueDebugLogLineEx(LOG_WARNING, L"Client %s is blocking too often and max slots are reached: avg20: %1.2f%%, all: %1.2f%%, avg. ul: %s", 
						blockclient->DbgGetClientInfo(), blockclient->socket->GetBlockingRatio(), blockclient->socket->GetOverallBlockingRatio(), CastItoXBytes(blockclient->GetSessionUp()/blockclient->GetUpStartTimeDelay()*1000.0f, false, true));
					blockclient->SendOutOfPartReqsAndAddToWaitingQueue(); //we SHOULD send this, no?

					m_BlockStopList.AddHead(curTick); //remember when this happened
					//because there are some users out there which set a too high uploadlimit, this code isn't usable
					//we deactivate it and warn the user
					if(m_BlockStopList.GetCount() >= 6) //5 old + one new element
					{
						const UINT oldestblocktime = m_BlockStopList.GetAt(m_BlockStopList.FindIndex(5)); // the 6th element
						if(curTick - oldestblocktime < HR2MS(1))
						{
							//5 block-drops during 1 hour is too much->warn the user and disable the feature
							thePrefs.SetDropBlockingSockets(false);
							CString msg = GetResString(IDS_DROP_BLOCKING_DISABLED);
							theApp.QueueLogLineEx(LOG_WARNING, msg);
							theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);
							m_BlockStopList.RemoveAll(); //clear list
						}
						else //added due to RemoveAll() above
							m_BlockStopList.RemoveTail();
					}
//					theApp.uploadBandwidthThrottler->SetNoNeedSlot(); //this can occur after increasing slotspeed
				}
			}
		}
	}
//<<< WiZaRd::Drop Blocking Sockets [Xman?]

    // Save used bandwidth for speed calculations
    uint64 sentBytes = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
    avarage_dr_list.AddTail(sentBytes);
    m_avarage_dr_sum += sentBytes;

    (void)theApp.uploadBandwidthThrottler->GetNumberOfSentBytesOverheadSinceLastCallAndReset();

    avarage_friend_dr_list.AddTail(theStats.sessionSentBytesToFriend);

    // Save time beetween each speed snapshot
    avarage_tick_list.AddTail(curTick);

    // don't save more than 30 secs of data
    while(avarage_tick_list.GetCount() > 3 && !avarage_friend_dr_list.IsEmpty() && ::GetTickCount()-avarage_tick_list.GetHead() > 30*1000)
    {
        m_avarage_dr_sum -= avarage_dr_list.RemoveHead();
        avarage_friend_dr_list.RemoveHead();
        avarage_tick_list.RemoveHead();
    }

    if (GetDatarate() > HIGHSPEED_UPLOADRATE_START && m_hHighSpeedUploadTimer == 0)
        UseHighSpeedUploadTimer(true);
    else if (GetDatarate() < HIGHSPEED_UPLOADRATE_END && m_hHighSpeedUploadTimer != 0)
        UseHighSpeedUploadTimer(false);
};

//>>> WiZaRd::ZZUL Upload [ZZ]
/*
bool CUploadQueue::AcceptNewClient(bool addOnNextConnect) const
{
    UINT curUploadSlots = (UINT)uploadinglist.GetCount();

    //We allow ONE extra slot to be created to accommodate lowID users.
    //This is because we skip these users when it was actually their turn
    //to get an upload slot..
    if(addOnNextConnect && curUploadSlots > 0)
        curUploadSlots--;

    return AcceptNewClient(curUploadSlots);
}
*/
//<<< WiZaRd::ZZUL Upload [ZZ]

bool CUploadQueue::AcceptNewClient(UINT curUploadSlots) const
{
	// check if we can allow a new client to start downloading from us
	if (curUploadSlots < MIN_UP_CLIENTS_ALLOWED)
		return true;

//>>> WiZaRd::ZZUL Upload [ZZ]
	UINT wantedNumberOfTrickles = GetWantedNumberOfTrickleUploads();
	if(curUploadSlots > m_MaxActiveClients+wantedNumberOfTrickles)
		return false;
//<<< WiZaRd::ZZUL Upload [ZZ]

	uint16 MaxSpeed;
    if (thePrefs.IsDynUpEnabled())
        MaxSpeed = (uint16)(theApp.lastCommonRouteFinder->GetUpload()/1024);
    else
		MaxSpeed = thePrefs.GetMaxUpload();
		
//>>> WiZaRd::ZZUL Upload [ZZ]	
	if (curUploadSlots >= 4 
		&& curUploadSlots >= (datarate/UPLOAD_CHECK_CLIENT_DR_DFLT)) // max number of clients to allow for all circumstances
	{
		if(curUploadSlots < 20) 
		{
			int numberOfOkClients = 0;
			const int neededOkClients = (int)ceil(curUploadSlots/4.0f);

			POSITION ulpos = uploadinglist.GetHeadPosition();
			while (numberOfOkClients < neededOkClients && ulpos != NULL) 
			{
				POSITION curpos = ulpos;
				uploadinglist.GetNext(ulpos);

				CUpDownClient* checkingClient = uploadinglist.GetAt(curpos);
				if(checkingClient->GetDatarate() > UPLOAD_CLIENT_DATARATE_DFLT) 
					++numberOfOkClients;
			}

			if(numberOfOkClients >= neededOkClients)
				return true;			 
			return false;
		} 
		else
			return false;
	}
/*
	UINT clientDataRate = GetClientDataRate();
    if(curUploadSlots >= 4)
	{
		// max number of clients to allow for all circumstances
        if(curUploadSlots >= MAX_UP_CLIENTS_ALLOWED
			|| curUploadSlots >= (datarate/GetClientDataRateCheck())
			|| curUploadSlots >= ((UINT)MaxSpeed)*1024/clientDataRate
			|| (
				  thePrefs.GetMaxUpload() == UNLIMITED
				  && !thePrefs.IsDynUpEnabled()
				  && thePrefs.GetMaxGraphUploadRate(true) > 0
				  && curUploadSlots >= ((UINT)thePrefs.GetMaxGraphUploadRate(false))*1024/clientDataRate
				)
			)    
	    return false;
	}
*/
//<<< WiZaRd::ZZUL Upload [ZZ]

	return true;
}

//>>> WiZaRd::ZZUL Upload [ZZ]
/*
bool CUploadQueue::ForceNewClient(bool allowEmptyWaitingQueue)
{
    if(!allowEmptyWaitingQueue && waitinglist.GetSize() <= 0)
        return false;

    if (::GetTickCount() - m_nLastStartUpload < 1000 && datarate < 102400 )
        return false;

    UINT curUploadSlots = (UINT)uploadinglist.GetCount();
*/
bool CUploadQueue::ForceNewClient(bool simulateScheduledClosingOfSlot) 
{
	if (::GetTickCount() - m_nLastStartUpload < SEC2MS(1) && datarate < 102400 )
		return false;

	UINT curUploadSlots = (UINT)GetEffectiveUploadListCount();
	UINT curUploadSlotsReal = (UINT)uploadinglist.GetCount();

	if(simulateScheduledClosingOfSlot)
	{
		if(curUploadSlotsReal < 1)
			return true;
		else
			--curUploadSlotsReal;
	}
//<<< WiZaRd::ZZUL Upload [ZZ]

    if (curUploadSlots < MIN_UP_CLIENTS_ALLOWED)
        return true;

//>>> WiZaRd::ZZUL Upload [ZZ]
	//if(CanForceClient(curUploadSlots)) {
	UINT activeSlots = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;

	if(simulateScheduledClosingOfSlot)
		activeSlots = m_MaxActiveClientsShortTime;

	if(curUploadSlotsReal < m_iHighestNumberOfFullyActivatedSlotsSinceLastCall && AcceptNewClient(curUploadSlots*2) /*+1*/ ||
		curUploadSlotsReal < m_iHighestNumberOfFullyActivatedSlotsSinceLastCall && ::GetTickCount() - m_nLastStartUpload > SEC2MS(1) ||
		curUploadSlots < m_iHighestNumberOfFullyActivatedSlotsSinceLastCall+1 && ::GetTickCount() - m_nLastStartUpload > SEC2MS(10))
			return true;
	//}

	//nope
	return false;
}

bool CUploadQueue::CanForceClient(UINT curUploadSlots) const
{
//<<< WiZaRd::ZZUL Upload [ZZ]
    if(!AcceptNewClient(curUploadSlots) || !theApp.lastCommonRouteFinder->AcceptNewClient())   // UploadSpeedSense can veto a new slot if USS enabled
        return false;

    uint16 MaxSpeed;
    if (thePrefs.IsDynUpEnabled())
        MaxSpeed = (uint16)(theApp.lastCommonRouteFinder->GetUpload()/1024);
    else
        MaxSpeed = thePrefs.GetMaxUpload();

//>>> WiZaRd::Dynamic Datarate
	UINT origUpPerClient = UPLOAD_CLIENT_DATARATE_DFLT;
    //UINT origUpPerClient = GetClientDataRate();
//<<< WiZaRd::Dynamic Datarate
    UINT upPerClient = origUpPerClient;

    // if throttler doesn't require another slot, go with a slightly more restrictive method
    if( MaxSpeed > 20 || MaxSpeed == UNLIMITED)
        upPerClient += datarate/43;

//>>> WiZaRd::Dynamic Datarate
    const UINT checkUp = UINT(2.5 * origUpPerClient);
	//const UINT checkUp = UPLOAD_CLIENT_DATARATE_DFLT + UPLOAD_LOWEST_VALUE;	
//<<< WiZaRd::Dynamic Datarate
    if(upPerClient > checkUp)
        upPerClient = checkUp;

    //now the final check
	UINT nMaxSlots;
    if ( MaxSpeed == UNLIMITED )
		nMaxSlots = datarate / upPerClient;
    else
    {        
        if (MaxSpeed > 12)
            nMaxSlots = (UINT)(((float)(MaxSpeed*1024)) / upPerClient);
        else if (MaxSpeed > 7)
            nMaxSlots = MIN_UP_CLIENTS_ALLOWED + 2;
        else if (MaxSpeed > 3)
            nMaxSlots = MIN_UP_CLIENTS_ALLOWED + 1;
        else
            nMaxSlots = MIN_UP_CLIENTS_ALLOWED;
//		AddLogLine(true,"maxslots=%u, upPerClient=%u, datarateslot=%u|%u|%u",nMaxSlots,upPerClient,datarate/UPLOAD_CHECK_CLIENT_DR, datarate, UPLOAD_CHECK_CLIENT_DR);
    }
	if ( curUploadSlots < nMaxSlots )
		return true;

    if(m_iHighestNumberOfFullyActivatedSlotsSinceLastCall > (UINT)uploadinglist.GetSize())
    {
        // uploadThrottler requests another slot. If throttler says it needs another slot, we will allow more slots
        // than what we require ourself. Never allow more slots than to give each slot high enough average transfer speed, though (checked above).
        //if(thePrefs.GetLogUlDlEvents() && waitinglist.GetSize() > 0)
        //    AddDebugLogLine(false, _T("UploadQueue: Added new slot since throttler needs it. m_iHighestNumberOfFullyActivatedSlotsSinceLastCall: %i uploadinglist.GetSize(): %i tick: %i"), m_iHighestNumberOfFullyActivatedSlotsSinceLastCall, uploadinglist.GetSize(), ::GetTickCount());
        return true;
    }

    //nope
    return false;
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP_UDP(UINT dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs)
{
    CUpDownClient* pMatchingIPClient = NULL;
    UINT cMatches = 0;
    for (POSITION pos = waitinglist.GetHeadPosition(); pos != 0;)
    {
        CUpDownClient* cur_client = waitinglist.GetNext(pos);
        if (dwIP == cur_client->GetIP() && nUDPPort == cur_client->GetUDPPort())
            return cur_client;
        else if (dwIP == cur_client->GetIP() && bIgnorePortOnUniqueIP)
        {
            pMatchingIPClient = cur_client;
            cMatches++;
        }
    }
    if (pbMultipleIPs != NULL)
        *pbMultipleIPs = cMatches > 1;

    if (pMatchingIPClient != NULL && cMatches == 1)
        return pMatchingIPClient;
    else
        return NULL;
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP(UINT dwIP)
{
    for (POSITION pos = waitinglist.GetHeadPosition(); pos != 0;)
    {
        CUpDownClient* cur_client = waitinglist.GetNext(pos);
        if (dwIP == cur_client->GetIP())
            return cur_client;
    }
    return 0;
}

/**
 * Add a client to the waiting queue for uploads.
 *
 * @param client address of the client that should be added to the waiting queue
 *
 * @param bIgnoreTimelimit don't check time limit to possibly ban the client.
 *
 * @param addInFirstPlace the client should be added first in queue, not last
 */
//>>> WiZaRd::ZZUL Upload [ZZ]
//void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit)
void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit, bool addInFirstPlace)
//<<< WiZaRd::ZZUL Upload [ZZ]
{
//>>> WiZaRd::ZZUL Upload [ZZ]
	CKnownFile* reqfile = NULL;
	if(!addInFirstPlace)
	{
//<<< WiZaRd::ZZUL Upload [ZZ]
    //This is to keep users from abusing the limits we put on lowID callbacks.
    //1)Check if we are connected to any network and that we are a lowID.
    //(Although this check shouldn't matter as they wouldn't have found us..
    // But, maybe I'm missing something, so it's best to check as a precaution.)
    //2)Check if the user is connected to Kad. We do allow all Kad Callbacks.
    //3)Check if the user is in our download list or a friend..
    //We give these users a special pass as they are helping us..
    //4)Are we connected to a server? If we are, is the user on the same server?
    //TCP lowID callbacks are also allowed..
    //5)If the queue is very short, allow anyone in as we want to make sure
    //our upload is always used.
    // removed servers
    client->AddAskedCount();
    client->SetLastUpRequest();

//>>> WiZaRd::ClientAnalyzer
    reqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
    if(reqfile == NULL)
        return; //should never happen, just in case, though...
    if(client->DownloadingAndUploadingFilesAreEqual(reqfile))
    {
        if(client->GetAntiLeechData())
            client->GetAntiLeechData()->SetBadForThisSession(AT_FILEFAKER);
        return; //nope - come back later...
    }
//<<< WiZaRd::ClientAnalyzer

    if (!bIgnoreTimelimit)
        client->AddRequestCount(client->GetUploadFileID());
    if (client->IsBanned())
        return;
//>>> WiZaRd::FiX
    //moved here... that way we count the request even in case of an early return (but still not if the request comes from a banned client)
    // statistic values
    reqfile->statistic.AddRequest();
//<<< WiZaRd::FiX
	} //>>> WiZaRd::ZZUL Upload [ZZ]
	else
	{
//>>> WiZaRd::ClientAnalyzer
		reqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
		if(reqfile == NULL)
			return; //should never happen, just in case, though...
		if(client->DownloadingAndUploadingFilesAreEqual(reqfile))
		{
			if(client->GetAntiLeechData())
				client->GetAntiLeechData()->SetBadForThisSession(AT_FILEFAKER);
			return; //nope - come back later...
		}
//<<< WiZaRd::ClientAnalyzer
	}

    uint16 cSameIP = 0;
    // check for double
    POSITION pos1, pos2;
    for (pos1 = waitinglist.GetHeadPosition(); ( pos2 = pos1 ) != NULL;)
    {
        waitinglist.GetNext(pos1);
        CUpDownClient* cur_client= waitinglist.GetAt(pos2);
        if (cur_client == client)
        {
//>>> WiZaRd::ZZUL Upload [ZZ]
			if (!addInFirstPlace && client->m_bAddNextConnect && AcceptNewClient(client->m_bAddNextConnect))
            //if (client->m_bAddNextConnect && AcceptNewClient(client->m_bAddNextConnect))
//<<< WiZaRd::ZZUL Upload [ZZ]
            {
                //Special care is given to lowID clients that missed their upload slot
                //due to the saving bandwidth on callbacks.
                if(thePrefs.GetLogUlDlEvents())
                    AddDebugLogLine(true, _T("Adding ****lowid when reconnecting. Client: %s"), client->DbgGetClientInfo());
                client->m_bAddNextConnect = false;
                RemoveFromWaitingQueue(client, true);
                // statistic values // TODO: Maybe we should change this to count each request for a file only once and ignore reasks
                reqfile->statistic.AddRequest();
                AddUpNextClient(_T("Adding ****lowid when reconnecting."), client);
                return;
            }
            client->SendRankingInfo();
            theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(client);
            return;
        }
        else if ( client->Compare(cur_client) )
        {
            theApp.clientlist->AddTrackClient(client); // in any case keep track of this client

            // another client with same ip:port or hash
            // this happens only in rare cases, because same userhash / ip:ports are assigned to the right client on connecting in most cases
            if (cur_client->credits != NULL && cur_client->credits->GetCurrentIdentState(cur_client->GetIP()) == IS_IDENTIFIED)
            {
                //cur_client has a valid secure hash, don't remove him
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, GetResString(IDS_SAMEUSERHASH), client->GetUserName(), cur_client->GetUserName(), client->GetUserName());
                return;
            }
            if (client->credits != NULL && client->credits->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED)
            {
                //client has a valid secure hash, add him remove other one
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, GetResString(IDS_SAMEUSERHASH), client->GetUserName(), cur_client->GetUserName(), cur_client->GetUserName());
                RemoveFromWaitingQueue(pos2,true);
                if (!cur_client->socket)
                {
                    if(cur_client->Disconnected(_T("AddClientToQueue - same userhash 1")))
                        delete cur_client;
                }
            }
            else
            {
                // remove both since we do not know who the bad one is
                if (thePrefs.GetVerbose())
                    AddDebugLogLine(false, GetResString(IDS_SAMEUSERHASH), client->GetUserName() ,cur_client->GetUserName(), _T("Both"));
                RemoveFromWaitingQueue(pos2,true);
                if (!cur_client->socket)
                {
                    if(cur_client->Disconnected(_T("AddClientToQueue - same userhash 2")))
                        delete cur_client;
                }
                return;
            }
        }
        else if (client->GetIP() == cur_client->GetIP())
        {
            // same IP, different port, different userhash
            cSameIP++;
        }
    }
    if (cSameIP >= 3)
    {
        // do not accept more than 3 clients from the same IP
        if (thePrefs.GetVerbose())
            DEBUG_ONLY( AddDebugLogLine(false,_T("%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP"), client->GetUserName(), ipstr(client->GetConnectIP())) );
        return;
    }
    else if (theApp.clientlist->GetClientsFromIP(client->GetIP()) >= 3)
    {
        if (thePrefs.GetVerbose())
            DEBUG_ONLY( AddDebugLogLine(false,_T("%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP (found in TrackedClientsList)"), client->GetUserName(), ipstr(client->GetConnectIP())) );
        return;
    }
    // done

//>>> WiZaRd::ZZUL Upload [ZZ]
	if(!addInFirstPlace)
	{
//<<< WiZaRd::ZZUL Upload [ZZ]
    // emule collection will bypass the queue
    if(reqfile && reqfile->GetFileSize() < theApp.uploadqueue->GetSmallFileSize())
    {
        RemoveFromWaitingQueue(client, true);
        AddUpNextClient(L"Small File Priority Slot", client);
        return;
    }

    // cap the list
    // the queue limit in prefs is only a soft limit. Hard limit is 25% higher, to let in powershare clients and other
    // high ranking clients after soft limit has been reached
//>>> WiZaRd::QueueSize FiX
    //this makes eMule to keep the PROPER limit that is set via preferences
    //by default:
    //softlimit = pref-limit
    //hardlimit = soft + 25% of soft but at least 200
    //now (fixed):
    //hardlimit = preflimit
    //softlimit = 80% of hard but at least 200
    UINT hardQueueLimit = thePrefs.GetQueueSize();
    UINT softQueueLimit = (UINT)(thePrefs.GetQueueSize()*0.8f);
    if(hardQueueLimit - softQueueLimit < 200)
        softQueueLimit = hardQueueLimit-200;
//<<< WiZaRd::QueueSize FiX

    // if soft queue limit has been reached, only let in high ranking clients
    if ((UINT)waitinglist.GetCount() >= hardQueueLimit ||
            (UINT)waitinglist.GetCount() >= softQueueLimit && // soft queue limit is reached
            (client->IsFriend() && client->GetFriendSlot()) == false && // client is not a friend with friend slot
            client->GetCombinedFilePrioAndCredit() < GetAverageCombinedFilePrioAndCredit())   // and client has lower credits/wants lower prio file than average client in queue
    {

        // then block client from getting on queue
        return;
    }
    if (client->IsDownloading())
    {
        // he's already downloading and wants probably only another file
        if (thePrefs.GetDebugClientTCPLevel() > 0)
            DebugSend("OP__AcceptUploadReq", client);
        Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
        theStats.AddUpDataOverheadFileRequest(packet->size);
        client->SendPacket(packet, true);
        return;
    }
//>>> WiZaRd::ZZUL Upload [ZZ]
		client->ResetQueueSessionUp();
	}
//<<< WiZaRd::ZZUL Upload [ZZ]
//>>> WiZaRd::Startup Flood Prevention
    //Requested by James R. Bath
    //http://forum.emule-project.net/index.php?showtopic=101181
/*
    if (waitinglist.IsEmpty() && ForceNewClient(true))
    	AddUpNextClient(_T("Direct add with empty queue."), client);
    else
*/
//<<< WiZaRd::Startup Flood Prevention
    {
        m_bStatisticsWaitingListDirty = true;
        waitinglist.AddTail(client);
        client->SetUploadState(US_ONUPLOADQUEUE);
//>>> WiZaRd::ZZUL Upload [ZZ]
		// Add client to waiting list. If addInFirstPlace is set, client should not have its waiting time resetted
		theApp.emuledlg->transferwnd->GetQueueList()->AddClient(client, (addInFirstPlace == false));
        //theApp.emuledlg->transferwnd->GetQueueList()->AddClient(client,true);
//<<< WiZaRd::ZZUL Upload [ZZ]
        theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
        client->SendRankingInfo();
    }
}

float CUploadQueue::GetAverageCombinedFilePrioAndCredit()
{
    DWORD curTick = ::GetTickCount();

    if (curTick - m_dwLastCalculatedAverageCombinedFilePrioAndCredit > 5*1000)
    {
        m_dwLastCalculatedAverageCombinedFilePrioAndCredit = curTick;

        // TODO: is there a risk of overflow? I don't think so...
        double sum = 0;
        for (POSITION pos = waitinglist.GetHeadPosition(); pos != NULL; /**/)
        {
            CUpDownClient* cur_client =	waitinglist.GetNext(pos);
            sum += cur_client->GetCombinedFilePrioAndCredit();
        }
        m_fAverageCombinedFilePrioAndCredit = (float)(sum/waitinglist.GetSize());
    }

    return m_fAverageCombinedFilePrioAndCredit;
}

bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason, bool updatewindow, bool earlyabort)
{
    bool result = false;
    UINT slotCounter = 1;
//>>> WiZaRd::Payback First
    POSITION findPos = m_lPaybackList.Find(client);
    if(findPos)
        m_lPaybackList.RemoveAt(findPos);
//<<< WiZaRd::Payback First
    for (POSITION pos = uploadinglist.GetHeadPosition(); pos != 0;)
    {
        POSITION curPos = pos;
        CUpDownClient* curClient = uploadinglist.GetNext(pos);
        if (client == curClient)
        {
//>>> WiZaRd::ZZUL Upload [ZZ]
			if(client->socket) 
			{
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__OutOfPartReqs", client);
				Packet* pCancelTransferPacket = new Packet(OP_OUTOFPARTREQS, 0);
				theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
				client->socket->SendPacket(pCancelTransferPacket, true, true);
			}
//<<< WiZaRd::ZZUL Upload [ZZ]
            if (updatewindow)
                theApp.emuledlg->transferwnd->GetUploadList()->RemoveClient(client);

            if (thePrefs.GetLogUlDlEvents())
//>>> WiZaRd::ZZUL Upload [ZZ]
				AddDebugLogLine(DLP_DEFAULT, true,_T("Removing client from upload list: %s Client: %s Transferred: %s SessionUp: %s QueueSessionUp: %s QueueSessionPayload: %s In buffer: %s Req blocks: %i File: %s"), pszReason==NULL ? _T("") : pszReason, client->DbgGetClientInfo(), CastSecondsToHM( client->GetUpStartTimeDelay()/1000), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false), CastItoXBytes(client->GetPayloadInBuffer()), client->GetNumberOfRequestedBlocksInQueue(), (theApp.sharedfiles->GetFileByID(client->GetUploadFileID())?theApp.sharedfiles->GetFileByID(client->GetUploadFileID())->GetFileName():_T("")));
                //AddDebugLogLine(DLP_DEFAULT, true,_T("Removing client from upload list: %s Client: %s Transferred: %s SessionUp: %s QueueSessionPayload: %s In buffer: %s Req blocks: %i File: %s"), pszReason==NULL ? _T("") : pszReason, client->DbgGetClientInfo(), CastSecondsToHM( client->GetUpStartTimeDelay()/1000), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false), CastItoXBytes(client->GetPayloadInBuffer()), client->GetNumberOfRequestedBlocksInQueue(), (theApp.sharedfiles->GetFileByID(client->GetUploadFileID())?theApp.sharedfiles->GetFileByID(client->GetUploadFileID())->GetFileName():_T("")));
//<<< WiZaRd::ZZUL Upload [ZZ]
            client->m_bAddNextConnect = false;
//>>> WiZaRd::ZZUL Upload [ZZ]
			client->UnscheduleForRemoval();
			if(!earlyabort)
				client->SetWaitStartTime();
//<<< WiZaRd::ZZUL Upload [ZZ]
            uploadinglist.RemoveAt(curPos);

            bool removed = theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);
            (void)removed;
            //if(thePrefs.GetLogUlDlEvents() && !(removed || pcRemoved)) {
            //    AddDebugLogLine(false, _T("UploadQueue: Didn't find socket to delete. Adress: 0x%x"), client->socket);
            //}

//>>> WiZaRd::ZZUL Upload [ZZ]
            //if(client->GetSessionUp() > 0)
			if(client->GetQueueSessionUp() > 0)
//<<< WiZaRd::ZZUL Upload [ZZ]
            {
//>>> WiZaRd::ZZUL Upload [ZZ]
				theStats.IncTotalCompletedBytes(client->GetQueueSessionUp()); 
				if(client->GetSessionUp() > 0)
//<<< WiZaRd::ZZUL Upload [ZZ]
					totaluploadtime += client->GetUpStartTimeDelay()/1000;
//>>> WiZaRd::ClientAnalyzer
                if(client->pAntiLeechData)
                    client->pAntiLeechData->AddULSession(false);
//<<< WiZaRd::ClientAnalyzer
                ++successfullupcount;				
            }
            else if(earlyabort == false)
            {
//>>> WiZaRd::ClientAnalyzer
                if(client->pAntiLeechData)
                    client->pAntiLeechData->AddULSession(true);
//<<< WiZaRd::ClientAnalyzer
                ++failedupcount;
            }

            CKnownFile* requestedFile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
            if(requestedFile != NULL)
            {
                requestedFile->UpdatePartsInfo();
            }
            theApp.clientlist->AddTrackClient(client); // Keep track of this client
            client->SetUploadState(US_NONE);
            client->ClearUploadBlockRequests();

            m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;

            result = true;
        }
        else
        {
            curClient->SetSlotNumber(slotCounter);
            slotCounter++;
        }
    }
    return result;
}

UINT CUploadQueue::GetAverageUpTime()
{
    if( successfullupcount )
    {
        return totaluploadtime/successfullupcount;
    }
    return 0;
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow)
{
    POSITION pos = waitinglist.Find(client);
    if (pos)
    {
        RemoveFromWaitingQueue(pos,updatewindow);
        return true;
    }
    else
        return false;
}

void CUploadQueue::RemoveFromWaitingQueue(POSITION pos, bool updatewindow)
{
    m_bStatisticsWaitingListDirty = true;
    CUpDownClient* todelete = waitinglist.GetAt(pos);
    waitinglist.RemoveAt(pos);
    if (updatewindow)
    {
        theApp.emuledlg->transferwnd->GetQueueList()->RemoveClient(todelete);
        theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
    }
    todelete->m_bAddNextConnect = false;
    todelete->SetUploadState(US_NONE);
}

void CUploadQueue::UpdateMaxClientScore()
{
    m_imaxscore=0;
    for(POSITION pos = waitinglist.GetHeadPosition(); pos != 0; )
    {
        UINT score = waitinglist.GetNext(pos)->GetScore(true, false);
        if(score > m_imaxscore )
            m_imaxscore=score;
    }
}

bool CUploadQueue::CheckForTimeOver(CUpDownClient* client)
{
    //If we have nobody in the queue, do NOT remove the current uploads..
    //This will save some bandwidth and some unneeded swapping from upload/queue/upload..
    if ( waitinglist.IsEmpty() || client->GetFriendSlot() )
        return false;

    // Allow the client to download a specified amount per session
    if( client->GetQueueSessionPayloadUp() > SESSIONMAXTRANS )
    {
        if (thePrefs.GetLogUlDlEvents())
            AddDebugLogLine(DLP_DEFAULT, false, _T("%s: Upload session ended due to max transferred amount. %s"), client->GetUserName(), CastItoXBytes(SESSIONMAXTRANS, false, false));
        return true;
    }
    return false;
}

void CUploadQueue::DeleteAll()
{
    waitinglist.RemoveAll();
    uploadinglist.RemoveAll();
    // PENDING: Remove from UploadBandwidthThrottler as well!
}

UINT CUploadQueue::GetWaitingPosition(CUpDownClient* client)
{
    if (!IsOnUploadQueue(client))
        return 0;
    UINT rank = 1;
    UINT myscore = client->GetScore(false);
    for (POSITION pos = waitinglist.GetHeadPosition(); pos != 0; )
    {
//>>> WiZaRd::ZZUL Upload [ZZ]
        //if (waitinglist.GetNext(pos)->GetScore(false) > myscore)
		CUpDownClient* compareClient = waitinglist.GetNext(pos);
		if (RightClientIsBetter(client, myscore, compareClient, compareClient->GetScore(false)))
//<<< WiZaRd::ZZUL Upload [ZZ]
            rank++;
    }
    return rank;
}

VOID CALLBACK CUploadQueue::UploadTimer(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
    // NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
    try
    {
        // Barry - Don't do anything if the app is shutting down - can cause unhandled exceptions
        if (!theApp.emuledlg->IsRunning())
            return;

        // Elandal:ThreadSafeLogging -->
        // other threads may have queued up log lines. This prints them.
        theApp.HandleLogQueues();
        // Elandal: ThreadSafeLogging <--

        // ZZ:UploadSpeedSense -->
        theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(),
                                               theApp.uploadqueue->GetDatarate(),
                                               thePrefs.GetMinUpload()*1024,
                                               (thePrefs.GetMaxUpload() != 0) ? thePrefs.GetMaxUpload() * 1024 : thePrefs.GetMaxGraphUploadRate(false) * 1024,
                                               thePrefs.IsDynUpUseMillisecondPingTolerance(),
                                               (thePrefs.GetDynUpPingTolerance() > 100) ? ((thePrefs.GetDynUpPingTolerance() - 100) / 100.0f) : 0,
                                               thePrefs.GetDynUpPingToleranceMilliseconds(),
                                               thePrefs.GetDynUpGoingUpDivider(),
                                               thePrefs.GetDynUpGoingDownDivider(),
                                               thePrefs.GetDynUpNumberOfPings(),
//>>> WiZaRd::ZZUL Upload [ZZ]
											   10, // PENDING: Hard coded min pLowestPingAllowed
											   theApp.uploadqueue->GetActiveUploadsCount() > (UINT)theApp.uploadqueue->GetUploadQueueLength());		
                                               //20); // PENDING: Hard coded min pLowestPingAllowed
//<<< WiZaRd::ZZUL Upload [ZZ]
        // ZZ:UploadSpeedSense <--

        theApp.uploadqueue->Process();
        theApp.downloadqueue->Process();
        if (thePrefs.ShowOverhead())
        {
            theStats.CompUpDatarateOverhead();
            theStats.CompDownDatarateOverhead();
        }
        counter++;

        // one second
        if (counter >= 10)
        {
            counter=0;

            // try to use different time intervals here to not create any disk-IO bottle necks by saving all files at once
            theApp.clientcredits->Process();	// 13 minutes
            theApp.knownfiles->Process();		// 11 minutes
            theApp.antileechlist->Process();	// 18 minutes  //>>> WiZaRd::ClientAnalyzer
            theApp.friendlist->Process();		// 19 minutes
            theApp.clientlist->Process();
            theApp.sharedfiles->Process();
            if( Kademlia::CKademlia::IsRunning() )
            {
                Kademlia::CKademlia::Process();
                if(Kademlia::CKademlia::GetPrefs()->HasLostConnection())
                {
                    LogError(LOG_STATUSBAR, GetResString(IDS_ERR_LOSTC), GetResString(IDS_KADEMLIA));
                    Kademlia::CKademlia::Stop();
                    theApp.emuledlg->ShowConnectionState();
                    Kademlia::CKademlia::Start(); //>>> WiZaRd::Kad Reconnect
                }
            }

            theApp.listensocket->UpdateConnectionsStatus();
            if (thePrefs.WatchClipboard4ED2KLinks())
            {
                // TODO: Remove this from here. This has to be done with a clipboard chain
                // and *not* with a timer!!
                theApp.SearchClipboard();
            }

            // 2 seconds
            i2Secs++;
            if (i2Secs>=2)
            {
                i2Secs=0;

                // Update connection stats...
                theStats.UpdateConnectionStats((float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
#ifdef HAVE_WIN7_SDK_H
                theApp.emuledlg->UpdateStatusBarProgress();
#endif
            }

            // display graphs
            if (thePrefs.GetTrafficOMeterInterval()>0)
            {
                igraph++;

                if (igraph >= (UINT)(thePrefs.GetTrafficOMeterInterval()) )
                {
                    igraph=0;
                    //theApp.emuledlg->statisticswnd->SetCurrentRate((float)(theApp.uploadqueue->Getavgupload()/theApp.uploadqueue->Getavg())/1024,(float)(theApp.uploadqueue->Getavgdownload()/theApp.uploadqueue->Getavg())/1024);
                    theApp.emuledlg->statisticswnd->SetCurrentRate((float)(theApp.uploadqueue->GetDatarate())/1024,(float)(theApp.downloadqueue->GetDatarate())/1024);
                    //theApp.uploadqueue->Zeroavg();
                }
            }
            if (theApp.emuledlg->activewnd == theApp.emuledlg->statisticswnd && theApp.emuledlg->IsWindowVisible() )
            {
                // display stats
                if (thePrefs.GetStatsInterval()>0)
                {
                    istats++;

                    if (istats >= (UINT)(thePrefs.GetStatsInterval()) )
                    {
                        istats=0;
                        theApp.emuledlg->statisticswnd->ShowStatistics();
                    }
                }
            }

            theApp.uploadqueue->UpdateDatarates();

            //save rates every second
            theStats.RecordRate();

            // ZZ:UploadSpeedSense -->
            theApp.emuledlg->ShowPing();

            /*bool gotEnoughHosts = */
            theApp.clientlist->GiveClientsForTraceRoute();
            // ZZ:UploadSpeedSense <--

            if (theApp.emuledlg->IsTrayIconToFlash())
                theApp.emuledlg->ShowTransferRate(true);

            sec++;
            // *** 5 seconds **********************************************
            if (sec >= 5)
            {
#ifdef _DEBUG
                if (thePrefs.m_iDbgHeap > 0 && !AfxCheckMemory())
                    AfxDebugBreak();
#endif

                sec = 0;
                theApp.listensocket->Process();
                theApp.OnlineSig(); // Added By Bouc7
                if (!theApp.emuledlg->IsTrayIconToFlash())
                    theApp.emuledlg->ShowTransferRate();

                thePrefs.EstimateMaxUploadCap(theApp.uploadqueue->GetDatarate()/1024);

                // update cat-titles with downloadinfos only when needed
                if (thePrefs.ShowCatTabInfos() &&
                        theApp.emuledlg->activewnd == theApp.emuledlg->transferwnd &&
                        theApp.emuledlg->IsWindowVisible())
                    theApp.emuledlg->transferwnd->UpdateCatTabTitles(false);

                theApp.emuledlg->transferwnd->UpdateListCount(CTransferDlg::wnd2Uploading, -1);
            }

            statsave++;
            // *** 60 seconds *********************************************
            if (statsave >= 60)
            {
                statsave=0;

                if (thePrefs.GetPreventStandby())
                    theApp.ResetStandByIdleTimer(); // Reset Windows idle standby timer if necessary
            }

            s_uSaveStatistics++;
            if (s_uSaveStatistics >= thePrefs.GetStatsSaveInterval())
            {
                s_uSaveStatistics = 0;
                thePrefs.SaveStats();
            }
        }
    }
    CATCH_DFLT_EXCEPTIONS(_T("CUploadQueue::UploadTimer"))
}

CUpDownClient* CUploadQueue::GetNextClient(const CUpDownClient* lastclient)
{
    if (waitinglist.IsEmpty())
        return 0;
    if (!lastclient)
        return waitinglist.GetHead();
    POSITION pos = waitinglist.Find(const_cast<CUpDownClient*>(lastclient));
    if (!pos)
    {
        TRACE(L"Error: CUploadQueue::GetNextClient");
        return waitinglist.GetHead();
    }
    waitinglist.GetNext(pos);
    if (!pos)
        return NULL;
    else
        return waitinglist.GetAt(pos);
}

void CUploadQueue::UpdateDatarates()
{
    // Calculate average datarate
    if(::GetTickCount()-m_lastCalculatedDataRateTick > 500)
    {
        m_lastCalculatedDataRateTick = ::GetTickCount();

        if(avarage_dr_list.GetSize() >= 2 && (avarage_tick_list.GetTail() > avarage_tick_list.GetHead()))
        {
            datarate = (UINT)(((m_avarage_dr_sum - avarage_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail() - avarage_tick_list.GetHead()));
            friendDatarate = (UINT)(((avarage_friend_dr_list.GetTail() - avarage_friend_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail() - avarage_tick_list.GetHead()));
        }
    }
}

UINT CUploadQueue::GetDatarate() const
{
    return datarate;
}

UINT CUploadQueue::GetToNetworkDatarate()
{
    if(datarate > friendDatarate)
    {
        return datarate - friendDatarate;
    }
    else
    {
        return 0;
    }
}

void CUploadQueue::ReSortUploadSlots(bool force)
{
    DWORD curtick = ::GetTickCount();
    if(force ||  curtick - m_dwLastResortedUploadSlots >= 10*1000)
    {
        m_dwLastResortedUploadSlots = curtick;

        theApp.uploadBandwidthThrottler->Pause(true);

        CTypedPtrList<CPtrList, CUpDownClient*> tempUploadinglist;

        // Remove all clients from uploading list and store in tempList
        while (!uploadinglist.IsEmpty())
            //WiZaRd: this is not needed because we call InsertInUploadingList below which will call theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->GetFileUploadSocket());
            //Within that function, the proper socket (GetFileUploadSocket()) will be removed prior to adding it to the list!
            tempUploadinglist.AddTail(uploadinglist.RemoveHead());

        // Remove one at a time from temp list and reinsert in correct position in uploading list
        // This will insert in correct place
        while(!tempUploadinglist.IsEmpty())
            InsertInUploadingList(tempUploadinglist.RemoveHead());

        theApp.uploadBandwidthThrottler->Pause(false);
    }
}

void  CUploadQueue::UseHighSpeedUploadTimer(bool bEnable)
{
    if (!bEnable)
    {
        if (m_hHighSpeedUploadTimer != 0)
        {
            KillTimer(0, m_hHighSpeedUploadTimer);
            m_hHighSpeedUploadTimer = 0;
        }
    }
    else
    {
        if (m_hHighSpeedUploadTimer == 0)
            VERIFY( (m_hHighSpeedUploadTimer = SetTimer(0 ,0 , 1, HSUploadTimer)) != 0 );
    }
    DebugLog(_T("%s HighSpeedUploadTimer"), bEnable ? _T("Enabled") : _T("Disabled"));
}

VOID CALLBACK CUploadQueue::HSUploadTimer(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
    // this timer is called every millisecond
    // all we do is feed the uploadslots with data, which is normally done only every 100ms with the big timer
    // the counting, checks etc etc are all done on the normal timer
    // the biggest effect comes actually from the BigBuffer parameter on CreateNextBlockPackage,
    // but beeing able to fetch a request packet up to 1/10 sec earlier gives also a slight speedbump
    for (POSITION pos = theApp.uploadqueue->uploadinglist.GetHeadPosition(); pos != NULL;)
    {
        CUpDownClient* cur_client = theApp.uploadqueue->uploadinglist.GetNext(pos);
        if (cur_client->socket != NULL)
            cur_client->CreateNextBlockPackage(true);
    }
}

UINT CUploadQueue::GetWaitingUserForFileCount(const CSimpleArray<CObject*>& raFiles, bool bOnlyIfChanged)
{
    if (bOnlyIfChanged && !m_bStatisticsWaitingListDirty)
        return (UINT)-1;

    m_bStatisticsWaitingListDirty = false;
    UINT nResult = 0;
    for (POSITION pos = waitinglist.GetHeadPosition(); pos != 0; )
    {
        const CUpDownClient* cur_client = waitinglist.GetNext(pos);
        for (int i = 0; i < raFiles.GetSize(); i++)
        {
            if (md4cmp(((CKnownFile*)raFiles[i])->GetFileHash(), cur_client->GetUploadFileID()) == 0)
                nResult++;
        }
    }
    return nResult;
}

UINT CUploadQueue::GetDatarateForFile(const CSimpleArray<CObject*>& raFiles) const
{
    UINT nResult = 0;
    for (POSITION pos = uploadinglist.GetHeadPosition(); pos != 0; )
    {
        const CUpDownClient* cur_client = uploadinglist.GetNext(pos);
        for (int i = 0; i < raFiles.GetSize(); i++)
        {
            if (md4cmp(((CKnownFile*)raFiles[i])->GetFileHash(), cur_client->GetUploadFileID()) == 0)
                nResult += cur_client->GetDatarate();
        }
    }
    return nResult;
}

//>>> WiZaRd::Small File Slot
//return the estimated transferred size within 10 seconds - that qualifies for a small file
uint64 CUploadQueue::GetSmallFileSize() const
{
    return GetDatarate() * 10;
}
//<<< WiZaRd::Small File Slot
//>>> WiZaRd::ZZUL Upload [ZZ]
/*
//>>> WiZaRd::Dynamic Datarate
UINT CUploadQueue::GetClientDataRateCheck() const
{
    return (GetClientDataRate() * 3) / 2;
}

UINT CUploadQueue::GetClientDataRate() const
{
    UINT maxSpeed = 0;
    if (thePrefs.IsDynUpEnabled())
        maxSpeed = theApp.lastCommonRouteFinder->GetUpload();
    else
    {
        if(thePrefs.GetMaxUpload() == UNLIMITED)
            maxSpeed = thePrefs.GetMaxGraphUploadRate(true); // estimate the limit
        else
            maxSpeed = thePrefs.GetMaxUpload()*1024;
    }

	UINT clientDR = UPLOAD_CLIENT_DATARATE_DFLT;
	if(maxSpeed > 307200)
		clientDR += ((maxSpeed - 307200) / (5 * UPLOAD_CLIENT_DATARATE_DFLT)) * 1024;
	return clientDR;
}
//<<< WiZaRd::Dynamic Datarate
*/
//<<< WiZaRd::ZZUL Upload [ZZ]
//>>> WiZaRd::ZZUL Upload [ZZ]
/**
 * Remove the client from upload socket if there's another client with same/higher
 * class that wants to get an upload socket. If there's not another matching client
 * move this client down in the upload list so that it is after all other of it's class.
 *
 * @param client address of the client that should be removed or moved down
 *
 * @return true if the client was removed. false if it is still in upload list
 */
bool CUploadQueue::RemoveOrMoveDown(CUpDownClient* client, bool onlyCheckForRemove) {
    //if(onlyCheckForRemove == false) {
    //    CheckForHighPrioClient();
    //}


    //CUpDownClient* newclient = FindBestClientInQueue(true, client);

//-
        CUpDownClient* newclient = FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated();

        CUpDownClient* queueNewclient = FindBestClientInQueue(false);

        if(queueNewclient &&
           (
            !newclient ||
            !newclient->GetScheduledUploadShouldKeepWaitingTime() && (newclient->IsFriend() && newclient->GetFriendSlot()) == false && newclient->IsPowerShared() == false ||
            (
			 (queueNewclient->IsFriend() && queueNewclient->GetFriendSlot()) == true && (newclient->IsFriend() && newclient->GetFriendSlot()) == false ||
             (queueNewclient->IsFriend() && queueNewclient->GetFriendSlot()) == (newclient->IsFriend() && newclient->GetFriendSlot()) &&
             (
              queueNewclient->IsPowerShared() == true && newclient->IsPowerShared() == false ||
              queueNewclient->IsPowerShared() == true && newclient->IsPowerShared() == true &&
              (
               queueNewclient->GetFilePrioAsNumber() > newclient->GetFilePrioAsNumber() ||
               queueNewclient->GetFilePrioAsNumber() == newclient->GetFilePrioAsNumber() && !newclient->GetScheduledUploadShouldKeepWaitingTime()
              )
             )
            )
           )
          ) {
            // didn't find a scheduled client, or the one we found
            // wasn't pre-empted, and is not special class client, so shouldn't be unscheduled from removal
            newclient = queueNewclient;
        }

//-

    if(newclient != NULL && // Only remove the client if there's someone to replace it
       (
        (client->IsFriend() && client->GetFriendSlot()) == false && // if it is not in a class that gives it a right
        client->IsPowerShared() == false ||                        // to have a check performed to see if it can stay, we remove at once
        (
         (
          (
           newclient->IsPowerShared() == true && client->IsPowerShared() == false || // new client wants powershare file, but old client don't
           newclient->IsPowerShared() == true && client->IsPowerShared() == true && newclient->GetFilePrioAsNumber() >= client->GetFilePrioAsNumber() // both want powersharedfile, and newer wants higher/same prio file
          ) &&
          (client->IsFriend() && client->GetFriendSlot()) == false
         ) || // old client don't have friend slot
         (newclient->IsFriend() && newclient->GetFriendSlot()) == true // new friend has friend slot, this means it is of highest prio, and will always get this slot
        )
       )
      ){

        // Remove client from ul list to make room for higher/same prio client
	    theApp.uploadqueue->ScheduleRemovalFromUploadQueue(client, _T("Successful completion of upload."), GetResString(IDS_UPLOAD_COMPLETED));

        return true;
    } else if(onlyCheckForRemove == false) {
		MoveDownInUploadQueue(client);

        return false;
    } else {
        return false;
    }
}

void CUploadQueue::MoveDownInUploadQueue(CUpDownClient* client) {
    // first find the client in the uploadinglist
    UINT posCounter = 1;
    POSITION foundPos = NULL;
    POSITION pos = uploadinglist.GetHeadPosition();
	while(pos != NULL) {
        CUpDownClient* curClient = uploadinglist.GetAt(pos);
		if (curClient == client){
            foundPos = pos;
        } else {
            curClient->SetSlotNumber(posCounter);
            posCounter++;
        }

        uploadinglist.GetNext(pos);
	}

    if(foundPos != NULL) {
        // Remove the found Client
		uploadinglist.RemoveAt(foundPos);
        theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);

        // then add it last in it's class
        InsertInUploadingList(client);
    }
}

/**
 * Compares two clients, considering requested file and score (waitingtime, credits, requested file prio), and decides if the right
 * client is better than the left clien. If so, it returns true.
 *
 * Clients are ranked in the following classes:
 *    1: Friends (friends are internally ranked by which file they want; if it is powershared; upload priority of file)
 *    2: Clients that wants powershared files of prio release
 *    3: Clients that wants powershared files of prio high
 *    4: Clients that wants powershared files of prio normal
 *    5: Clients that wants powershared files of prio low
 *    6: Clients that wants powershared files of prio lowest
 *    7: Other clients
 *
 * Clients are then ranked inside their classes by their credits and waiting time (== score).
 *
 * Another description of the above ranking:
 *
 * First sortorder is if the client is a friend with a friend slot. Any client that is a friend with a friend slot,
 * is ranked higher than any client that does not have a friend slot.
 *
 * Second sortorder is if the requested file if a powershared file. All clients wanting powershared files are ranked higher
 * than any client wanting a not powershared filed.
 *
 * If the file is powershared, then second sortorder is file priority. For instance. Any client wanting a powershared file with
 * upload priority high, is ranked higher than any client wanting a powershared file with upload file priority normal.
 *
 * If both clients wants powershared files, and of the same upload priority, then the score is used to decide which client is better.
 * The score, as usual, weighs in the client's waiting time, credits, and requested file's upload priority.
 *
 * If both clients wants files that are not powershared, then scores are used to compare the clients, as in official eMule.
 *
 * @param leftClient a pointer to the left client
 *
 * @param leftScore the precalculated score for leftClient, which is calculated with leftClient->GetSCore()
 *
 * @param rightClient a pointer to the right client
 *
 * @param rightScore the precalculated score for rightClient, which is calculated with rightClient->GetSCore()
 *
 * @return true if right client is better, false if clients are equal. False if left client is better.
 */
bool CUploadQueue::RightClientIsBetter(CUpDownClient* leftClient, UINT leftScore, CUpDownClient* rightClient, UINT rightScore) {
    if(!rightClient) 
        return false;

    bool leftLowIdMissed = false;
    bool rightLowIdMissed = false;
    
    if(leftClient) 
	{
        leftLowIdMissed = leftClient->HasLowID() && leftClient->socket && leftClient->socket->IsConnected() && leftClient->m_bAddNextConnect && leftClient->GetQueueSessionPayloadUp() < SESSIONMAXTRANS;
        rightLowIdMissed = rightClient->HasLowID() && rightClient->socket && rightClient->socket->IsConnected() && rightClient->m_bAddNextConnect && rightClient->GetQueueSessionPayloadUp() < SESSIONMAXTRANS;
    }

    if(
       (leftClient != NULL &&
        (
         (rightClient->IsFriend() && rightClient->GetFriendSlot()) == true && (leftClient->IsFriend() && leftClient->GetFriendSlot()) == false || // rightClient has friend slot, but leftClient has not, so rightClient is better

         (leftClient->IsFriend() && leftClient->GetFriendSlot()) == (rightClient->IsFriend() && rightClient->GetFriendSlot()) && // both or none have friend slot, let file prio and score decide
         (
          leftClient->IsPowerShared() == false && rightClient->IsPowerShared() == true || // rightClient wants powershare file, but leftClient not, so rightClient is better

          leftClient->IsPowerShared() == true && rightClient->IsPowerShared() == true && // they both want powershare file
          leftClient->GetFilePrioAsNumber() < rightClient->GetFilePrioAsNumber() || // and rightClient wants higher prio file, so rightClient is better

          (
           leftClient->GetFilePrioAsNumber() ==  rightClient->GetFilePrioAsNumber() || // same prio file
           leftClient->IsPowerShared() == false && rightClient->IsPowerShared() == false //neither want powershare file
          ) && // they are equal in powersharing
          (
           !leftLowIdMissed && rightLowIdMissed || // rightClient is lowId and has missed a slot and is currently connected

           leftLowIdMissed && rightLowIdMissed && // both have missed a slot and both are currently connected
           leftClient->m_bAddNextConnect > rightClient->m_bAddNextConnect || // but right client missed earlier

           (
            !leftLowIdMissed && !rightLowIdMissed || // neither have both missed and is currently connected

            leftLowIdMissed && rightLowIdMissed && // both have missed a slot
            leftClient->m_bAddNextConnect == rightClient->m_bAddNextConnect // and at same time (should hardly ever happen)
           ) &&
           rightScore > leftScore // but rightClient has better score, so rightClient is better
          )
         )
        ) ||
        leftClient == NULL // there's no old client to compare with, so rightClient is better (than null)
       ) &&
       (!rightClient->IsBanned()) && // don't allow banned client to be best
       IsDownloading(rightClient) == false // don't allow downloading clients to be best
      ) 
		return true;
	
	return false;
}


/* Insert the client at the correct place in the uploading list.
	* The client should be inserted after all of its class, but before any
	* client of a lower ranking class.
	*
	* Clients are ranked in the following classes:
	*    1: Friends (friends are internally ranked by which file they want; if it is powershared; upload priority of file)
	*    2: Clients that wants powershared files of prio release
	*    3: Clients that wants powershared files of prio high
	*    4: Clients that wants powershared files of prio normal
	*    5: Clients that wants powershared files of prio low
	*    6: Clients that wants powershared files of prio lowest
	*    7: Other clients
	*
	* Since low ID clients are only put in an upload slot when they call us, it means they will
	* have to wait about 10-30 minutes longer to be put in an upload slot than a high id client.
	* In that time, the low ID client could possibly have gone from being a trickle slot, into
	* being a fully activated slot. At the time when the low ID client would have been put into an
	* upload slot, if it had been a high id slot, a boolean flag is set to true (AddNextConnect = true).
	*
	* A client that has AddNextConnect set when it calls back, will immiediately be given an upload slot.
	* When it is added to the upload list with this method, it will also get the time when it entered the
	* queue taken into consideration. It will be added so that it is before all clients (within in its class) 
	* that entered queue later than it. This way it will be able to possibly skip being a trickle slot,
	* since it has already been forced to wait extra time to be put in a upload slot. This makes the
	* low ID clients have almost exactly the same bandwidth from us (proportionally to the number of low ID
	* clients compared to the number of high ID clients) as high ID clients. This is a definitely a further
	* improvement of VQB's excellent low ID handling.
	*
	* @param newclient address of the client that should be inserted in the uploading list
*/
void CUploadQueue::InsertInUploadingList(CUpDownClient* newclient) 
{
//>>> WiZaRd::Payback First
    if(IsPayback(newclient))
        m_lPaybackList.AddTail(newclient);
//<<< WiZaRd::Payback First

	POSITION insertPosition = NULL;
	UINT posCounter = uploadinglist.GetCount();

	bool foundposition = false;
	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL && !foundposition) 
	{
		CUpDownClient* uploadingClient = uploadinglist.GetAt(pos);

		if(!uploadingClient->IsScheduledForRemoval() && newclient->IsScheduledForRemoval() ||
			uploadingClient->IsScheduledForRemoval() == newclient->IsScheduledForRemoval() &&
			(
			uploadingClient->HasSmallFileUploadSlot() && !newclient->HasSmallFileUploadSlot() ||
			uploadingClient->HasSmallFileUploadSlot() == newclient->HasSmallFileUploadSlot() &&
			(
			(uploadingClient->IsFriend() && uploadingClient->GetFriendSlot()) && !(newclient->IsFriend() && newclient->GetFriendSlot()) ||
			(uploadingClient->IsFriend() && uploadingClient->GetFriendSlot()) == (newclient->IsFriend() && newclient->GetFriendSlot()) &&
			(
			uploadingClient->IsPowerShared() && !newclient->IsPowerShared() ||
			uploadingClient->IsPowerShared() && newclient->IsPowerShared() && uploadingClient->GetFilePrioAsNumber() > newclient->GetFilePrioAsNumber() ||
			(
			uploadingClient->IsPowerShared() && newclient->IsPowerShared() && uploadingClient->GetFilePrioAsNumber() == newclient->GetFilePrioAsNumber() ||
			!uploadingClient->IsPowerShared() && !newclient->IsPowerShared()
			) &&
			(
			//uploadingClient->GetDatarate() > newclient->GetDatarate() ||
			//uploadingClient->GetDatarate() == newclient->GetDatarate() &&
			!newclient->IsScheduledForRemoval() &&
			(
				!newclient->HasLowID() ||
				!newclient->m_bAddNextConnect ||
				newclient->GetQueueSessionPayloadUp() >= SESSIONMAXTRANS ||
				newclient->HasLowID() && newclient->m_bAddNextConnect &&
				newclient->GetUpStartTimeDelay() + (::GetTickCount()-newclient->m_bAddNextConnect) <= uploadingClient->GetUpStartTimeDelay() + ((uploadingClient->HasLowID() && uploadingClient->m_bAddNextConnect && uploadingClient->GetQueueSessionPayloadUp() < SESSIONMAXTRANS) ? (::GetTickCount()-uploadingClient->m_bAddNextConnect) : 0)
			) ||
			newclient->IsScheduledForRemoval() &&
			uploadingClient->IsScheduledForRemoval() &&
			uploadingClient->GetScheduledUploadShouldKeepWaitingTime() &&
			!newclient->GetScheduledUploadShouldKeepWaitingTime()
			)
			)
			)
			)
			) {
				foundposition = true;
		}
		else 
		{
			insertPosition = pos;
			uploadinglist.GetPrev(pos);
			posCounter--;
		}
	}

	if(insertPosition != NULL) 
	{
		POSITION renumberPosition = insertPosition;
		UINT renumberSlotNumber = posCounter+1;

		while(renumberPosition != NULL) 
		{
			CUpDownClient* renumberClient = uploadinglist.GetAt(renumberPosition);

			renumberSlotNumber++;

			renumberClient->SetSlotNumber(renumberSlotNumber);
			renumberClient->UpdateDisplayedInfo(true);

			theApp.emuledlg->transferwnd->GetUploadList()->RefreshClient(renumberClient);

			uploadinglist.GetNext(renumberPosition);
		}

		// add it at found pos
		newclient->SetSlotNumber(posCounter+1);
		uploadinglist.InsertBefore(insertPosition, newclient);
		newclient->UpdateDisplayedInfo(true);
		theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->GetFileUploadSocket());
	}
	else 
	{
		// Add it last
		theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket());
		uploadinglist.AddTail(newclient);
		newclient->SetSlotNumber(uploadinglist.GetCount());
	}
}

CUpDownClient* CUploadQueue::FindLastUnScheduledForRemovalClientInUploadList() const
{
	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL)
	{
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetPrev(pos);

		if(!cur_client->IsScheduledForRemoval())
			return cur_client;
	}

	return NULL;
}

CUpDownClient* CUploadQueue::FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated() const
{
	POSITION pos = uploadinglist.GetHeadPosition();
	while(pos != NULL)
	{
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);

		if(cur_client->IsScheduledForRemoval() /*&& cur_client->GetScheduledUploadShouldKeepWaitingTime()*/)
			return cur_client;
	}

	return NULL;
}

UINT CUploadQueue::GetEffectiveUploadListCount() const
{
	UINT count = 0;

	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL){
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetPrev(pos);

		if(!cur_client->IsScheduledForRemoval())
			pos = NULL;
		else
			++count;
	}

	return uploadinglist.GetCount()-count;
}

bool CUploadQueue::AddUpNextClient(LPCTSTR pszReason, CUpDownClient* directadd, bool highPrioCheck) 
{
	CUpDownClient* newclient = NULL;
	// select next client or use given client
	if (!directadd)
	{
		if(!highPrioCheck)
			newclient = FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated();

		CUpDownClient* queueNewclient = FindBestClientInQueue(highPrioCheck == false, newclient);

		if(queueNewclient &&
			(
			!newclient ||
			!newclient->GetScheduledUploadShouldKeepWaitingTime() && (newclient->IsFriend() && newclient->GetFriendSlot()) == false && newclient->IsPowerShared() == false ||
			(
			(queueNewclient->IsFriend() && queueNewclient->GetFriendSlot()) == true && (newclient->IsFriend() && newclient->GetFriendSlot()) == false ||
			(queueNewclient->IsFriend() && queueNewclient->GetFriendSlot()) == (newclient->IsFriend() && newclient->GetFriendSlot()) &&
			(
			queueNewclient->IsPowerShared() == true && newclient->IsPowerShared() == false ||
			queueNewclient->IsPowerShared() == true && newclient->IsPowerShared() == true &&
			(
			queueNewclient->GetFilePrioAsNumber() > newclient->GetFilePrioAsNumber() ||
			queueNewclient->GetFilePrioAsNumber() == newclient->GetFilePrioAsNumber() && !newclient->GetScheduledUploadShouldKeepWaitingTime()
			)
			)
			)
			)
			) {
				// didn't find a scheduled client, or the one we found
				// wasn't pre-empted, and is not special class client, so shouldn't be unscheduled from removal
				newclient = queueNewclient;
		}

		if(newclient) 
		{
			if(highPrioCheck) 
			{
				if((newclient->IsFriend() && newclient->GetFriendSlot()) || newclient->IsPowerShared()) 
				{
					CUpDownClient* lastClient = FindLastUnScheduledForRemovalClientInUploadList();

					if(lastClient != NULL) {
						if (
							(newclient->IsFriend() && newclient->GetFriendSlot()) == true && (lastClient->IsFriend() && lastClient->GetFriendSlot()) == false ||
							(newclient->IsFriend() && newclient->GetFriendSlot()) == (lastClient->IsFriend() && lastClient->GetFriendSlot()) &&
							(
							newclient->IsPowerShared() == true && lastClient->IsPowerShared() == false ||
							newclient->IsPowerShared() == true && lastClient->IsPowerShared() == true && newclient->GetFilePrioAsNumber() > lastClient->GetFilePrioAsNumber()
							)
							) {
								// Remove last client from ul list to make room for higher prio client
								ScheduleRemovalFromUploadQueue(lastClient, _T("Ended upload to make room for higher prio client."), GetResString(IDS_UPLOAD_PREEMPTED), true);
						} 
						else 
							return false;						
					}
				} 
				else 
					return false;
			}

			if(!IsDownloading(newclient) && !newclient->IsScheduledForRemoval()) 
			{
				RemoveFromWaitingQueue(newclient, true);
				theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
				//} else {
				//    newclient->UnscheduleForRemoval();
				//    MoveDownInUploadQueue(newclient);
			}
		}
	}
	else 
		newclient = directadd;

	if(newclient == NULL) 
		return false;

	if (IsDownloading(newclient))
	{
		if(newclient->IsScheduledForRemoval()) 
		{
			newclient->UnscheduleForRemoval();
			m_nLastStartUpload = ::GetTickCount();

			MoveDownInUploadQueue(newclient);

			if(pszReason && thePrefs.GetLogUlDlEvents())
				AddDebugLogLine(false, _T("Unscheduling client from being removed from upload list: %s Client: %s File: %s"), pszReason, newclient->DbgGetClientInfo(), (theApp.sharedfiles->GetFileByID(newclient->GetUploadFileID())?theApp.sharedfiles->GetFileByID(newclient->GetUploadFileID())->GetFileName():_T("")));
			return true;
		}

		return false;
	}

//>>> WiZaRd::Optimization
    CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
    if (reqfile == NULL)
    {
        if(thePrefs.GetLogUlDlEvents())
            AddDebugLogLine(false, L"Tried to add client %s to upload list but reqfile wasn't found - prevented failed UL session!", newclient->DbgGetClientInfo());
        return false;
    }
//<<< WiZaRd::Optimization

	if(pszReason && thePrefs.GetLogUlDlEvents())
		AddDebugLogLine(false, _T("Adding client to upload list: %s Client: %s File: %s"), pszReason, newclient->DbgGetClientInfo(), (theApp.sharedfiles->GetFileByID(newclient->GetUploadFileID())?theApp.sharedfiles->GetFileByID(newclient->GetUploadFileID())->GetFileName():_T("")));

	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected())
	{
		newclient->SetUploadState(US_CONNECTING);
		if (!newclient->TryToConnect(true))
			return false;
	}
	else
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__AcceptUploadReq", newclient);
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theStats.AddUpDataOverheadFileRequest(packet->size);
		newclient->SendPacket(packet, true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();

	InsertInUploadingList(newclient);

	m_nLastStartUpload = ::GetTickCount();

	if(newclient->GetQueueSessionUp() > 0) {
		// This client has already gotten a successfullupcount++ when it was early removed.
		// Negate that successfullupcount++ so we can give it a new one when this session ends
		// this prevents a client that gets put back first on queue, being counted twice in the
		// stats.
		successfullupcount--;
		theStats.DecTotalCompletedBytes(newclient->GetQueueSessionUp());
	}

	// statistic
	reqfile->statistic.AddAccepted();

	theApp.emuledlg->transferwnd->GetUploadList()->AddClient(newclient);

	return true;
}

bool CUploadQueue::AcceptNewClient() const
{
	UINT curUploadSlots = (UINT)GetEffectiveUploadListCount();
	return AcceptNewClient(curUploadSlots);
}

void CUploadQueue::ScheduleRemovalFromUploadQueue(CUpDownClient* client, LPCTSTR pszDebugReason, CString strDisplayReason, bool earlyabort) 
{
	if (thePrefs.GetLogUlDlEvents())
		AddDebugLogLine(DLP_VERYLOW, true,_T("Scheduling to remove client from upload list: %s Client: %s Transfered: %s SessionUp: %s QueueSessionUp: %s QueueSessionPayload: %s File: %s"), pszDebugReason==NULL ? _T("") : pszDebugReason, client->DbgGetClientInfo(), CastSecondsToHM( client->GetUpStartTimeDelay()/1000), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false), (theApp.sharedfiles->GetFileByID(client->GetUploadFileID())?theApp.sharedfiles->GetFileByID(client->GetUploadFileID())->GetFileName():_T("")));

	client->ScheduleRemovalFromUploadQueue(pszDebugReason, strDisplayReason, earlyabort);
	MoveDownInUploadQueue(client);

	m_nLastStartUpload = ::GetTickCount();
}

UINT CUploadQueue::GetWantedNumberOfTrickleUploads() const
{
	UINT minNumber = MINNUMBEROFTRICKLEUPLOADS;

	if(minNumber < 1 && GetDatarate() >= 2*1024)
		minNumber = 1;

	return max((UINT)(GetEffectiveUploadListCount()*0.1), minNumber);
}

void CUploadQueue::CheckForHighPrioClient() 
{
	// PENDING: Each 3 seconds
	DWORD curTick = ::GetTickCount();
	if(curTick - m_dwLastCheckedForHighPrioClient >= 3*1000) 
	{
		m_dwLastCheckedForHighPrioClient = curTick;

		bool added = true;
		while(added)
			added = AddUpNextClient(_T("High prio client (i.e. friend/powershare)."), NULL, true);
	}
}

UINT CUploadQueue::GetActiveUploadsCountLongPerspective() const
{
	return m_MaxActiveClients;
}
//<<< WiZaRd::ZZUL Upload [ZZ]