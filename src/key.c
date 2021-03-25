/*
 * Brian Chrzanowski
 * 2021-03-09 00:24:47
 *
 * Key / Value / Metadata Handling Functions
 *
 * Until I feel the need / desire to not just use the filesystem, we use the filesystem's names to
 * keep the keys. For each key, we store one file formatted as such:
 *
 *   Content-Type: application/json
 *   Content-Length: 17
 *
 *   {"testing": true}
 *
 * This allows us to just YEET the data across the network; however it requires getting a little
 * creative with how we handle POSTs because we have to reassemble the type and length, but that's
 * okay.
 */

#include "common.h"

#include "key.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

struct pcgrand_t {
	u64 state;
	u64 inc;
	s32 init;
};

struct pcgrand_t localrand = {0};

char KEYMAP[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
	'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
};

// RNG FUNCTIONS
// pcg_rand : get a random num from the pcgrand state
unsigned long pcg_rand(struct pcgrand_t *rng);
// pcg_seed : seed the random structure with some junk data
void pcg_seed(struct pcgrand_t *rng, u64 initstate, u64 initseq);

// KeyGet: returns the data for the given key
void *KeyGet(char *key, size_t *len)
{
	char buf[BUFLARGE];

	KeyMakePath(buf, sizeof buf, key);

	return sys_readfile(buf, len);
}

// KeyWrite: writes the key
int KeyWrite(char *key, void *data, size_t len)
{
	char path[BUFLARGE];
	FILE *fp;

	KeyMakePath(path, sizeof path, key);

	fp = fopen(path, "wb");
	fwrite(data, 1, len, fp);
	fclose(fp);

	return 0;
}

// KeyDelete: deletes the key
int KeyDelete(char *key)
{
	char path[BUFLARGE];

	if (KeyDoesExist(key)) {
		KeyMakePath(path, sizeof path, key);
		remove(path);
	}

	return 0;
}

// KeyDoesExist: returns true if the key exists
int KeyDoesExist(char *key)
{
	struct stat sb;
	char fname[BUFSMALL];

	snprintf(fname, sizeof fname, "%s/%s", DATADIR, key);

	return stat(fname, &sb) == 0 && S_ISREG(sb.st_mode);
}

// KeyMake: write len random bytes from KEYMAP into s
void KeyMake(char *s, size_t len)
{
	int i;

	if (s == NULL) {
		return;
	}

	assert(len <= KEYLEN);

	for (i = 0; i < len; i++) {
		s[i] = KEYMAP[pcg_rand(&localrand) % ARRSIZE(KEYMAP)];
	}
}

// KeyMakePath: prints the key path into s
void KeyMakePath(char *s, size_t slen, char *key)
{
	if (s == NULL)
		return;

	snprintf(s, slen, "%s/%s", DATADIR, key);
}

// KeyIsLegal: returns true if the key is legal, false if it isn't
int KeyIsLegal(char *s, size_t len)
{
	int i, j;

	if (s == NULL) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		for (j = 0; j < ARRSIZE(KEYMAP); j++) {
			if (s[i] == KEYMAP[j]) {
				break;
			}
		}

		if (j == ARRSIZE(KEYMAP)) {
			return 0;
		}
	}

	return 1;
}

// pcg_rand : get a random num from the pcgrand state
unsigned long pcg_rand(struct pcgrand_t *rng)
{
	unsigned long old;
	unsigned int xorshift, rot;

	old = rng->state;

	/* advance internal state */
	rng->state = old * 6364136223846793005ULL + (rng->inc | 1);

	/* calculate output function (XSH RR), uses old state for max ILP */
	xorshift = ((old >> 18u) ^ old) >> 27u;
	rot = old >> 59u;

	return (xorshift >> rot) | (xorshift << ((-rot) & 31));
}

// pcg_seed : seed the random structure with some junk data
void pcg_seed(struct pcgrand_t *rng, u64 initstate, u64 initseq)
{
	if (rng->init) return;

    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg_rand(rng);
    rng->state += initstate;
    pcg_rand(rng);
}

