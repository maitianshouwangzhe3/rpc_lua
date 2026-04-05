
extern "C" {
#include "pb.h"
#include "lfs.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#ifdef rpc_EXPORTS
#ifdef _WIN32
#define RPC_EXPORT __declspec(dllexport)
#else
#define RPC_EXPORT
#endif
#else
#define RPC_EXPORT
#endif

#include "server.h"
#include "client.h"

struct context {
    server* svr = nullptr;
    client* cli = nullptr;
};

static context g_context;

static server* get_server(lua_State* L) {
    intptr_t p;
    memcpy(&p, lua_getextraspace(L), LUA_EXTRASPACE);
    return reinterpret_cast<context*>(p)->svr;
}

static client* get_client(lua_State* L) {
    intptr_t p;
    memcpy(&p, lua_getextraspace(L), LUA_EXTRASPACE);
    return reinterpret_cast<context*>(p)->cli;
}

static int create_server(lua_State* L) {
    server* svr = new server();
    if (!svr) {
        lua_pushnil(L);
        return 1;
    }

    g_context.svr = svr;
    intptr_t p = (intptr_t)&g_context;
    memcpy(lua_getextraspace(L), &p, LUA_EXTRASPACE);
    lua_push_object(L, svr);
    return 1;
}

static int create_client(lua_State* L) { 
    client* cli = new client();
    if (!cli) {
        lua_pushnil(L);
        return 1;
    }

    g_context.cli = cli;
    intptr_t p = (intptr_t)&g_context;
    memcpy(lua_getextraspace(L), &p, LUA_EXTRASPACE);
    lua_push_object(L, cli);
    return 1;
}

static int traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static int callback(lua_State* L) {
    luaL_checktype(L,1,LUA_TFUNCTION);
	lua_settop(L,1);
	callback_context* cb_context_ = (callback_context*)lua_newuserdatauv(L, sizeof(callback_context), 2);
	cb_context_->L = lua_newthread(L);
	lua_pushcfunction(cb_context_->L, traceback);
	lua_setiuservalue(L, -2, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, "CallbackContext");
	lua_setiuservalue(L, -2, 2);
	lua_setfield(L, LUA_REGISTRYINDEX, "CallbackContext");
	lua_xmove(L, cb_context_->L, 1);
    server* svr = get_server(L);
    svr->call_back(cb_context_);
    return 0;
}

extern "C" {
RPC_EXPORT LUALIB_API int luaopen_rpc(lua_State* L) {
    luaL_checkversion(L);
    
	luaL_Reg l[] = {
        {"new_server", create_server},
        {"new_client", create_client},
        {"callback", callback},
		{ NULL, NULL },
	};

    luaL_newlib(L, l);
    return 1;
}

}

#define REGISTER_LIBRARYS(name, lua_c_fn) \
    luaL_requiref(L, name, lua_c_fn, 0); \
    lua_pop(L, 1) /* remove lib */

void init(lua_State *L) {
    REGISTER_LIBRARYS("rpc", luaopen_rpc);

    REGISTER_LIBRARYS("rpc.pb", luaopen_rpc_pb);

    REGISTER_LIBRARYS("rpc.lfs", luaopen_rpc_lfs);

    // REGISTER_LIBRARYS("onbnet.loggerdriver", luaopen_onbnet_loggerdriver);
}

int main(int argc, char** argv) { 
    lua_State *L = luaL_newstate();
	lua_gc(L, LUA_GCSTOP, 0);
	lua_pushboolean(L, 1);
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
	luaL_openlibs(L);
    init(L);
	lua_pushcfunction(L, traceback);
	assert(lua_gettop(L) == 1);

	const char * loader = argv[1];
	int r = luaL_loadfile(L,loader);
	if (r != LUA_OK) {
		return 1;
	}

	r = lua_pcall(L,0,0,1);
	if (r != LUA_OK) {
		lua_pop(L, 1);
		printf("error: %s\n", lua_tostring(L, -1));
		return 1;
	}

	lua_settop(L,0);

	lua_gc(L, LUA_GCRESTART, 0);
	return 0;

}