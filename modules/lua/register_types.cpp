#include "register_types.h"
#include "resource_lua.h"

static Ref<ResourceFormatLoaderLua> lua_loader;

void initialize_lua_module(ModuleInitializationLevel p_level) {
	lua_loader.instantiate();
	ResourceLoader::add_resource_format_loader(lua_loader);
}

void uninitialize_lua_module(ModuleInitializationLevel p_level) {
	ResourceLoader::remove_resource_format_loader(lua_loader);
	lua_loader.unref();
}
