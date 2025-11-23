#ifndef __KSU_H_MANAGER_SIGN
#define __KSU_H_MANAGER_SIGN

#include <linux/types.h>

// rsuntk/KernelSU
#define EXPECTED_SIZE_RSUNTK 0x396
#define EXPECTED_HASH_RSUNTK                                                   \
	"f415f4ed9435427e1fdf7f1fccd4dbc07b3d6b8751e4dbcec6f19671f427870b"

typedef struct {
	u32 size;
	const char *sha256;
} apk_sign_key_t;

#endif /* MANAGER_SIGN_H */
