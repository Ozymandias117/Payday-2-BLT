#ifndef __SIGNATURE_HEADER__
#define __SIGNATURE_HEADER__

#include <string>
#include <vector>

#define CREATE_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	name ## ptr name ## _orig = NULL; \
	SignatureSearch name ## search(&name ## _orig, signature, mask, offset);

#define CREATE_CALLABLE_CLASS_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(void*, __VA_ARGS__); \
	name ## ptr name ## _orig = NULL; \
	SignatureSearch name ## search(&name ## _orig, signature, mask, offset);

#define CREATE_LUA_FUNCTION(lua_func, name) \
	lua_pushcclosure_orig(L, lua_func, 0); \
	lua_setfield_orig(L, LUA_GLOBALSINDEX, name);

struct SignatureF {
	const char* signature;
	const char* mask;
	int offset;
	void* address;
};

class SignatureSearch {
public:
	SignatureSearch(void* address, const char* signature, const char* mask, int offset);
	static void Search();
};

#endif // __SIGNATURE_HEADER__
