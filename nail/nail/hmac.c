/*
 * Heirloom mailx - a mail user agent derived from Berkeley Mail.
 *
 * Copyright (c) 2000-2004 Gunnar Ritter, Freiburg i. Br., Germany.
 */
/*
 * Derived from:

Network Working Group					    H. Krawczyk
Request for Comments: 2104					    IBM
Category: Informational					     M. Bellare
								   UCSD
							     R. Canetti
								    IBM
							  February 1997


	     HMAC: Keyed-Hashing for Message Authentication

Status of This Memo

   This memo provides information for the Internet community.  This memo
   does not specify an Internet standard of any kind.  Distribution of
   this memo is unlimited.

Appendix -- Sample Code

   For the sake of illustration we provide the following sample code for
   the implementation of HMAC-MD5 as well as some corresponding test
   vectors (the code is based on MD5 code as described in [MD5]).
*/

/*	Sccsid @(#)hmac.c	1.8 (gritter) 3/4/06	*/

#include "rcv.h"
#include "md5.h"

/*
** Function: hmac_md5
*/

void 
hmac_md5 (
    unsigned char *text,		/* pointer to data stream */
    int text_len,	/* length of data stream */
    unsigned char *key,		/* pointer to authentication key */
    int key_len,	/* length of authentication key */
    void *digest	/* caller digest to be filled in */
)

{
	MD5_CTX context;
	unsigned char k_ipad[65];    /* inner padding -
				      * key XORd with ipad
				      */
	unsigned char k_opad[65];    /* outer padding -
				      * key XORd with opad
				      */
	unsigned char tk[16];
	int i;
	/* if key is longer than 64 bytes reset it to key=MD5(key) */
	if (key_len > 64) {

		MD5_CTX	     tctx;

		MD5Init(&tctx);
		MD5Update(&tctx, key, key_len);
		MD5Final(tk, &tctx);

		key = tk;
		key_len = 16;
	}

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */

	/* start out by storing key in pads */
	memset(k_ipad, 0, sizeof k_ipad);
	memset(k_opad, 0, sizeof k_opad);
	memcpy(k_ipad, key, key_len);
	memcpy(k_opad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i=0; i<64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}
	/*
	 * perform inner MD5
	 */
	MD5Init(&context);		     /* init context for 1st
					      * pass */
	MD5Update(&context, k_ipad, 64);	     /* start with inner pad */
	MD5Update(&context, text, text_len); /* then text of datagram */
	MD5Final(digest, &context);	     /* finish up 1st pass */
	/*
	 * perform outer MD5
	 */
	MD5Init(&context);		     /* init context for 2nd
					      * pass */
	MD5Update(&context, k_opad, 64);     /* start with outer pad */
	MD5Update(&context, digest, 16);     /* then results of 1st
					      * hash */
	MD5Final(digest, &context);	     /* finish up 2nd pass */
}
