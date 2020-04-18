#ifndef UTF8_CODEPAGES_H
#define UTF8_CODEPAGES_H

enum ciolib_codepage {
	CIOLIB_CP437,
	CIOLIB_CP1251,
	CIOLIB_KOI8_R,
	CIOLIB_ISO_8859_2,
	CIOLIB_ISO_8859_4,
	CIOLIB_CP866M,
	CIOLIB_ISO_8859_9,
	CIOLIB_ISO_8859_8,
	CIOLIB_KOI8_U,
	CIOLIB_ISO_8859_15,
	CIOLIB_ISO_8859_5,
	CIOLIB_CP850,
	CIOLIB_CP865,
	CIOLIB_ISO_8859_7,
	CIOLIB_ISO_8859_1,
	CIOLIB_CP866M2,
	CIOLIB_CP866U,
	CIOLIB_CP1131,
	CIOLIB_ARMSCII8,
	CIOLIB_HAIK8,
	CIOLIB_ATASCII,
	CIOLIB_PETSCIIU,
	CIOLIB_PETSCIIL,
	CIOLIB_CP_COUNT
};

struct codepage_def {
	char name[32];
	enum ciolib_codepage cp;
	uint8_t *(*to_utf8)(const char *cp437str, size_t buflen, size_t *outlen, struct codepage_def *cpdef);
	char *(*utf8_to)(const uint8_t *utf8str, char unmapped, size_t buflen, size_t *outlen, struct codepage_def *cpdef);
	uint8_t (*from_unicode_cpoint)(uint32_t cpoint, char unmapped, struct codepage_def *cpdef);
	uint32_t (*from_cpchar)(uint8_t cpoint, struct codepage_def *cpdef);
	uint32_t (*from_cpchar_ext)(uint8_t cpoint, struct codepage_def *cpdef);
	struct ciolib_cpmap *cp_table;
	size_t cp_table_sz;
	uint32_t *cp_unicode_table;
	uint32_t *cp_ext_unicode_table;
};

struct codepage_def ciolib_cp[CIOLIB_CP_COUNT];

uint8_t *cp_to_utf8(enum ciolib_codepage cp, const char *cpstr, size_t buflen, size_t *outlen);
char *utf8_to_cp(enum ciolib_codepage cp, const uint8_t *utf8str, char unmapped, size_t buflen, size_t *outlen);
uint8_t cpchar_from_unicode_cpoint(enum ciolib_codepage cp, uint32_t cpoint, char unmapped);
uint32_t cpoint_from_cpchar(enum ciolib_codepage cp, uint8_t ch);
uint32_t cpoint_from_cpchar_ext(enum ciolib_codepage cp, uint8_t ch);

#endif
