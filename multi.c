/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <conf.h>
#include <mydefs.h>

#include "multi.h"
#define FILE_NUM 147

OSErr MultiReferences(TOCHandle tocH, MessHandle messH);
OSErr MultiBody(TOCHandle tocH, MessHandle messH);
short BoxFindVectorMID(uShort ** vector, TOCHandle tocH, uLong value);
DecBVCompare(msgIdHash)
void ReplyCCOFilter(ConConOutDescPtr thisODP, ConConMessElmPtr thisMEP,
		    MessHandle messH, ConConElmH cceh, StackHandle stack,
		    PETEHandle pte, void *userData);


/**********************************************************************
 * DoReplyMultiple - reply to multiple messages, making a single reply
 **********************************************************************/
OSErr DoReplyMultiple(TOCHandle tocH, short modifiers)
{
	MyWindowPtr newWin;
	WindowPtr newWinWP;
	MessHandle newMessH;
	short sumNum;
	Boolean all, self, quote;
	HeadSpec hs;
	Handle text;
	Accumulator addrAcc;
	Str255 scratch;
	OSErr err = noErr;
	Boolean cacheReplyTo = HasFeature(featureNicknameWatching)
	    && !PrefIsSet(PREF_NICK_CACHE)
	    && !PrefIsSet(PREF_NICK_CACHE_NOT_ADD_REPLY_TO);

	Zero(addrAcc);

	//
	// Set the proper personality
	PushPers(MultiPers(tocH));

	//
	// Figure out what kind of reply
	ReplyDefaults(modifiers, &all, &self, &quote);

	////////////////////////
	// Make the new window
	if (newWin = DoComposeNew(0)) {
		newMessH = Win2MessH(newWin);
		newWinWP = GetMyWindowWindowPtr(newWin);

		////////////////////////
		// IMAP; sigh
		for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
			if ((*tocH)->sums[sumNum].selected
			    && !EnsureMsgDownloaded(tocH, sumNum, false)) {
				CloseMyWindow(newWin);
				goto done;
			}
		////////////////////////
		// Copy the addresses from each selected message
		for (sumNum = 0; sumNum < (*tocH)->count && !err; sumNum++)
			if ((*tocH)->sums[sumNum].selected) {
				// grab the message text
				CacheMessage(tocH, sumNum);
				if (text = (*tocH)->sums[sumNum].cache) {
					HNoPurge(text);

					// put all our addresses in the address accumulator, so that
					// we ignore them, if the user is so inclined
					if (!self)
						AddSelfAddrHashes
						    (&addrAcc);

					// grab the "from" header for reply
					if (HandleHeadFindReply(text, &hs))
						HandleHeadCopyAddrs(text,
								    &hs,
								    newMessH,
								    TO_HEAD,
								    &addrAcc,
								    cacheReplyTo);

					// how about the other recipients?
					if (all) {
						if (HandleHeadFindStr
						    (text,
						     HeaderName(TO_HEAD),
						     &hs))
							HandleHeadCopyAddrs
							    (text, &hs,
							     newMessH,
							     PrefIsSet
							     (PREF_CC_REPLY)
							     ? CC_HEAD :
							     TO_HEAD,
							     &addrAcc,
							     false);
						if (HandleHeadFindStr
						    (text,
						     HeaderName(CC_HEAD),
						     &hs))
							HandleHeadCopyAddrs
							    (text, &hs,
							     newMessH,
							     CC_HEAD,
							     &addrAcc,
							     false);
					}

					HPurge(text);
				}
			}
		////////////////////////
		// Subject
		MultiSubj(tocH, scratch);
		if (!ReMatch(scratch, Re)) {
			PInsert(scratch, sizeof(scratch), Re, scratch + 1);
			PInsertC(scratch, sizeof(scratch), ' ',
				 scratch + 1 + *Re);
		}
		CompHeadSetIndexStr((*newMessH)->bodyPTE, SUBJ_HEAD,
				    scratch);

		////////////////////////
		// Do the references (ouch)
		if (!err)
			err = MultiReferences(tocH, newMessH);

		////////////////////////
		// The bodies.  Yowza yowza yowza
		if (!err)
			err = MultiBody(tocH, newMessH);

		if (err) {
			PeteCleanList(newWin->pteList);
			newWin->isDirty = false;
			CloseMyWindow(newWin);
			goto done;
		}
		////////////////////////
		// Make it visible
		UpdateSum(newMessH, SumOf(newMessH)->offset,
			  SumOf(newMessH)->length);
		ShowMyWindow(newWinWP);
		UpdateMyWindow(newWinWP);
		newWin->isDirty = False;
		PeteCleanList(newWin->pte);

		////////////////////////
		// change the states of the original messages
		for (sumNum = (*tocH)->count - 1; sumNum >= 0; sumNum--)
			if ((*tocH)->sums[sumNum].selected) {
				AccuAddLong(&(*newMessH)->aSourceMID,
					    (*tocH)->sums[sumNum].uidHash);
				AccuAddLong(&(*newMessH)->aSourceMID,
					    (*tocH)->sums[sumNum].state);
				AccuAddLong(&(*newMessH)->aSourceMID,
					    REPLIED);
				SetState(tocH, sumNum, REPLIED);
			}
	}

      done:

	PopPers();
	return err;
}

/************************************************************************
 * MultiPers - return the personality that should be used for a reply to
 *   multiple messages
 ************************************************************************/
PersHandle MultiPers(TOCHandle tocH)
{
	// short the selected messages by personality
	uShort **vector = BoxSortVector(tocH, true, BVCompare_persId);
	uShort count = HandleCount(vector);
	// figure out which one is used most
	uShort winnerCount;
	uShort winnerI =
	    MultiMost(vector, tocH, BVCompare_subj, &winnerCount);
	uShort sumNum = (*vector)[winnerI];

	return (PERS_FORCE(FindPersById((*tocH)->sums[sumNum].persId)));
}

/************************************************************************
 * MultiSubj - return the subject that should be used for a reply to
 *   multiple messages
 ************************************************************************/
PStr MultiSubj(TOCHandle tocH, PStr subject)
{
	// short the selected messages by subject
	uShort **vector = BoxSortVector(tocH, true, BVCompare_subj);
	Str255 localSubj;

	if (vector) {
		UHandle text;
		uShort count = HandleCount(vector);
		// figure out which one is used most
		uShort winnerCount;
		uShort winnerI =
		    MultiMost(vector, tocH, BVCompare_subj, &winnerCount);
		uShort sumNum = (*vector)[winnerI];

		// Grab the subject from the winner
		CacheMessage(tocH, sumNum);
		if (text = (*tocH)->sums[sumNum].cache) {
			HNoPurge(text);
			HandleHeadGetPStr(text, HeaderStrn + SUBJ_HEAD,
					  localSubj);
			HPurge(text);
		}
		// el slammo el dunko?
		if (winnerCount == count)
			PCopy(subject, localSubj);
		else
			// el no.  add some text to indicate we've changed the subject
			ComposeRString(subject, AMBIG_SUBJ_FMT, localSubj);
	} else
		*subject = 0;	// abject failure

	return subject;
}

/************************************************************************
 * MultiBody - Copy the bodies from originals to response
 ************************************************************************/
OSErr MultiBody(TOCHandle tocH, MessHandle messH)
{
	Str63 profile;

	if (!ConConProFind
	    (GetRString(profile, CONCON_MULTI_REPLY_PROFILE))) {
		ComposeStdAlert(Stop, CONCON_PROFILE_MISSING,
				CONCON_MULTI_REPLY_PROFILE);
		return fnfErr;
	}
	PeteSelect(nil, TheBody, REAL_BIG, REAL_BIG);
	return ConConMultipleR(tocH, TheBody, CONCON_MULTI_REPLY_PROFILE,
			       conConTypeAttribution, ReplyCCOFilter, nil);
}

/************************************************************************
 * ReplyCCOFilter - filter concentrator output for benefit of reply
 ************************************************************************/
void ReplyCCOFilter(ConConOutDescPtr thisODP, ConConMessElmPtr thisMEP,
		    MessHandle messH, ConConElmH cceh, StackHandle stack,
		    PETEHandle pte, void *userData)
{
	switch (thisMEP->type) {
	case conConTypeHeader:
	case conConTypeSeparator:
		thisODP->type = conConOutTypeRemove;
		break;
	default:
		thisODP->quoteIncrement++;
		break;
	}
}

/************************************************************************
 * MultiReferences - Copy the references.  Pain and suffering.
 ************************************************************************/
OSErr MultiReferences(TOCHandle tocH, MessHandle messH)
{
	// short the selected messages by message-id (hash)
	uShort **vector = BoxSortVector(tocH, true, BVCompare_msgIdHash);
	OSErr err = noErr;

	if (!vector)
		err = MemError();
	else {
		////////////////////////
		// Phase 1 - eliminate all messages referred to by other messages
		uShort count = HandleCount(vector);
		Boolean **used =
		    ZeroHandle(NuHTempBetter(count * sizeof(Boolean)));

		if (!used)
			err = MemError();
		else {
			short i;
			UHandle text;
			UHandle references = nil;
			UPtr less, great, end;
			// we'll use the existing extra header accumulator for irt, since we
			// know we'll have that.
			Accumulator extras = (*messH)->extras;
			Accumulator refAcc;

			Zero(refAcc);
			AccuAddRes(&refAcc, HeaderStrn + REFERENCES_HEAD);
			AccuAddRes(&extras, HeaderStrn + IN_REPLY_TO_HEAD);

			for (i = count; i--;) {
				CacheMessage(tocH, (*vector)[i]);
				if (text =
				    (*tocH)->sums[(*vector)[i]].cache) {
					// grab the references field
					HNoPurge(text);
					HandleHeadGetIdText(text,
							    HeaderStrn +
							    REFERENCES_HEAD,
							    &references);
					HPurge(text);

					if (references) {
						// parse the references field, using <>'s
						end =
						    LDRef(references) +
						    GetHandleSize
						    (references);
						for (less = *references;
						     less < end;
						     less = great + 1) {
							while (less < end
							       && *less !=
							       '<')
								less++;
							for (great =
							     less + 1;
							     great < end
							     && *great !=
							     '>'; great++);

							// did we find one?
							if (great < end) {
								// hash it, look for the hash
								uLong hash
								    =
								    MIDHash
								    (less,
								     great
								     -
								     less +
								     1);
								short index
								    =
								    BoxFindVectorMID
								    (vector,
								     tocH,
								     hash);

								// mark the one referred to as being used
								if (index
								    >= 0)
									(*used)[index] = true;
							}
						}
						ZapHandle(references);
					}
				}
			}

			////////////////////////
			// Phase 2 - gather the references fields from those left standing,
			// and gather the message-ids for the irt field
			for (i = count; i--;) {
				Str255 mid;
				Boolean doRefs = !(*used)[i];

				CacheMessage(tocH, (*vector)[i]);
				if (text =
				    (*tocH)->sums[(*vector)[i]].cache) {
					// grab the message-id and references field
					HNoPurge(text);
					if (doRefs)
						HandleHeadGetIdText(text,
								    HeaderStrn
								    +
								    REFERENCES_HEAD,
								    &references);
					HandleHeadGetPStr(text,
							  HeaderStrn +
							  MSGID_HEAD, mid);
					HPurge(text);

					// copy references?
					if (doRefs && references) {
						AccuAddChar(&refAcc, ' ');
						AccuAddHandle(&refAcc,
							      references);
						ZapHandle(references);
					}
					// add our message-id to in-reply-to and references
					AccuAddChar(&extras, ' ');
					AccuAddStr(&extras, mid);
					AccuAddChar(&refAcc, ' ');
					AccuAddStr(&refAcc, mid);
				}
			}

			////////////////////////
			// Phase 3 - clean up and put back

			// Terminate irt
			AccuAddChar(&extras, '\015');

			AccuAddChar(&refAcc, '\015');
			AccuTrim(&refAcc);
			AccuAddHandle(&extras, refAcc.data);
			AccuZap(refAcc);

			AccuTrim(&extras);

			// copy it back
			(*messH)->extras = extras;
		}
	}
	return err;
}

/************************************************************************
 * MultiMost - what value do we have the most of?
 ************************************************************************/
uShort MultiMost(uShort ** vector, TOCHandle tocH, short (*compare)(),
		 uShort * winnerCountP)
{
	uShort i = HandleCount(vector) - 1;

	// No winner yet
	short winnerI = 0;
	short winnerCount = 0;

	// The nth element is the first to be examined,
	// and so is our first winner
	short currentI = i;
	short currentCount = 1;

	// sanity check
	if (i <= 0)
		ASSERT(0);
	else {
		// examine the rest
		while (i--) {
			// if it's the same as the current one,
			// just count it
			if (!compare(vector, tocH, currentI, i))
				currentCount++;
			else {
				// it's different.  See if the one
				// we were looking at is the new winner
				if (currentCount > winnerCount) {
					// the king is dead.  long live the king.
					winnerI = currentI;
					winnerCount = currentCount;
				}
				currentI = i;
				currentCount = 1;
			}
		}

		// check one last time for regime change
		if (currentCount > winnerCount) {
			winnerI = currentI;
			winnerCount = currentCount;
		}
	}

	// if the caller wants to know how many, tell it
	if (winnerCountP)
		*winnerCountP = winnerCount;

	// now return the index into the vector
	return winnerI;
}

/************************************************************************
 * BoxSortVector - build a sort vector from a mailbox
 ************************************************************************/
uShort **BoxSortVector(TOCHandle tocH, Boolean selected,
		       short (*compare)())
{
	short count;
	uShort **vector;
	short i, n;

	/*
	 * allocate our sort vector
	 */
	if (!selected)
		count = (*tocH)->count;
	else
		count = CountSelectedMessages(tocH);

	if (!count)
		return nil;

	vector = (uShort **) NewHandle(sizeof(uShort) * count);

	if (!vector)
		return nil;

	/*
	 * copy the summary numbers into the vector
	 */
	if (selected) {
		for (i = n = 0; n < count; i++)
			if ((*tocH)->sums[i].selected)
				(*vector)[n++] = i;
	} else {
		for (i = 0; i < count; i++)
			(*vector)[i] = i;
	}

	VQuickSort(vector, 0, count - 1, (void *) tocH, compare);

	return vector;
}

/************************************************************************
 * BoxBinSearchVector - binary seach in a vector of toc entries
 ************************************************************************/
short BoxFindVectorMID(uShort ** vector, TOCHandle tocH, uLong value)
{
	short first = 0;
	short last = HandleCount(vector) - 1;
	short mid = (first + last) / 2;

	do {
		if ((*tocH)->sums[(*vector)[mid]].msgIdHash > value)
			first = mid + 1;
		else if ((*tocH)->sums[(*vector)[mid]].msgIdHash < value)
			last = mid - 1;
		else
			return mid;

		mid = (first + last) / 2;
	}
	while (first <= last);

	return -1;		// fail
}

/************************************************************************
 * BV comparison functions
 ************************************************************************/

#define ImpBVCompare(x) 																																		\
	short BVCompare_##x(uShort **vector,TOCHandle tocH,short i,short j)												\
	{																																													\
		if ((*tocH)->sums[(*vector)[i]].x > (*tocH)->sums[(*vector)[j]].x) return 1;						\
		else if ((*tocH)->sums[(*vector)[i]].x < (*tocH)->sums[(*vector)[j]].x) return -1;			\
		return 0;																																								\
	}

ImpBVCompare(persId)
    ImpBVCompare(msgIdHash)
short BVCompare_subj(uShort ** vector, TOCHandle tocH, short i, short j)
{
	return SubjCompare((*tocH)->sums[(*vector)[i]].subj,
			   (*tocH)->sums[(*vector)[j]].subj);
}
