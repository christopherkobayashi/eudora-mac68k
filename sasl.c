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

#include "sasl.h"
#define FILE_NUM 143
/* Copyright (c) 2002 by QUALCOMM Incorporated */

typedef struct {
	gss_ctx_id_t ctx;
	gss_name_t crname;
	short internalState;
} SASLGSSAPIContext, *SASLGSSAPIContextPtr, **SASLGSSAPIContextHandle;

#ifdef DEBUG			// these should be opaque, but I know them for debugging purposes

struct _krb5_auth_context {
	krb5_magic magic;
	krb5_address *remote_addr;
	krb5_address *remote_port;
	krb5_address *local_addr;
	krb5_address *local_port;
	krb5_keyblock *keyblock;
	krb5_keyblock *local_subkey;
	krb5_keyblock *remote_subkey;

	krb5_int32 auth_context_flags;
	krb5_int32 remote_seq_number;
	krb5_int32 local_seq_number;
	krb5_authenticator *authentp;	/* mk_req, rd_req, mk_rep, ... */
	krb5_cksumtype req_cksumtype;	/* mk_safe, ... */
	krb5_cksumtype safe_cksumtype;	/* mk_safe, ... */
	krb5_pointer i_vector;	/* mk_priv, rd_priv only */
	krb5_rcache rcache;
	krb5_enctype *permitted_etypes;	/* rd_req */
};

typedef struct _krb5_gss_ctx_id_rec {
	int initiate;		/* nonzero if initiating, zero if accepting */
	OM_uint32 gss_flags;
	int seed_init;
	unsigned char seed[16];
	krb5_principal here;
	krb5_principal there;
	krb5_keyblock *subkey;
	int signalg;
	int cksum_size;
	int sealalg;
	krb5_keyblock *enc;
	krb5_keyblock *seq;
	krb5_timestamp endtime;
	krb5_flags krb_flags;
	/* XXX these used to be signed.  the old spec is inspecific, and
	   the new spec specifies unsigned.  I don't believe that the change
	   affects the wire encoding. */
	krb5_ui_4 seq_send;
	krb5_ui_4 seq_recv;
	void *seqstate;
	int established;
	int big_endian;
	krb5_auth_context auth_context;
	gss_OID_desc *mech_used;
	int nctypes;
	krb5_cksumtype *ctypes;
} krb5_gss_ctx_id_rec, *krb5_gss_ctx_id_t;

#endif

OSErr SASLGSSAPI(PStr service, short rounds, long *state, AccuPtr chalAcc,
		 AccuPtr respAcc);
OSErr SASLCramMD5(short rounds, AccuPtr chalAcc, AccuPtr respAcc);
OSErr SASLPlain(short rounds, AccuPtr chalAcc, AccuPtr respAcc);
OSErr SASLLogin(short rounds, AccuPtr chalAcc, AccuPtr respAcc);
void SASLGSSAPIReport(OM_uint32 err);

/************************************************************************
 * SASLFind - is this a valid SASL mechanism?  0 for no, otherwise an index
 ************************************************************************/
short SASLFind(PStr service, PStr mechStr, SASLEnum mech)
{
	short foundMech = FindSTRNIndex(SASLStrn, mechStr);
	short outMech;

	ComposeLogS(LOG_PROTO, nil, "\pSASL mech %p %p", mechStr,
		    foundMech ? "\punderstood" :
		    "\pfrom planet ten by way of the eighth dimension");

	// Only do GSSAPI if we can
	if (foundMech == saslGSSAPI && !HaveKerbV()) {
		ComposeLogS(LOG_PROTO, nil,
			    "\pSASL mech %p ignored because GSSAPI not installed",
			    mechStr);
		foundMech = 0;
	}

	// Only do K4 if we can
	if (foundMech == saslKerbIV && !HaveKerbIV()) {
		ComposeLogS(LOG_PROTO, nil,
			    "\pSASL mech %p ignored because KerberosIV not installed",
			    mechStr);
		foundMech = 0;
	}

	if (foundMech) {
		Str255 badMechs;
		Str31 token;
		UPtr spot = badMechs + 1;
		Str63 servMech;

		// Build a service:mech string, and look for that, too
		PSCopy(servMech, service);
		PCatC(servMech, ':');
		PSCat(servMech, mechStr);

		// Look through the list of mechanisms we don't like
		GetRString(badMechs, SASL_DONT);
		while (PToken(badMechs, token, &spot, " "))
			if (StringSame(token, mechStr)
			    || StringSame(token, servMech)) {
				// oops, we don't like this one
				ComposeLogS(LOG_PROTO, nil,
					    "\pSASL mech %p VERBOTEN!!! (%p)",
					    mechStr, token);
				foundMech = 0;
				break;
			}
	}

	if (foundMech == saslKerbIV
	    && !EqualStrRes(service, KERBEROS_IMAP_SERVICE))
		foundMech = 0;

	if (!foundMech)
		outMech = mech;
	else if (!mech)
		outMech = foundMech;
	// the following little dance tells us not to do kerberos unless:
	//  a) Kerberos is all that is offered
	//      or
	//      b) The user has specifically requested it
	else if (KerberosMech(mech) && !KerberosMech(foundMech)
		 && !PrefIsSet(PREF_KERBEROS)) {
		ComposeLogS(LOG_PROTO, nil,
			    "\pSASL mech %r discarded because PREF_KERBEROS not set and non-Kerberos alternative offered",
			    SASLStrn + mech);
		outMech = foundMech;
	} else if (!KerberosMech(mech) && KerberosMech(foundMech)
		   && !PrefIsSet(PREF_KERBEROS)) {
		// leave current mech alone
		ComposeLogS(LOG_PROTO, nil,
			    "\pSASL mech %p ignored because PREF_KERBEROS not set and non-Kerberos alternative offered",
			    mechStr);
		outMech = mech;
	}
	// in general, smaller indices are considered more secure
	else
		outMech = MIN(mech, foundMech);

	ComposeLogS(LOG_PROTO, nil, "\pSASL mech was %r now %r",
		    SASLStrn + mech, SASLStrn + outMech);

	return outMech;
}

/************************************************************************
 * SASLDo - perform a round of SASL authentication
 ************************************************************************/
OSErr SASLDo(PStr service, SASLEnum mech, short rounds, long *state,
	     AccuPtr chalAcc, AccuPtr respAcc)
{
	short err = 501;

	switch (mech) {
	case saslGSSAPI:
		err = SASLGSSAPI(service, rounds, state, chalAcc, respAcc);
		break;
	case saslCramMD5:
		err = SASLCramMD5(rounds, chalAcc, respAcc);
		break;
	case saslPlain:
		err = SASLPlain(rounds, chalAcc, respAcc);
		break;
	case saslLogin:
		err = SASLLogin(rounds, chalAcc, respAcc);
		break;
	case saslKerbIV:
		err = 501;
		break;
	}

	if (err == 501) {
		// the auth mechanism thinks life is bad.  Cancel the auth
		AccuAddRes(respAcc, SASL_CANCEL);
		err = 0;	// let the server give the 501 back to us
	}

	return err;
}

/************************************************************************
 * SASLDone - the authentication has succeeded or failed, and the mechanism
 *   should have a chance to react appropriately, possibly by invalidating
 *   or validating a password or ticket or whatever
 ************************************************************************/
void SASLDone(PStr service, SASLEnum mech, short rounds, long *state,
	      short err)
{
	switch (mech) {
	case saslGSSAPI:
		{
			SASLGSSAPIContextHandle contextH =
			    (SASLGSSAPIContextHandle) * state;
			OM_uint32 res;

			if (contextH) {
				LDRef(contextH);
				if ((*contextH)->ctx)
					gss_delete_sec_context(&res,
							       &
							       (*contextH)->
							       ctx, nil);
				if ((*contextH)->crname)
					gss_release_name(&res,
							 &(*contextH)->
							 crname);
				ZapHandle(contextH);
				*state = 0;
			}
		}
		break;
	case saslCramMD5:
	case saslPlain:
	case saslLogin:
		if (err == 535)
			InvalidatePasswords(false, true, false);
		break;
	}
	if (err / 100 == 2 && !(*CurPers)->popSecure) {
		(*CurPers)->popSecure = true;
		(*CurPers)->dirty = true;
	}
}

/************************************************************************
 * SASLCramMD5 - perform a round of SASL authentication
 ************************************************************************/
OSErr SASLCramMD5(short rounds, AccuPtr chalAcc, AccuPtr respAcc)
{
	Str63 pass;
	Str63 user;
	Str255 challenge;

	if (rounds > 1) {
		ASSERT(0);
		return 501;
	}

	// build the initial command
	if (rounds == 0) {
		AccuAddRes(respAcc, SASLStrn + saslCramMD5);
		return noErr;
	}

	// respond to the challenge
	if (rounds == 1) {
		AccuToStr(chalAcc, challenge);
		PSCopy(pass, (*CurPers)->password);
		GetPOPInfo(user, nil);
		if (*challenge && !DecodeB64String(challenge)) {
			GenKeyedDigest(challenge, pass, GlobalTemp);
			PCopy(challenge, user);
			PCatC(challenge, ' ');
			PCat(challenge, GlobalTemp);
			AccuAddStrB64(respAcc, challenge);
			return noErr;
		} else {
			// empty/failed challenge.  Life is bad
			ASSERT(0);
			return 501;
		}
	}

	return fnfErr;
}

/************************************************************************
 * SASLPlain - perform a round of SASL authentication
 ************************************************************************/
OSErr SASLPlain(short rounds, AccuPtr chalAcc, AccuPtr respAcc)
{
	Str63 pass;
	Str63 user;
	Str255 raw;

	if (rounds == 0)
		DealingWithIdiotIMail = false;

	if (rounds == 1) {
		// Words cannot begin to express my disdain for the need
		// for the following code:
		AccuToStr(chalAcc, raw);
		DecodeB64String(raw);
		if (EqualStrRes(raw, USERNAME_PROMPT)) {
			DealingWithIdiotIMail = true;
			ProgressMessageR(kpMessage,
					 IMAIL_DOES_PLAIN_WRONG);
			Pause(60);
		}
	}

	if (DealingWithIdiotIMail)
		return SASLLogin(rounds, chalAcc, respAcc);

	// we will use the shortcut method,
	// since some servers insist on it.
	// however, if they ignore our initial argument
	// we can just resend it when we get a challenge, minus
	// the actual mechanism name.
	PSCopy(pass, (*CurPers)->password);
	GetPOPInfo(user, nil);
	ComposeRString(raw, AUTHPLAIN_FMT, PREF_SASL_AUTHORIZE, user,
		       pass);

	// Here is where we say "PLAIN" if we're doing the
	// initial command
	if (rounds == 0) {
		AccuAddRes(respAcc, SASLStrn + saslPlain);
		AccuAddChar(respAcc, ' ');
	}

	// Now we tack on the actual data...
	AccuAddStrB64(respAcc, raw);

	return 0;
}

/************************************************************************
 * SASLLogin - perform a round of SASL authentication
 ************************************************************************/
OSErr SASLLogin(short rounds, AccuPtr chalAcc, AccuPtr respAcc)
{
	Str63 pass;
	Str63 user;

	if (rounds > 2) {
		ASSERT(0);
		return 501;
	}

	// round 0: [auth ]login
	if (rounds == 0) {
		AccuAddRes(respAcc, SASLStrn + saslLogin);
		return noErr;
	}

#ifdef DEBUG
	// purely for interest' sake, what is the prompt?
	DecodeB64Accu(chalAcc, true);
#endif

	// round 1: username
	if (rounds == 1) {
		GetPOPInfo(user, nil);
		AccuAddStrB64(respAcc, user);
		return 0;
	}

	// round 2: password
	if (rounds == 2) {
		PSCopy(pass, (*CurPers)->password);
		AccuAddStrB64(respAcc, pass);
		return 0;
	}

	//      Unreachable
	ASSERT(0);
	return 501;
}

#define AUTH_GSSAPI_P_NONE 1

static gss_OID_desc oids[] = {
	{ 10, "\052\206\110\206\367\022\001\002\001\004" },
};

static gss_OID_desc *gss_nt_service_name = oids + 0;

OSErr SASLGSSAPIBuildServiceName(PStr fullService, PStr service);

#ifdef DEBUG
// uLong HashCTX(struct _krb5_gss_ctx_id_rec *ctx);
uLong HashPrincipal(krb5_principal p, uLong hash);
uLong HashAuthContext(krb5_auth_context a, uLong hash);
uLong HashKeyblock(krb5_keyblock * k, uLong hash);
uLong HashK5Address(krb5_address * a, uLong hash);
// void K5DumpContext(PStr s,struct _krb5_gss_ctx_id_rec *ctx,uLong hashBefore,uLong hashAfter);

#define LOG_K5_KEYBLOCK(k)	do {\
	if (k) {ComposeLogS(LOG_PROTO,nil,"\pCTX:" #k ": %x %x %x %d",k,k->magic,k->enctype,k->length);	\
					if (k->length<500) HexLog(LOG_PROTO,GSSAPI_LOG_FMT,k->contents,k->length);} \
	else ComposeLogS(LOG_PROTO,nil,"\pCTX:" #k ": nil");	\
	} while (0)

#ifdef I_EVER_FIX_THIS
void K5DumpContext(PStr s, struct _krb5_gss_ctx_id_rec *ctx,
		   uLong hashBefore, uLong hashAfter)
{
	ComposeLogS(LOG_PROTO, nil,
		    "\pCTX %p: ctx %x hashBefore %x hashAfter %x", s, ctx,
		    hashBefore, hashAfter);
	if (0)			//if (ctx)
	{
		ComposeLogS(LOG_PROTO, nil,
			    "\pCTX: init %d flags %x seed_init %d seed %x%x%x%x",
			    ctx->initiate, ctx->gss_flags, ctx->seed_init,
			    (long *) ctx->seed, ((long *) ctx->seed)[1],
			    ((long *) ctx->seed)[2],
			    ((long *) ctx->seed)[3]);
		ComposeLogS(LOG_PROTO, nil,
			    "\pCTX: here %x %o there %x %o",
			    ctx->here->magic,
			    *(long *) ctx->here->realm.data,
			    ctx->there->magic,
			    *(long *) ctx->there->realm.data);
		ComposeLogS(LOG_PROTO, nil,
			    "\pCTX: seq_send %d seq_recv %d established %d big_endian %d",
			    ctx->seq_send, ctx->seq_recv, ctx->established,
			    ctx->big_endian);
		LOG_K5_KEYBLOCK(ctx->subkey);
		LOG_K5_KEYBLOCK(ctx->enc);
		LOG_K5_KEYBLOCK(ctx->seq);
		LOG_K5_KEYBLOCK(ctx->auth_context->keyblock);
		LOG_K5_KEYBLOCK(ctx->auth_context->local_subkey);
		LOG_K5_KEYBLOCK(ctx->auth_context->remote_subkey);
	}
}

uLong HashCTX(struct _krb5_gss_ctx_id_rec *ctx)
{
	uLong hash = 0;
	if (ctx) {
		hash =
		    HashWithSeedLo(&ctx->gss_flags, sizeof(ctx->gss_flags),
				   hash);
		hash = HashWithSeedLo(ctx->seed, sizeof(ctx->seed), hash);
		hash = HashPrincipal(ctx->here, hash);
		hash = HashPrincipal(ctx->there, hash);
		hash = HashKeyblock(ctx->subkey, hash);
		hash =
		    HashWithSeedLo(&ctx->signalg, sizeof(ctx->signalg),
				   hash);
		hash =
		    HashWithSeedLo(&ctx->cksum_size,
				   sizeof(ctx->cksum_size), hash);
		hash =
		    HashWithSeedLo(&ctx->sealalg, sizeof(ctx->sealalg),
				   hash);
		hash = HashKeyblock(ctx->enc, hash);
		hash = HashKeyblock(ctx->seq, hash);
		hash =
		    HashWithSeedLo(&ctx->big_endian,
				   sizeof(ctx->big_endian), hash);
		hash = HashAuthContext(ctx->auth_context, hash);
	}
	return hash;
}
#endif

uLong HashPrincipal(krb5_principal p, uLong hash)
{
	short i;
	if (p) {
		hash = HashWithSeedLo(p, sizeof(*p), hash);
		hash =
		    HashWithSeedLo(p->realm.data, p->realm.length, hash);
		for (i = 0; i < p->length; i++) {
			hash =
			    HashWithSeedLo(p->data + i, sizeof(krb5_data),
					   hash);
			hash =
			    HashWithSeedLo(p->data[i].data,
					   p->data[i].length, hash);
		}
	}
	return hash;
}

uLong HashAuthContext(krb5_auth_context a, uLong hash)
{
	if (a) {
		hash = HashWithSeedLo(a, sizeof(*a), hash);
		hash = HashK5Address(a->remote_addr, hash);
		hash = HashK5Address(a->remote_port, hash);
		hash = HashK5Address(a->local_addr, hash);
		hash = HashK5Address(a->local_port, hash);
		hash = HashKeyblock(a->keyblock, hash);
		hash = HashKeyblock(a->local_subkey, hash);
		hash = HashKeyblock(a->remote_subkey, hash);
		if (a->authentp)
			hash =
			    HashWithSeedLo(a->authentp,
					   sizeof(*a->authentp), hash);
	}
	return hash;
}

uLong HashK5Address(krb5_address * a, uLong hash)
{
	if (a) {
		hash = HashWithSeedLo(a, sizeof(*a), hash);
		if (a->contents)
			hash =
			    HashWithSeedLo(a->contents, a->length, hash);
	}
	return hash;
}

uLong HashKeyblock(krb5_keyblock * k, uLong hash)
{
	if (k) {
		hash = HashWithSeedLo(k, sizeof(*k), hash);
		hash = HashWithSeedLo(k->contents, k->length, hash);
	}
	return hash;
}

#endif

/************************************************************************
 * SASLGSSAPI - perform a round of SASL authentication
 ************************************************************************/
OSErr SASLGSSAPI(PStr service, short rounds, long *state, AccuPtr chalAcc,
		 AccuPtr respAcc)
{
	OSErr err = noErr;
	OM_uint32 min, maj;
	gss_buffer_desc buf, chal, resp;
	SASLGSSAPIContextHandle contextH =
	    (SASLGSSAPIContextHandle) * state;
	gss_name_t crname = nil;	// gssapi's idea of the service name
	gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;	// gssapi's idea of a context
	Str255 scratch;
#ifdef DEBUG
	uLong hashBefore = 0, hashAfter = 0;
#endif

	Zero(chal);
	Zero(resp);
	Zero(buf);

	if (rounds == 0) {
		// initialize our context
		contextH = NewZH(SASLGSSAPIContext);
		*state = (long) contextH;
		if (!contextH)
			return 501;

		// just tack on "gssapi" for now
		// note that some smtp servers may barf if we don't put the initial
		// response here, too, so be warned.  However, clueless dweebs may
		// not be doing gssapi, so this may be safe
		AccuAddRes(respAcc, SASLStrn + saslGSSAPI);

		return noErr;
	}

	if (rounds == 1) {
		// do initialization

		// get our idea of the service name
		if (err = SASLGSSAPIBuildServiceName(scratch, service))
			return 501;

		// gssapi has its own idea of the service name
		PTerminate(scratch);
		buf.length = *scratch + 1;
		buf.value = scratch + 1;
		if (err =
		    (gss_import_name
		     (&min, &buf, gss_nt_service_name,
		      &crname) != GSS_S_COMPLETE))
			return 501;

		// Work around bug in kfm 4.0
		Kfm40Hack();

		// start it up
		Zero(resp);
#ifdef DEBUG
		// crashes with new K5... hashBefore = HashCTX(ctx);
#endif
		maj = gss_init_sec_context(&min,	// [output] "minor" status
					   GSS_C_NO_CREDENTIAL,	// Use default credential
					   &ctx,	// [output] our magic context
					   crname,	// its idea of our service name above
					   GSS_C_NO_OID,	// Use default mechanism
					   GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,	// self-authenticate & avoid replay
					   0,	// Default context validity period
					   GSS_C_NO_CHANNEL_BINDINGS,	// ???
					   GSS_C_NO_BUFFER,	// no input for this round
					   NIL,	// [output] we don't care about actual mech
					   &resp,	// [output] our initial response
					   NIL,	// [output] we don't give a darn about flags
					   NIL);	// [output] we don't give a darn about ticket lifetime
#ifdef DEBUG
		// crashes with new K5... hashAfter = HashCTX(ctx);
		ComposeLogS(LOG_PROTO, nil,
			    "\pgss_init_sec_context: maj %x resp.length %d",
			    maj, resp.length);
		// K5DumpContext("\pafter gss_init_sec_context",(void*)ctx,hashBefore,hashAfter);
#endif

		// Did we fail?
		if (maj != GSS_S_CONTINUE_NEEDED) {
			gss_release_buffer(&min, &resp);
			return 501;
		}

		// Save our context
		(*contextH)->ctx = ctx;
		(*contextH)->crname = crname;

		// format the response
		//HexLog(LOG_PROTO,GSSAPI_LOG_FMT,resp.value,resp.length);  
		err = AccuAddPtrB64(respAcc, resp.value, resp.length);
		gss_release_buffer(&min, &resp);

		return err ? 501 : noErr;
	}

	if (rounds > 1 && (*contextH)->internalState == 0) {
		// We will have a challenge.  Decode it.                
		if (DecodeB64Accu(chalAcc, false))
			return 501;
		//HexLog(LOG_PROTO,GSSAPI_LOG_FMT,LDRef(chalAcc->data),chalAcc->offset);
		UL(chalAcc->data);

		// Need local copy
		ctx = (*contextH)->ctx;

		// Around we go again
		chal.value = LDRef(chalAcc->data);
		chal.length = chalAcc->offset;
		Zero(resp);
#ifdef DEBUG
		// hashBefore = HashCTX(ctx);
#endif
		maj = gss_init_sec_context(&min,	// [output] "minor" status
					   GSS_C_NO_CREDENTIAL,	// Use default credential
					   &ctx,	// [output] our magic context
					   (*contextH)->crname,	// its idea of our service name above
					   GSS_C_NO_OID,	// Use default mechanism
					   GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,	// self-authenticate & avoid replay
					   0,	// Default context validity period
					   GSS_C_NO_CHANNEL_BINDINGS,	// ???
					   &chal,	// challenge
					   NIL,	// [output] we don't care about actual mech
					   &resp,	// [output] our initial response
					   NIL,	// [output] we don't give a darn about flags
					   NIL);	// [output] we don't give a darn about ticket lifetime
#ifdef DEBUG
		// hashAfter = HashCTX(ctx);
		ComposeLogS(LOG_PROTO, nil,
			    "\pgss_init_sec_context: maj %x resp.length %d",
			    maj, resp.length);
		// K5DumpContext("\pafter gss_init_sec_context",(void*)ctx,hashBefore,hashAfter);
#endif
		(*contextH)->ctx = ctx;	// save our new context

		// first unlock the data...
		UL(chalAcc->data);

		// another round?
		if (maj == GSS_S_CONTINUE_NEEDED || maj == GSS_S_COMPLETE) {
			if (resp.length)
				AccuAddPtrB64(respAcc, resp.value,
					      resp.length);
			gss_release_buffer(&min, &resp);
			if (maj == GSS_S_COMPLETE)
				(*contextH)->internalState++;
			return noErr;
		}

		goto fail;
	}

	if ((*contextH)->internalState == 1) {
		int conf;
		gss_qop_t qop;
#ifdef I_EVER_FIX_THIS
		uLong hashBefore, hashAfter;
#endif

		// We will have a challenge.  Decode it.                
		if (DecodeB64Accu(chalAcc, false))
			return 501;

		// Voodoo that means nothing to me
		Zero(resp);
		chal.value = LDRef(chalAcc->data);
		chal.length = chalAcc->offset;
#ifdef I_EVER_FIX_THIS
		// hashBefore = HashCTX((*contextH)->ctx);
#endif
		maj =
		    gss_unwrap(&min, (*contextH)->ctx, &chal, &resp, &conf,
			       &qop);
#ifdef I_EVER_FIX_THIS
		// hashAfter = HashCTX((*contextH)->ctx);
		ComposeLogS(LOG_PROTO, nil,
			    "\pgss_unwrap: maj %x conf %d qop %d", maj,
			    conf, qop);
		// K5DumpContext("\pafter gss_unwrap",(*contextH)->ctx,hashBefore,hashAfter);
#endif
		UL(chalAcc->data);

		if (maj == GSS_S_COMPLETE && resp.length >= 4
		    && ((*(char *) resp.value) & AUTH_GSSAPI_P_NONE)) {
			// re-use the challenge buffer for some temp work
			chalAcc->offset = 0;

			AccuAddPtr(chalAcc, resp.value, 4);
			gss_release_buffer(&min, &resp);

			// no, we don't want session "protection" (SSL?)
			**chalAcc->data = AUTH_GSSAPI_P_NONE;

			// tack on username
			GetPOPInfo(scratch, nil);
			AccuAddStr(chalAcc, scratch);

			// more voodoo
			buf.value = LDRef(chalAcc->data);
			buf.length = chalAcc->offset;
			maj =
			    gss_wrap(&min, (*contextH)->ctx, false, qop,
				     &buf, &conf, &resp);
			UL(chalAcc->data);

			// did we win?
			if (maj == GSS_S_COMPLETE) {
				AccuAddPtrB64(respAcc, resp.value,
					      resp.length);
				gss_release_buffer(&min, &resp);
				(*contextH)->internalState++;
				return noErr;
			}
		} else if (maj == GSS_S_COMPLETE && resp.length < 4) {
			// for Kerberos, this is normal.  Return "no data" to server
			// which will happen sneakily if we return a 501 here
			return noErr;
		}

		goto fail;
	}

      fail:

	switch (maj) {
	case GSS_S_CREDENTIALS_EXPIRED:
		GetRString(scratch,
			   GSSAPIErrorsStrn + kGSSAPICredExpiredError);
		AlertStr(OK_ALRT, Note, scratch);
		break;

	case GSS_S_FAILURE:
		if (min == (OM_uint32) KRB5_FCC_NOFILE) {
			GetRString(scratch,
				   GSSAPIErrorsStrn + kGSSAPINoCredError);
			AlertStr(OK_ALRT, Note, scratch);
		} else
			SASLGSSAPIReport(min);
		break;

	default:		/* miscellaneous errors */
		if (maj)
			SASLGSSAPIReport(maj);
		if (min)
			SASLGSSAPIReport(min);
		break;
	}

	if (resp.value)
		gss_release_buffer(&min, &resp);

	return 501;
}

/************************************************************************
 * SASLGSSAPIReport - get errors from gssapi
 ************************************************************************/
void SASLGSSAPIReport(OM_uint32 err)
{
	OM_uint32 mmin, mmaj;
	gss_buffer_desc buf;
	gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
	Str255 scratch;

	do {
		mmaj =
		    gss_display_status(&mmin, err, GSS_C_MECH_CODE,
				       GSS_C_NULL_OID, &ctx, &buf);
		switch (mmaj) {
		case GSS_S_COMPLETE:
		case GSS_S_CONTINUE_NEEDED:
			ComposeRString(scratch,
				       GSSAPIErrorsStrn + kGSSAPIFailure,
				       buf.value);
			break;
		default:
			ComposeRString(scratch,
				       GSSAPIErrorsStrn +
				       kGSSAPIUnknownFailure, buf.value);
			break;
		}
		AlertStr(OK_ALRT, Note, scratch);
		gss_release_buffer(&mmin, &buf);
	}
	while (mmaj == GSS_S_CONTINUE_NEEDED);

	gss_delete_sec_context(&mmin, &ctx, nil);
}

/************************************************************************
 * SASLGSSAPIBuildServiceName - build the service name for GSSAPI
 ************************************************************************/
OSErr SASLGSSAPIBuildServiceName(PStr fullService, PStr service)
{
	Str127 user, host;
	Str63 realm;
	struct hostInfo *hip, hi;
	Str63 shortHost;
	Str31 fmt;
	UPtr spot;
	OSErr err;

	GetPref(realm, PREF_REALM);
	GetPOPInfo(user, host);
	if (EqualStrRes(service, K5_SMTP_SERVICE))
		GetSMTPInfo(host);

	if (!PrefIsSet(PREF_K5_STRICT_HOSTNAMES)) {
		/*
		 * the best thing in the world is four million DNS calls
		 */
		if (err = GetHostByName(host, &hip))
			return (err);
		if (err = GetHostByAddr(&hi, hip->addr[0]))
			return (err);
		CtoPCpy(host, hi.cname);
	}

	/*
	 * build the service name
	 */
	spot = host + 1;
	PToken(host, shortHost, &spot, ".");
	GetRString(fmt, K5_SERVICE_FMT);
	utl_PlugParams(fmt, fullService, service, host, realm, shortHost);
	return noErr;
}
