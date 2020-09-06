/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

#include <conf.h>
#include <mydefs.h>

/* Copyright (c) 1998 by QUALCOMM Incorporated */
#define	FILE_NUM 119

/**********************************************************************
 *	imapconnections.c
 *
 *		This file contains the functions that manage IMAP connections
 **********************************************************************/

#include "imapconnections.h" 
#include "myssl.h"

// connection management
short CountConnections(PersHandle pers);
void ZapIMAPConnectionHandle(IMAPConnectionHandle *node);
IMAPConnectionHandle FindConnectionFromStream(IMAPStreamPtr imapStream);
void CloseImapStream(IMAPConnectionHandle node);
void CloseIMAPStreamSilently(IMAPConnectionHandle node);
Boolean AnyThreadsRunning(void);

/************************************************************************
 *	EnsureConnectionPool - build the connection pool for a given pers
 ************************************************************************/
OSErr EnsureConnectionPool(PersHandle pers)
{
	OSErr err = noErr;
	PersHandle oldPers = CurPers;
	long numConnections;
	IMAPConnectionHandle node;
	Str255 user, host;
	
	CurPers = pers;
	
	numConnections = GetRLong(IMAP_MAX_CONNECTIONS) - CountConnections(CurPers);
	GetPOPInfo(user,host);
	
	// create as many connections as we need for this personality
	while(numConnections > 0 && (err==noErr))
	{
		node = NewZH(IMAPConnectionStruct);
		if (node)
		{
			// create the imapstream when we first use this node ...
			(*node)->owner = (*CurPers)->persId;
			(*node)->lifeTime = GetRLong(IMAP_MAIN_CON_TIMEOUT);
			
			LL_Queue(gIMAPConnectionPool, node, (IMAPConnectionHandle));
		}
		else
		{
			WarnUser(MEM_ERR, err=MemError());
			break;
		}
		numConnections--;
	}
	CurPers = oldPers;
	
	return (err);
}

/************************************************************************
 *	CountConnections - return the number of existing connections to a 
 *	 the server used by a given personality.
 ************************************************************************/
short CountConnections(PersHandle pers)
{
	short result = 0;
	PersHandle oldPers = CurPers;
	IMAPConnectionHandle node = gIMAPConnectionPool, next = nil;
	Str255 user, host;
	long port;
			
	CurPers = pers;
	port = GetRLong(IMAP_PORT);
	GetPOPInfoLo(user,host,&port);
	CurPers = oldPers;

	while (node)
	{	
		next = (*node)->next;
		
		// if this node is owned by the given personality ...
		if ((*node)->owner == (*pers)->persId)
		{
			result++;
		}
		
		// next node ...
		node = next;
	}
	
	return (result);
} 

/************************************************************************
 *	FindConnectionFromStream - given an IMAP stream, locate the connection
 *	 node that wraps it.
 ************************************************************************/
IMAPConnectionHandle FindConnectionFromStream(IMAPStreamPtr imapStream)
{
	IMAPConnectionHandle node = gIMAPConnectionPool;
	
	while (node)
	{
		if ((*node)->imapStream == imapStream) return (node);
		else node = (*node)->next;
	}
	
	return (nil);
}

/************************************************************************
 *	CleanupConnection - clean up a connection so someone else can use it.
 ************************************************************************/
void CleanupConnection(IMAPStreamPtr *imapStream)
{
	IMAPConnectionHandle node = nil;
	
	// must actually have a stream to close ...
	if (imapStream && *imapStream)
	{		
		if (node = FindConnectionFromStream(*imapStream))
		{
			// cleanup alerts and display any new ones
			IMAPAlert(*imapStream, (*node)->task);
		
			if ((*node)->imapStream->mailStream && (*node)->imapStream->mailStream->refN > 0) 
			{
				MyFSClose((*node)->imapStream->mailStream->refN);
				(*node)->imapStream->mailStream->refN = -1;
			}

	#ifdef	DEBUG
			if ((*node)->imapStream->mailStream && (*node)->imapStream->mailStream->flagsRefN > 0) 
			{
				MyFSClose((*node)->imapStream->mailStream->flagsRefN);
				(*node)->imapStream->mailStream->flagsRefN = -1;
			}
	#endif
		
			(*node)->lastUsed = TickCount();
			(*node)->task = UndefinedTask;
			
			// unlock this node
			(*node)->inUse = false;
			UL(node);
		}
		else
		{
			// the node for this stream was not found.  Kill the imapstream.
			ZapImapStream(imapStream);
		}
		
		*imapStream = nil;
	}
}

/************************************************************************
 *	GetIMAPConnection - return the first unused IMAP connection for
 *	 the current personality.  Sit and wait until one is available.
 ************************************************************************/
IMAPStreamPtr GetIMAPConnectionLo(TaskKindEnum forWhat, Boolean progress, Boolean bPerformUpdate)
{
	IMAPStreamPtr stream = nil;
	IMAPConnectionHandle node = nil;
	long numConnections = GetRLong(IMAP_MAX_CONNECTIONS);
	Str255 user, host;
	OSErr err = noErr;
	long ticks = TickCount();
	Boolean progressed = false;
	Boolean connectionBusy = false;	// set to true to allow only one type of this connection
	long port;
	
#ifdef ESSL
	if(GetPrefLong(PREF_SSL_IMAP_SETTING) & esslUseAltPort)
		port = GetRLong(IMAP_SSL_PORT);
	else
#endif
		port = GetRLong(IMAP_PORT);
	HesOK = True;	// force re-fetch of hesiod info				
	GetPOPInfoLo(user, host, &port);
	HesOK = False;
			
	// user specified unlimited connections.
	if (numConnections < 1)	
	{
		// create a new one.
		NewImapStream(&stream, host, port);
		if (stream == nil)
		{
			WarnUser(MEM_ERR, MemError());
		}
	}
	else
	{
		// locate an available connection
		while (!CommandPeriod && !stream)
		{
			// make sure there's an ass to pull a stream out of ...
			EnsureConnectionPool(CurPers);
		
			// Allow only one resync, fetch, or append at a time, if we're being smart.
			connectionBusy = false;
			if (!PrefIsSet(PREF_IMAP_NO_CONNECTION_MANAGEMENT))
			{
				if ((forWhat==IMAPResyncTask) || (forWhat==IMAPFetchingTask) 
					|| (forWhat==IMAPAttachmentFetch) || (forWhat==IMAPAppendTask)
					|| (forWhat == IMAPMultResyncTask))
				{
					node = gIMAPConnectionPool;
					while (node && !stream)
					{
						// there's one running
						if ((*node)->task == forWhat) 
						{
							// always allow one message text fetch per personality
							if ((forWhat!=IMAPFetchingTask) || ((*node)->owner==(*CurPers)->persId))
							{
								connectionBusy = true;
								break;
							}
						}
						
						// next node
						node = (*node)->next;
					}
				}
			}

			if (!connectionBusy)
			{
				node = gIMAPConnectionPool;
				while (node && !stream)
				{
					if (((*node)->owner == (*CurPers)->persId))
					{	
						// make sure the node, if in use, really is.
						if ((*node)->inUse)
						{			
							// this node was found to be in use.  Are there any threads running?
							if (!AnyThreadsRunning())
							{
								// there are no threads, or we're in the only thread running.
								(*node)->inUse = false;
								UL(node);
							}
						}
						
						// if the node is not in use ...
						if (!(*node)->inUse)
						{
							// lock it now.  No one else my touch it.
							LDRef(node);
							(*node)->inUse = true;

							// Can we reuse this stream?
							if ((*node)->imapStream)
							{
								if ((port != (*node)->imapStream->portNumber) 
								 || (!StringSame(host, (*node)->imapStream->pServerName)) 
								 || (*node)->rude
								 ||	(*node)->dontReuse)
								{
									// force us to reconnect to the server
									CloseImapStream(node);
									
									// this connection CAN be reused in the future
									(*node)->dontReuse = false;	
								}	
							}
							
							// Ping this connection to make sure it's still alive
							if ((*node)->imapStream)
							{
								// do the ping silently.  If it fails, we'll create a new stream.
								if ((*node)->imapStream->mailStream && (*node)->imapStream->mailStream->transStream)
									(*node)->imapStream->mailStream->transStream->BeSilent = true;
									
								if (!Noop((*node)->imapStream))
								{
									// the ping failed.  Kill the mailStream, which kills the network stream.
									CloseImapStream(node);
								}
							}
							
							// create a new conneciton if we need one.
							if ((*node)->imapStream == nil)
							{
								if ((err=NewImapStream(&(*node)->imapStream, host, port))!=noErr)
								{
									// unlock this node.
									(*node)->inUse = false;
									UL(node);
									
									// couldn't get a stream.
									WarnUser(MEM_ERR, err);
									break;
								}
							}
							
							// what is this connection going to be doing?
							(*node)->task = forWhat;

							// remember when this stream was last used
							(*node)->lastUsed = TickCount();
							
							stream = (*node)->imapStream;
							break;
						}				
						
					}
					node = (*node)->next;
				}
			}
											
			// put up a progress message if we've waiting for more than a second ...
			if (!progressed && ((TickCount() - ticks)>60))
			{
				PROGRESS_MESSAGER(kpMessage,IMAP_WAITING_FOR_CONNECTION);
				
				progressed = true;
			}
								
			CycleBalls();
			if (MyYieldToAnyThread()) break;
		}
	}

	// processed all queued IMAP commands for this personality
	if (bPerformUpdate)
		PerformQueuedCommands(CurPers, stream, progress);
	
	return (stream);
}

/************************************************************************
 *	CheckIMAPConnections - close all the connections to all the servers
 *	 if the user hasn't done anything for the specified amount of
 *	 time.
 ************************************************************************/
void CheckIMAPConnections(void)
{
	PersHandle oldPers = CurPers;
	long timeOut, secondaryTimeOut;
	IMAPConnectionHandle node;
	long now = TickCount();		
	Boolean foundMainConnection;
	
	for (CurPers = PersList; CurPers; CurPers = (*CurPers)->next)
	{
		timeOut = GetRLong(IMAP_MAIN_CON_TIMEOUT);					// timeOut for main IMAP connection
		secondaryTimeOut = GetRLong(IMAP_SECONDARY_CON_TIMEOUT);	// timeOut for all other IMAP connections
		foundMainConnection = false;								// set to true once we've processed the main connection
		
		// look at the connections for this personality ...
		for (node = gIMAPConnectionPool; node; node = (*node)->next)
		{
			// if this connection belongs to the current personality ...
			if ((*node)->owner == (*CurPers)->persId)
			{
				// and if it's not in use ...
				if (!(*node)->inUse)
				{
					// and if a connection has been made through it once before ...
					if ((*node)->imapStream != nil)
					{
						// and it hasn't been used for the last <timeOut> seconds ...
						if (((*node)->lastUsed + 60*(foundMainConnection?secondaryTimeOut:timeOut))< now)
						{
							CloseIMAPStreamSilently(node);
						}
					}
				}
				foundMainConnection = true;	// we've processed the first/main connection for this personality
			}
		}	
	}
	
	CurPers = oldPers;
}

/************************************************************************
 *	ZapIMAPConnectionHandle - zap a connection node
 ************************************************************************/
void ZapIMAPConnectionHandle(IMAPConnectionHandle *node)
{		
	// nothing to do if nothing to zap ...
	if (!node || !*node) return;
		
	if (**node)
	{		
		// lock this node, no one else may touch it.
		LDRef(*node);
		(**node)->inUse = true;

		// if there's a network stream leftover, make sure it's silent when we kill it.
		if ((**node)->imapStream && (**node)->imapStream->mailStream && (**node)->imapStream->mailStream->transStream)
			(**node)->imapStream->mailStream->transStream->BeSilent = true;
		
		// now zap the IMAP stream, including the network stream.
		ZapImapStream(&(**node)->imapStream);
		
		// unlock the node.
		(**node)->inUse = false;
		UL(*node);
	}	
	
	ZapHandle(*node);
	*node = nil;
}

/************************************************************************
 *	ZapAllIMAPConnections - drain the IMAP connection pool
 ************************************************************************/
void ZapAllIMAPConnections(Boolean force)
{
	IMAPConnectionHandle node = gIMAPConnectionPool;
	IMAPConnectionHandle next;
	
	while (node)
	{
		next = (*node)->next;
		
		if (!(*node)->inUse || force)
		{
			LL_Remove(gIMAPConnectionPool,node,(IMAPConnectionHandle));
			ZapIMAPConnectionHandle(&node);
		}
		
		node = next;
	}
}

/************************************************************************
 * PrepareToExpunge - Close down idle connections to the mailbox.
 *	This makes expunging on servers that require exclusive access
 *	more likely to succeed.
 ************************************************************************/
void PrepareToExpunge(IMAPStreamPtr imapStream)
{
	IMAPConnectionHandle node = gIMAPConnectionPool;
	IMAPConnectionHandle next;
	
	while (node)
	{
		next = (*node)->next;
		if (((*node)->owner == (*CurPers)->persId))
		{
			if (!(*node)->inUse && (*node)->imapStream)
			{
				// idle connection found.  Is it connected to the same mailbox?
				if (!strcmp(imapStream->mailboxName, (*node)->imapStream->mailboxName))
					CloseIMAPStreamSilently(node);
			}
		}		
		node = next;
	}
}

/************************************************************************
 * CloseImapStream - actually close down an imap connetion
 ************************************************************************/
void CloseImapStream(IMAPConnectionHandle node)
{
	(*node)->rude = false;
	ZapImapStream(&(*node)->imapStream);
}

/************************************************************************
 * CloseIMAPStreamSilently - do it silently
 ************************************************************************/
void CloseIMAPStreamSilently(IMAPConnectionHandle node)
{
	if ((*node)->imapStream)
	{
		// silent killing ...
		if ((*node)->imapStream->mailStream && (*node)->imapStream->mailStream->transStream)
			(*node)->imapStream->mailStream->transStream->BeSilent = true;

		// close the connection to the server.
		LDRef(node);
		(*node)->inUse = true;
		CloseImapStream(node);
		(*node)->inUse = false;
		UL(node);
	}
}

/************************************************************************
 * IMAPRudeConnectionClose - mark a connection so it gets slammed closed
 *	rudely next time it's used.  This will abort big operations.
 ************************************************************************/
void IMAPRudeConnectionClose(IMAPStreamPtr imapStream)
{
	IMAPConnectionHandle node;
	
	if (!PrefIsSet(PREF_IMAP_POLITE_LOGOUT))
		if ((node = FindConnectionFromStream(imapStream))!=nil)
			(*node)->rude = true;
}

/************************************************************************
 * IMAPInvalidatePerConnections - cause a personalities IMAP connections
 *	to be reestablished.  Necessary if certain settings change, or if
 *	passwords are forgotten.
 ************************************************************************/
void IMAPInvalidatePerConnections(PersHandle pers)
{
	IMAPConnectionHandle node;
			
	// look at the connections for this personality ...
	for (node = gIMAPConnectionPool; node; node = (*node)->next)
	{
		// if this connection belongs to the current personality ...
		if ((*node)->owner == (*pers)->persId)
		{
			// and this connection is up and running ...
			if ((*node)->imapStream != nil)
			{
				if ((*node)->inUse)
				{
					// this connection is currently being used.  Let it finish,
					// but don't reuse it
					(*node)->dontReuse = true;
				}
				else CloseIMAPStreamSilently(node);
			}
		}
	}	
}

/************************************************************************
 * AnyThreadsRunning - are there any threads running that might be using
 *	a connection?
 ************************************************************************/
Boolean AnyThreadsRunning(void)
{
	int numThreads = GetNumBackgroundThreads();
	
	// if there's only one thread running and it's a filter progress 
	// thread, we can ignore it.  It doesn't use any connections.
	if (numThreads  == 1)
		if (IsIMAPOperationUnderway(IMAPFilterTask))
			numThreads = 0;
	
	return (numThreads != 0);
}
