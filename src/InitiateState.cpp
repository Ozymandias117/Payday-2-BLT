#include "InitState.h"
#include "./subhook/subhook.h"

#include "signatures/signatures.h"
#include "util/util.h"
#include "console/console.h"
#include "threading/queue.h"
#include "http/http.h"

#include <stddef.h>
#include <thread>
#include <list>

class lua_State;

typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);
typedef int(*lua_CFunction) (lua_State *L);
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
typedef struct luaL_Reg {
	const char* name;
	lua_CFunction func;
} luaL_Reg;

CREATE_CALLABLE_SIGNATURE(lua_call, void, "lua_call", "", 0, lua_State*, int, int)
CREATE_CALLABLE_SIGNATURE(lua_pcall, int, "lua_pcall", "", 0, lua_State*, int, int, int)
CREATE_CALLABLE_SIGNATURE(lua_gettop, int, "lua_gettop", "", 0, lua_State*)
CREATE_CALLABLE_SIGNATURE(lua_settop, void, "lua_settop", "", 0, lua_State*, int)
CREATE_CALLABLE_SIGNATURE(lua_tolstring, const char*, "lua_tolstring", "", 0, lua_State*, int, size_t*)
CREATE_CALLABLE_SIGNATURE(luaL_loadfile, int, "luaL_loadfile", "", 0, lua_State*, const char*)
CREATE_CALLABLE_SIGNATURE(lua_load, int, "lua_load", "", 0, lua_State*, lua_Reader, void*, const char*)
CREATE_CALLABLE_SIGNATURE(lua_setfield, void, "lua_setfield", "", 0, lua_State*, int, const char*)
CREATE_CALLABLE_SIGNATURE(lua_createtable, void, "lua_createtable", "", 0, lua_State*, int, int)
CREATE_CALLABLE_SIGNATURE(lua_insert, void, "lua_insert", "", 0, lua_State*, int)
CREATE_CALLABLE_SIGNATURE(lua_newstate, lua_State*, "lua_newstate", "", 0, lua_Alloc, void*)
CREATE_CALLABLE_SIGNATURE(lua_close, void, "lua_close", "", 0, lua_State*)

CREATE_CALLABLE_SIGNATURE(lua_rawset, void, "lua_rawset", "", 0, lua_State*, int)
CREATE_CALLABLE_SIGNATURE(lua_settable, void, "lua_settable", "", 0, lua_State*, int)

CREATE_CALLABLE_SIGNATURE(lua_pushnumber, void, "lua_pushnumber", "", 0, lua_State*, double)
CREATE_CALLABLE_SIGNATURE(lua_pushinteger, void, "lua_pushinteger", "", 0, lua_State*, ptrdiff_t)
CREATE_CALLABLE_SIGNATURE(lua_pushboolean, void, "lua_pushboolean", "", 0, lua_State*, bool)
CREATE_CALLABLE_SIGNATURE(lua_pushcclosure, void, "lua_pushcclosure", "", 0, lua_State*, lua_CFunction, int);
CREATE_CALLABLE_SIGNATURE(lua_pushlstring, void, "lua_pushlstring", "", 0, lua_State*, const char*, size_t)

CREATE_CALLABLE_SIGNATURE(luaI_openlib, void, "luaL_openlib", "", 0, lua_State*, const char*, const luaL_Reg*, int)
CREATE_CALLABLE_SIGNATURE(luaL_ref, int, "luaL_ref", "", 0, lua_State*, int);
CREATE_CALLABLE_SIGNATURE(lua_rawgeti, void, "lua_rawgeti", "", 0, lua_State*, int, int);
CREATE_CALLABLE_SIGNATURE(luaL_unref, void, "luaL_unref", "", 0, lua_State*, int, int);
CREATE_CALLABLE_CLASS_SIGNATURE(do_game_update, void*, "_ZN3dsl12EventManager6updateEv", "", 0, int*, int*)
CREATE_CALLABLE_CLASS_SIGNATURE(luaL_newstate, int, "luaL_newstate", "", 0, char, char, int)

SubHook* gameUpdateDetour;
SubHook* newStateDetour;
SubHook* luaCallDetour;
SubHook* luaCloseDetour;

// lua c-functions

#define LUA_REGISTRYINDEX	(-10000)
#define LUA_GLOBALSINDEX	(-10002)

// more bloody lua shit
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5
#define LUA_ERRFILE     (LUA_ERRERR+1)

std::list<lua_State*> activeStates;
void add_active_state(lua_State* L){
	activeStates.push_back(L);
}

void remove_active_state(lua_State* L){
	activeStates.remove(L);
}

bool check_active_state(lua_State* L){
	std::list<lua_State*>::iterator it;
	for (it = activeStates.begin(); it != activeStates.end(); it++){
		if (*it == L) {
			return true;
		}
	}
	return false;
}

void lua_newcall(lua_State* L, int args, int returns){
    SubHook::ScopedRemove hookLock( (SubHook*)luaCallDetour );

	int result = lua_pcall_orig(L, args, returns, 0);
	if (result != 0) {
		size_t len;
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
	}
}

int luaH_getcontents(lua_State* L, bool files){
	size_t len;
	const char* dirc = lua_tolstring_orig(L, 1, &len);
	std::string dir(dirc, len);
	std::vector<std::string> directories;

	try {
		directories = Util::GetDirectoryContents(dir, files);
	}
	catch (int e){
		lua_pushboolean_orig(L, false);
		return 1;
	}

	lua_createtable_orig(L, 0, 0);

	std::vector<std::string>::iterator it;
	int index = 1;
	for (it = directories.begin(); it < directories.end(); it++){
		if (*it == "." || *it == "..") continue;
		lua_pushinteger_orig(L, index);
		lua_pushlstring_orig(L, it->c_str(), it->length());
		lua_settable_orig(L, -3);
		index++;
	}

	return 1;
}

int luaF_getdir(lua_State* L){
	return luaH_getcontents(L, true);
}

int luaF_getfiles(lua_State* L){
	return luaH_getcontents(L, false);
}

int luaF_directoryExists(lua_State* L){
	size_t len;
	const char* dirc = lua_tolstring_orig(L, 1, &len);
	bool doesExist = Util::DirectoryExists(dirc);
	lua_pushboolean_orig(L, doesExist);
	return 1;
}

int luaF_unzipfile(lua_State* L){
	size_t len;
	const char* archivePath = lua_tolstring_orig(L, 1, &len);
	const char* extractPath = lua_tolstring_orig(L, 2, &len);

	ZIPArchive* archive = new ZIPArchive(archivePath, extractPath);
	archive->ReadArchive();
	delete archive;
	return 0;
}

int luaF_removeDirectory(lua_State* L){
	size_t len;
	const char* directory = lua_tolstring_orig(L, 1, &len);
	bool success = Util::RemoveEmptyDirectory(directory);
	lua_pushboolean_orig(L, success);
	return 1;
}

int luaF_pcall(lua_State* L){
	int args = lua_gettop_orig(L);

	int result = lua_pcall_orig(L, args - 1, -1, 0);
	if (result == LUA_ERRRUN){
		size_t len;
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
		return 0;
	}
	lua_pushboolean_orig(L, result == 0);
	lua_insert_orig(L, 1);

	//if (result != 0) return 1;

	return lua_gettop_orig(L);
}

int luaF_dofile(lua_State* L){

	size_t length = 0;
	const char* filename = lua_tolstring_orig(L, 1, &length);
	int error = luaL_loadfile_orig(L, filename);
	if (error == LUA_ERRSYNTAX){
		size_t len;
		Logging::Log(filename, Logging::LOGGING_ERROR);
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
	}
	error = lua_pcall_orig(L, 0, 0, 0);
	if (error == LUA_ERRRUN){
		size_t len;
		Logging::Log(filename, Logging::LOGGING_ERROR);
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
	}
	return 0;
}

struct lua_http_data {
	int funcRef;
	int progressRef;
	int requestIdentifier;
	lua_State* L;
};

void return_lua_http(void* data, std::string& urlcontents){
	lua_http_data* ourData = (lua_http_data*)data;

	if (!check_active_state(ourData->L)) {
		delete ourData;
		return;
	}

	lua_rawgeti_orig(ourData->L, LUA_REGISTRYINDEX, ourData->funcRef);
	lua_pushlstring_orig(ourData->L, urlcontents.c_str(), urlcontents.length());
	lua_pushinteger_orig(ourData->L, ourData->requestIdentifier);
	lua_pcall_orig(ourData->L, 2, 0, 0);
	luaL_unref_orig(ourData->L, LUA_REGISTRYINDEX, ourData->funcRef);
	luaL_unref_orig(ourData->L, LUA_REGISTRYINDEX, ourData->progressRef);
	delete ourData;
}

void progress_lua_http_orig(void* data, long progress, long total){
	lua_http_data* ourData = (lua_http_data*)data;

	if (!check_active_state(ourData->L)){
		return;
	}

	if (ourData->progressRef == 0) return;
	lua_rawgeti_orig(ourData->L, LUA_REGISTRYINDEX, ourData->progressRef);
	lua_pushinteger_orig(ourData->L, ourData->requestIdentifier);
	lua_pushinteger_orig(ourData->L, progress);
	lua_pushinteger_orig(ourData->L, total);
	lua_pcall_orig(ourData->L, 3, 0, 0);
}

static int HTTPReqIdent = 0;

int luaF_dohttpreq(lua_State* L){
	Logging::Log("Incoming HTTP Request/Request");

	int args = lua_gettop_orig(L);
	int progressReference = 0;
	if (args >= 3){
		progressReference = luaL_ref_orig(L, LUA_REGISTRYINDEX);
	}

	int functionReference = luaL_ref_orig(L, LUA_REGISTRYINDEX);
	size_t len;
	const char* url_c = lua_tolstring_orig(L, 1, &len);
	std::string url = std::string(url_c, len);

	Logging::Log(url);
	Logging::Log(std::to_string(functionReference));

	lua_http_data* ourData = new lua_http_data();
	ourData->funcRef = functionReference;
	ourData->progressRef = progressReference;
	ourData->L = L;

	HTTPReqIdent++;
	ourData->requestIdentifier = HTTPReqIdent;

	HTTPItem* reqItem = new HTTPItem();
	reqItem->call = return_lua_http;
	reqItem->data = ourData;
	reqItem->url = url;

	if (progressReference != 0){
		reqItem->progress = progress_lua_http_orig;
	}

	HTTPManager::GetSingleton()->LaunchHTTPRequest(reqItem);
	lua_pushinteger_orig(L, HTTPReqIdent);
	return 1;
}

CConsole* gbl_mConsole = NULL;

int luaF_createconsole(lua_State* L){
	if (gbl_mConsole) return 0;
	gbl_mConsole = new CConsole();
	return 0;
}

int luaF_destroyconsole(lua_State* L){
	if (!gbl_mConsole) return 0;
	delete gbl_mConsole;
	gbl_mConsole = NULL;
	return 0;
}

int luaF_print(lua_State* L){
	size_t len;
	const char* str = lua_tolstring_orig(L, 1, &len);
	Logging::Log(str, Logging::LOGGING_LUA);
	return 0;
}

int updates = 0;
std::thread::id main_thread_id;

void* do_game_update_new(void* thislol, int edx, int* a, int* b){

    SubHook::ScopedRemove hookLock( (SubHook*)gameUpdateDetour );
	// If someone has a better way of doing this, I'd like to know about it.
	// I could save the this pointer?
	// I'll check if it's even different at all later.
	if (std::this_thread::get_id() != main_thread_id){
		return do_game_update_orig(thislol, a, b);
	}

	if (updates == 0){
		HTTPManager::GetSingleton()->init_locks();
	}

	if (updates > 1){
		EventQueueM::GetSingleton()->ProcessEvents();
	}

	updates++;
	return do_game_update_orig(thislol, a, b);
}

// Random dude who wrote what's his face?
// I 'unno, I stole this method from the guy who wrote the 'underground-light-lua-hook'
// Mine worked fine, but this seems more elegant.
//int luaL_newstate_new(void* thislol, int edx, char no, char freakin, int clue){
lua_State* lua_newstate_new( lua_Alloc f, void* ud){
    SubHook::ScopedRemove hookLock( (SubHook*)newStateDetour );

	lua_State* L = lua_newstate_orig( f, ud);

	if (!L) return NULL;

	add_active_state(L);

	int stack_size = lua_gettop_orig(L);

    printf( "Createing functions\n" );
	CREATE_LUA_FUNCTION(luaF_pcall, "pcall")
	CREATE_LUA_FUNCTION(luaF_dofile, "dofile")
	CREATE_LUA_FUNCTION(luaF_dohttpreq, "dohttpreq")
	CREATE_LUA_FUNCTION(luaF_print, "log")
	CREATE_LUA_FUNCTION(luaF_unzipfile, "unzip")

	luaL_Reg consoleLib[] = { { "CreateConsole", luaF_createconsole }, { "DestroyConsole", luaF_destroyconsole }, { NULL, NULL } };
	luaI_openlib_orig(L, "console", consoleLib, 0);

	luaL_Reg fileLib[] = { { "GetDirectories", luaF_getdir }, { "GetFiles", luaF_getfiles }, { "RemoveDirectory", luaF_removeDirectory }, { "DirectoryExists", luaF_directoryExists }, { NULL, NULL } };
	luaI_openlib_orig(L, "file", fileLib, 0);

	int result;
	Logging::Log("Initiating Hook");
	
    printf( "Loading base.lua\n" );
	result = luaL_loadfile_orig(L, "mods/base/base.lua");
	if (result == LUA_ERRSYNTAX){
		size_t len;
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
		return L;
	}
	result = lua_pcall_orig(L, 0, 1, 0);
	if (result == LUA_ERRRUN){
		size_t len;
		Logging::Log(lua_tolstring_orig(L, -1, &len), Logging::LOGGING_ERROR);
		return L;
	}

    printf( "Setting opt\n" );
	lua_settop_orig(L, stack_size);
	return L;
}

void luaF_close(lua_State* L){
    SubHook::ScopedRemove hookLock( (SubHook*)luaCloseDetour );

	remove_active_state(L);
	lua_close_orig(L);
}


static HTTPManager mainManager;

void InitiateStates(){

	main_thread_id = std::this_thread::get_id();

	SignatureSearch::Search();

    printf( "Hooking %p\n", do_game_update_orig );
    gameUpdateDetour = new SubHook((void*)do_game_update_orig, (void*)do_game_update_new);
    gameUpdateDetour->Install();

    printf( "Hooking %p\n", lua_newstate_orig );
    newStateDetour = new SubHook((void*)lua_newstate_orig, (void*)lua_newstate_new);
    newStateDetour->Install();

    printf( "Hooking %p\n", lua_call_orig );
    luaCallDetour = new SubHook((void*)lua_call_orig, (void*)lua_newcall);
    luaCallDetour->Install();

    printf( "Hooking %p\n", lua_close_orig );
    luaCloseDetour = new SubHook((void*)lua_close_orig, (void*)luaF_close);
    luaCloseDetour->Install();

    Logging::Log( "Finished installing hooks", Logging::LOGGING_LOG );
	
	new EventQueueM();
}

void DestroyStates(){
    delete gameUpdateDetour;
    delete newStateDetour;
    delete luaCallDetour;
    delete luaCloseDetour;
}
