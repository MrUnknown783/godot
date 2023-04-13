#include "TestNode.h"

TestNode::TestNode() {
	init();
}

TestNode::~TestNode() {
	//lua_close(state);
}

int callback(lua_State* state);
void add_func_into_register(String functionName, Callable function);
Array get_lua_function_input_params(lua_State* state, int ind);

int print(lua_State* state) {
	auto params = get_lua_function_input_params(state, 1);

	for (int i = 0; i < params.size(); i++)
	{
		print_line((String)params[i]);
	}

	return 0;
}


void TestNode::init() {
	if (!stateClosed) {
		close_state();
	}

	open_state();
	luaL_openlibs(state);
}

void TestNode::open_state() {
	state = luaL_newstate();
	instances.insert({ state, this });
	stateClosed = false;

	lua_register(state, "print_debug", print);
}

void TestNode::close_state() {
	//instances.erase(state);
	state = luaL_newstate();
	stateClosed = true;
}

void TestNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init"), &TestNode::init);
	ClassDB::bind_method(D_METHOD("_process", "delta"), &TestNode::_process);
	ClassDB::bind_method(D_METHOD("load_lua", "code"), &TestNode::load_lua);
	ClassDB::bind_method(D_METHOD("call_update"), &TestNode::call_update);
	//ClassDB::bind_method(D_METHOD("register_function", "functionName", "function"), &TestNode::register_function);
	ClassDB::bind_method(D_METHOD("register_function", "functionName", "callback"), &TestNode::register_function);
	ClassDB::bind_method(D_METHOD("compile", "code"), &TestNode::compile);
	ClassDB::bind_method(D_METHOD("print_error"), &TestNode::print_error);

	ClassDB::bind_method(D_METHOD("get_compiled_code"), &TestNode::get_compiled_code);
	ClassDB::bind_method(D_METHOD("get_origignal_code"), &TestNode::get_origignal_code);

	ClassDB::bind_method(D_METHOD("add_global", "name", "data"), &TestNode::add_global);
	ClassDB::bind_method(D_METHOD("call_function", "functionName", "args"), &TestNode::call_function);
}

String TestNode::get_compiled_code() {
	return compiledCode;
}

String TestNode::get_origignal_code() {
	return originalCode;
}

void push_value(lua_State* state, Variant value) {
	switch (value.get_type()) {
		case Variant::INT: lua_pushinteger(state, value); break;
		case Variant::STRING: lua_pushstring(state, (value.operator String()).to_char_star()); break;
	}
}

bool create_table(String name, lua_State* state, Variant input) {
	List<PropertyInfo> props = List<PropertyInfo>();
	input.get_property_list(&props);

	if (props.size() > 0) {
		auto indOffset = lua_gettop(state) + 1;
		lua_newtable(state);

		for (int i = 0; i < props.size(); i++)
		{
			Variant prop = input.get(props[i].name);

			switch (prop.get_type()) {
				case Variant::STRING: {
					String data = prop;
					lua_pushstring(state, data.to_char_star());
					lua_setfield(state, indOffset, props[i].name.to_char_star());
					break;
				}
				case Variant::OBJECT: {
					Callable data = prop;
					lua_pushcfunction(state, callback);
					lua_setfield(state, indOffset, props[i].name.to_char_star());

					add_func_into_register(name + "." + props[i].name, data);
					break;
				}
			}
		}

		return true;
	}

	return false;
}

void TestNode::add_global(String name, Variant input) {
	create_table(name, state, input);

	lua_setglobal(state, name.to_char_star());
}

int create_table_from_variant(lua_State* state, Variant input) {
	/*List<PropertyInfo> props = List<PropertyInfo>();
	input.get_property_list(&props);

	if (props.size() > 0) {
		lua_newtable(state);

		for (int i = 0; i < props.size(); i++)
		{
			if (props[i].type == Variant::OBJECT) {
				print_line(typeid(props[i].type).name());
			}
			else if (props[i].type == Variant::STRING) {
				lua_pushstring(state, props[i].name.to_char_star());
				lua_setfield(state, i + 1, props[i].name.to_char_star());
				//push_value(state, input.get(props[i].name));
			}
		}

		lua_settable(state, -3);

		return 1;
	}*/

	auto created = create_table("anon", state, input);

	if (created) {
		return 1;
	}

	return 0;
}

String get_lua_function_name(lua_State* state) {
	lua_Debug ar;

	lua_getstack(state, 0, &ar);
	lua_getinfo(state, "nSl", &ar);

	return String(ar.name);
}

tuple<String, String> get_function_metadata(lua_State* state) {
	lua_Debug ar;
	String functionName;
	String functionSource;

	lua_getstack(state, 0, &ar);
	lua_getinfo(state, "fnSlu", &ar);

	functionName = String(ar.name);

	regex functionCallRegex("[A-z,0-9,\\.]{0,}" + string(ar.name) + "\\s{0,}\\(.{0,}\\)");

	lua_getstack(state, 1, &ar);
	lua_getinfo(state, "fnSlu", &ar);

	smatch m;
	auto source = string(ar.source);
	regex_search(source, m, functionCallRegex);
	functionSource = String(m.str().c_str());

	return { functionName, functionSource };
}

String get_function_name_from_source(String originalName, String call) {
	auto params = "\\([a-zA-Z,0-9,\\,\\s,\\.,\\_,\\\",\\\']{0,}\\)";
	regex paramsRegex(params);
	auto functionRegexString = "([a-zA-Z,\\.,\\_]{0,}" + string(originalName.to_char_star()) + params + ")";
	regex functionRegex(functionRegexString);
	smatch name;
	auto functionCall = string(call.to_char_star());
	regex_search(functionCall, name, functionRegex);
	return String(regex_replace(name.str(), paramsRegex, "").c_str());
}

Dictionary lua_table_to_variant(lua_State* state, int id) {
	lua_gettable(state, id);
	lua_pushnil(state);
	Dictionary asd = Dictionary();

	while (lua_next(state, id) != 0)
	{
		//lua_pushnil(L);
		if (lua_isstring(state, -2)) {
			Variant a;
			auto name = StringName(lua_tostring(state, -2));
			auto text = String(lua_tostring(state, -1));
			asd[name] = text;
		}

		lua_pop(state, 1);
	}

	return asd;
}

Array get_lua_function_input_params(lua_State* state, int ind) {
	Array inputs = Array();

	for (int i = ind; i < 100; i++) {
		auto type = lua_type(state, i);

		if (type == LUA_TNIL || type == -1) {
			break;
		}
		else if (type == LUA_TSTRING) {
			inputs.push_back(lua_tostring(state, i));
		}
		else if (type == LUA_TNUMBER) {
			inputs.push_back(lua_tointeger(state, i));
		}
		else if (type == LUA_TTABLE) {
			inputs.push_back(lua_table_to_variant(state, i));
		}
	}

	return inputs;
}

int callback(lua_State* state) {
	try {
		auto functionMetadata = get_function_metadata(state);
		auto functionName = get<0>(functionMetadata);
		auto functionCall = get<1>(functionMetadata);
		auto fullFunctionName = get_function_name_from_source(functionName, functionCall);

		if (funcs.find(fullFunctionName) == funcs.end()) {
			return 0;
		}

		CallbackFunctionInfo func = funcs[fullFunctionName];
		Variant inputs = get_lua_function_input_params(state, 1);

		/*const Variant* asd = inputs.data();
		const Variant** q = &asd;*/

		auto output = func.function.callv(inputs);

		return create_table_from_variant(state, output);
	}
	catch (const std::exception e) {
		print_line(e.what());
	}
}

Variant TestNode::call_function(String functionName, Variant args) {
	lua_getglobal(state, functionName.to_char_star());

	if (lua_isfunction(state, -1))
	{
		int argsCount = create_table_from_variant(state, args);

		lua_pcall(state, argsCount, LUA_MULTRET, 0);

		return Variant(get_lua_function_input_params(state, lua_gettop(state)));
	}

	return NULL;
}

void TestNode::register_function(String functionName, Callable function)
{
	add_func_into_register(functionName, function);

	//lua_CFunction
	lua_register(state, functionName.to_char_star(), callback);
}

void add_func_into_register(String functionName, Callable function) {
	auto fName = functionName;

	if (funcs.find(fName) == funcs.end()) {
		funcs.erase(fName);
	}

	funcs.insert({ fName, {
		function,
		functionName,
		0
	} });
}

int TestNode::getFunctionId(string name)
{
	for (int i = 0; i < funcs.size(); i++)
	{
		//print_line("Match: " + funcs[i].name + " == " + name.c_str());
		/*if (funcs[i].name.match(String(name.c_str()))) {
			return i;
		}*/
	}

	return -1;
}

bool TestNode::compile(String _code) {
	string delimiter = "\n";
	int pos = 0;
	int offset = 0;
	string token;

	originalCode = _code;

	auto code = string(_code.to_char_star());

	regex functionCallRegex("[A-z,0-9,\\.]{0,}\\s{0,}\\(.{0,}\\)");
	regex paramsRegex("\\(.{0,}\\)");
	regex functionNameRegex("[A-z,0-9,\\.]{0,}");

	/*while (pos < code.length()) {
		pos = code.find(delimiter, offset);
		token = code.substr(offset, pos);
		int length = token.length();

		smatch m;
		regex_search(token, m, functionCallRegex);
		smatch params;
		smatch name;
		auto functionCall = m.str();
		regex_search(functionCall, params, paramsRegex);
		regex_search(functionCall, name, functionNameRegex);

		//print_line(m.str().c_str());
		//print_line(params.str().c_str());
		//print_line(name.str().c_str());

		int funcId = getFunctionId(name.str());
		bool haveArgs = params.str().length() > 2;

		//print_line(String(std::to_string(params.str().length()).c_str()));
		//print_line(String(std::to_string(params.str().length()).c_str()));

		if (funcId != -1) {
			string b = params.str().insert(1, std::to_string(funcId) + (haveArgs ? "," : ""));
			token = regex_replace(token, paramsRegex, b);
		}

		//token.insert(token.length(), "\n");

		code.erase(offset, length);
		code.insert(offset, token);
		std::cout << token << std::endl;
		offset = (int)pos + ((int)token.length() - length) + 1;
		//code.erase(0, pos + delimiter.length());
	}*/

	compiledCode = String(code.c_str());

	int result = luaL_loadstring(state, code.c_str());

	if (result != LUA_OK) {
		print_error();
		return false;
	}

	result = lua_pcall(state, 0, LUA_MULTRET, 0);

	if (result != LUA_OK) {
		print_error();
		return false;
	}

	return true;
}

void TestNode::_process(float delta) {
	
}

bool TestNode::load_lua(String code) {
	int result = luaL_loadstring(state, code.to_char_star());

	if (result != LUA_OK) {
		return false;
	}

	return true;
}

void TestNode::call_update()
{
	//ofstream myfile("C:\\Users\\unkno\\Documents\\GODOT\\log.txt");
	//myfile << "start" << endl;

	try {
		auto asd = lua_getglobal(state, "_update");

		//myfile << "1" << endl;
		//myfile << asd << endl;
		//myfile << "2" << endl;
		lua_call(state, 0, 0);
		//myfile << "3" << endl;
	}
	catch (const std::exception e) {
		//myfile << e.what() << endl;
	}
}

String TestNode::print_error()
{
	ofstream myfile("C:\\Users\\unkno\\Documents\\GODOT\\log.txt");
	myfile << "start" << endl;

	// The error message is on top of the stack.
	// Fetch it, print it and then pop it off the stack.
	try {
		myfile << "1" << endl;
		const char* message = lua_tostring(state, -1);
		myfile << message << endl;
		myfile << "2" << endl;
		lua_pop(state, 1);
		myfile << "end" << endl;

		myfile.close();
		return message;
	}
	catch (const std::exception e) {
		
		myfile << e.what() << endl;
		myfile.close();

		return e.what();
	}
}
