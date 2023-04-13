#ifndef TEST_NODE_H
#define TEST_NODE_H

#include "scene/main/node.h"
#include "core/lua/lua.hpp"
#include <core/variant/callable.h>
#include <regex>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
using namespace std;

struct CallbackFunctionInfo {
	Callable function;
	String name;
	int inputArgsCount;
};

struct Info {
	Callable function;
	int inputArgsCount;
};

//static vector<CallbackFunctionInfo> funcs = vector<CallbackFunctionInfo>();

class LuaNode : public Node {
	GDCLASS(LuaNode, Node);

private:
	bool stateClosed = true;
	lua_State* state;
	String originalCode;
	String compiledCode;

	int getFunctionId(string name);
	void open_state();
	void close_state();

	//map<lua_CFunction, Info> funcs;

protected:
	static void _bind_methods();

public:
	void add_global(String name, Variant input);
	void init();
	String get_compiled_code();
	String get_original_code();

	void _process(float delta);

	bool compile(String code);
	bool load_lua(String code);
	void call_update();
	Variant call_function(String functionName, Variant args);

	//void register_function(String functionName, Ref<FuncRef> function);
	void register_function(String functionName, Callable function);

	String print_error();

	LuaNode();
	~LuaNode();
};

static map<String, CallbackFunctionInfo> funcs = map<String, CallbackFunctionInfo>();
static map<lua_State*, LuaNode*> instances = map<lua_State*, LuaNode*>();

//typedef int (TestNode::* lua_CFunction) (lua_State* L);

#endif
