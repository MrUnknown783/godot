#ifndef RESOURCE_LUA_H
#define RESOURCE_LUA_H

#include "core/io/json.h"

class LuaResource : public Resource {
	GDCLASS(LuaResource, Resource);

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_code", "code"), &LuaResource::set_code);
		ClassDB::bind_method(D_METHOD("get_code"), &LuaResource::get_code);

		ADD_PROPERTY(PropertyInfo(Variant::STRING, "content"), "set_code", "get_code");
	}

private:
	String content;

public:
	Error load_file(const String& p_path);
	Error save_file(const String& p_path, const Ref<Resource>& p_resource);

	void set_code(const String code);
	String get_code();
};

class ResourceFormatLoaderLua : public ResourceFormatLoader {
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "", Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual void get_recognized_extensions(List<String>* p_extensions) const;
	virtual bool handles_type(const String& p_type) const;
	virtual String get_resource_type(const String& p_path) const;
};
#endif // RESOURCE_JSON_H
