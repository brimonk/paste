#if !defined(KEY_H)

#include "common.h"

#define KEYLEN  (8)
#define DATADIR ("./data")

struct pcgrand_t;

extern struct pcgrand_t localrand;

// KeyGet: returns the data for the given key
void *KeyGet(char *key, size_t *len);

// KeyDelete: deletes the key
int KeyDelete(char *key);

// KeyDoesExist: returns true if the key exists
int KeyDoesExist(char *key);

// KeyMake: makes the key; writes len random bytes from KEYMAP into s
void KeyMake(char *s, size_t len);

// KeyMakePath: prints the key path into s
void KeyMakePath(char *s, size_t slen, char *key);

// KeyIsLegal: returns true if the key is legal, false if it isn't
int KeyIsLegal(char *s, size_t len);

// pcg_rand : get a random num from the pcgrand state
unsigned long pcg_rand(struct pcgrand_t *rng);

// pcg_seed : seed the random structure with some junk data
void pcg_seed(struct pcgrand_t *rng, u64 initstate, u64 initseq);

#endif // KEY_H

