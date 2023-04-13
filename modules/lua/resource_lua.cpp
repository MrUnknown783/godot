#include "resource_lua.h"

Error LuaResource::load_file(const String& p_path) {
	Error error;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &error);
	if (error != OK) {
		if (!file.is_null()) {
			file->close();
		}
		return error;
	}

	String json_string = String("");
	while (!file->eof_reached()) {
		json_string += file->get_line() + "\n";
	}
	file->close();

	content = String(json_string);

	return OK;
}

Error LuaResource::save_file(const String& p_path, const Ref<Resource>& p_resource) {
	Error error;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &error);
	if (error != OK) {
		if (!file.is_null()) {
			file->close();
		}
		return error;
	}

	Ref<LuaResource> json_ref = p_resource;
	JSON json;

	file->store_string(json.stringify(json_ref->get_code(), "    "));
	file->close();
	return OK;
}

void LuaResource::set_code(const String code) {
	content = code;
}

String LuaResource::get_code() {
	return content;
}

///////////////////

Ref<Resource> ResourceFormatLoaderLua::load(const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode) {
	Ref<LuaResource> json = memnew(LuaResource);
	if (r_error) {
		*r_error = OK;
	}
	Error err = json->load_file(p_path);
	return json;
}

void ResourceFormatLoaderLua::get_recognized_extensions(List<String>* r_extensions) const {
	if (!r_extensions->find("lua")) {
		r_extensions->push_back("lua");
	}
}

String ResourceFormatLoaderLua::get_resource_type(const String& p_path) const {
	return "LuaResource";
}

bool ResourceFormatLoaderLua::handles_type(const String& p_type) const {
	return ClassDB::is_parent_class(p_type, "LuaResource");
}
