JSLIB_BUILD	=	..$(DIRSEP)build$(DIRSEP)$(JS_LIB)
CRYPTLIB_BUILD	=	..$(DIRSEP)build$(DIRSEP)$(CRYPT_LIB)

all: lib

jslib: $(JSLIB_BUILD)

cryptlib: $(CRYPTLIB_BUILD)

lib: $(JSLIB_BUILD) $(CRYPTLIB_BUILD)
