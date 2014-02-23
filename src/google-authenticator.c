#include "pebble.h"
#include "base32.h"
#include "hmac.h"
#include "sha1.h"

#define SECRET_BITS               80          // Must be divisible by eight
#define VERIFICATION_CODE_MODULUS (1000*1000) // Six digits
#define SCRATCHCODES              5           // Number of initial scratchcodes
#define SCRATCHCODE_LENGTH        8           // Eight digits per scratchcode
#define BYTES_PER_SCRATCHCODE     4           // 32bit of randomness is enough
#define BITS_PER_BASE32_CHAR      5           // Base32 expands space by 8/5

static char *generateCode(const char *key, int timezone_offset) {
	//long tm = time(NULL)/30;
	long tm = (time(NULL) + (timezone_offset*60))/30;
	uint8_t challenge[8];
	for (int i = 8; i--; tm >>= 8) {
		challenge[i] = tm;
	}
	
	// Estimated number of bytes needed to represent the decoded secret. Because
	// of white-space and separators, this is an upper bound of the real number,
	// which we later get as a return-value from base32_decode()
	int secretLen = (strlen(key) + 7)/8*BITS_PER_BASE32_CHAR;
	
	// Sanity check, that our secret will fixed into a reasonably-sized static
	// array.
	if (secretLen <= 0 || secretLen > 100) {
		//return -1;
		return "";
	}
	
	// Decode secret from Base32 to a binary representation, and check that we
	// have at least one byte's worth of secret data.
	uint8_t secret[100];
	if ((secretLen = base32_decode((const uint8_t *)key, secret, secretLen))<1) {
		//return -1;
		return "";
	}
	
	// Compute the HMAC_SHA1 of the secrete and the challenge.
	uint8_t hash[SHA1_DIGEST_LENGTH];
	hmac_sha1(secret, secretLen, challenge, 8, hash, SHA1_DIGEST_LENGTH);
	
	// Pick the offset where to sample our hash value for the actual verification
	// code.
	int offset = hash[SHA1_DIGEST_LENGTH - 1] & 0xF;
	
	// Compute the truncated hash in a byte-order independent loop.
	unsigned int truncatedHash = 0;
	for (int i = 0; i < 4; ++i) {
		truncatedHash <<= 8;
		truncatedHash  |= hash[offset + i];
	}
	
	// Truncate to a smaller number of digits.
	truncatedHash &= 0x7FFFFFFF;
	truncatedHash %= VERIFICATION_CODE_MODULUS;

	// Convert the truncatedHash int to a Char/String
	static char tokenText[7] = "000000";

	for(int i = 5; i >= 0; i--)
	{
		tokenText[i] = '0' + (truncatedHash % 10);
		truncatedHash /= 10;
	}
	tokenText[6] = '\0';

	return tokenText;
}