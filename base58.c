/*
 * Copyright 2012 Luke Dashjr
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the standard MIT license.  See COPYING for more details.
 */

#ifndef WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <libbase58.h>
#include <segwit_addr.h>

#include <blkmaker.h>

#include "private.h"

bool _blkmk_b58tobin(void *bin, size_t binsz, const char *b58, size_t b58sz) {
	return b58tobin(bin, &binsz, b58, b58sz);
}

int _blkmk_b58check(void *bin, size_t binsz, const char *base58str) {
	if (!b58_sha256_impl)
		b58_sha256_impl = blkmk_sha256_impl;
	return b58check(bin, binsz, base58str, 34);
}

size_t blkmk_address_to_script(void *out, size_t outsz, const char *addr) {
	unsigned char addrbin[25];
	unsigned char *cout = out;
	const size_t b58sz = strlen(addr);
	int addrver;
	size_t rv;

	uint8_t data[82];
	char hrp[84];
	char witdest[65];
	char rebuild[92];
	uint8_t witprog[40];
	size_t witprog_len;
	int witver;
	uint8_t scriptpubkey[42];
	size_t scriptpubkey_len;

	printf(">>> blkmk_address_to_script=%s\n", addr);

	if (bech32_decode(hrp, data, &rv, addr)) {

#ifndef TEMP
		_blkmk_bin2hex(witdest, &data[1], 32);
		printf(">>> bech32 succeeded! hrp=%s len=%d ver=%d witdest=0x%s\n", hrp, rv, data[0], witdest);

		if (!bech32_encode(rebuild, hrp, data, rv)) {
			printf(">>> bech32 rebuild failed\n");
			return 0;
		}

		if (!strcmpi(rebuild, data)) {
			printf(">>> bech32 rebuild different %s\n", rebuild);
			return 0;
		}
		else {
			printf(">>> bech32 rebuild same %s\n", rebuild);
		}
#endif

		if (segwit_addr_decode(&witver, witprog, &witprog_len, hrp, addr)) {

			_blkmk_bin2hex(witdest, witprog, witprog_len);
			printf(">>> segwit_addr_decode success! ver=%d len=%d witprog=%s\n", witver, witprog_len, witdest);

			if (witver == 0) {
				rv = witprog_len + 2;

				if (outsz == rv) {
					cout[0] = 0x00;         // OP_0
					cout[1] = witprog_len;  // push size of script
					memcpy(&cout[2], witprog, witprog_len);
				}
			}
		}

		return rv;
	}
	
	rv = sizeof(addrbin);
	if (!b58_sha256_impl)
		b58_sha256_impl = blkmk_sha256_impl;
	if (!b58tobin(addrbin, &rv, addr, b58sz))
		return 0;
 
	addrver = b58check(addrbin, sizeof(addrbin), addr, b58sz);
	
	printf(">>> addrver=%d\n", addrver);

	switch (addrver) {
		case   0:  // Bitcoin pubkey hash
		case 111:  // Testnet pubkey hash
			if (outsz < (rv = 25))
				return rv;
			cout[ 0] = 0x76;  // OP_DUP
			cout[ 1] = 0xa9;  // OP_HASH160
			cout[ 2] = 0x14;  // push 20 bytes
			memcpy(&cout[3], &addrbin[1], 20);
			cout[23] = 0x88;  // OP_EQUALVERIFY
			cout[24] = 0xac;  // OP_CHECKSIG
			return rv;
		case   5:  // Bitcoin script hash
		case 196:  // Testnet script hash
			if (outsz < (rv = 23))
				return rv;
			cout[ 0] = 0xa9;  // OP_HASH160
			cout[ 1] = 0x14;  // push 20 bytes
			memcpy(&cout[2], &addrbin[1], 20);
			cout[22] = 0x87;  // OP_EQUAL
			return rv;
		default:
			return 0;
	}
}
