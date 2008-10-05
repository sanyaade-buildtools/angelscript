#include "utils.h"

namespace TestObject
{

#define TESTNAME "TestObject"



static const char *script1 =
"void TestObject()              \n"
"{                              \n"
"  Object a = TestReturn();     \n"
"  a.Set(1);                    \n"
"  TestArgVal(a);               \n"
"  Assert(a.Get() == 1);        \n"
"  TestArgRef(a);               \n"
"  Assert(a.Get() != 1);        \n"
"  TestProp();                  \n"
"  TestSysArgs();               \n"
"  TestSysReturn();             \n"
"  TestGlobalProperty();        \n"
"}                              \n"
"Object TestReturn()            \n"
"{                              \n"
"  return Object();             \n"
"}                              \n"
"void TestArgVal(Object a)      \n"
"{                              \n"
"}                              \n"
"void TestArgRef(Object &out a) \n"
"{                              \n"
"  a = Object();                \n"
"}                              \n"
"void TestProp()                \n"
"{                              \n"
"  Object a;                    \n"
"  a.val = 2;                   \n"
"  Assert(a.Get() == a.val);    \n"
"  Object2 b;                   \n"
"  b.obj = a;                   \n"
"  Assert(b.obj.val == 2);      \n"
"}                              \n"
"void TestSysReturn()             \n"
"{                                \n"
"  // return object               \n"
"  // by val                      \n"
"  Object a;                      \n"
"  a = TestReturnObject();        \n"
"  Assert(a.val == 12);           \n"
"  // by ref                      \n"
"  a.val = 12;                    \n"
"  TestReturnObjectRef() = a;     \n"
"  a = TestReturnObjectRef();     \n"
"  Assert(a.val == 12);           \n"
"}                                \n"
"void TestSysArgs()               \n"
"{                                \n"
"  Object a;                      \n"
"  a.val = 12;                    \n"
"  TestSysArgRef(a);              \n"
"  Assert(a.val == 2);            \n"
"  a.val = 12;                    \n"
"  TestSysArgVal(a);              \n"
"  Assert(a.val == 12);           \n"
"}                                \n"
"void TestGlobalProperty()        \n"
"{                                \n"
"  Object a;                      \n"
"  a.val = 12;                    \n"
"  TestReturnObjectRef() = a;     \n"
"  a = obj;                       \n"
"  obj = a;                       \n"
"}                                \n";

class CObject
{
public:
	CObject() {val = 0;/*printf("C:%x\n",this);*/}
	~CObject() {/*printf("D:%x\n", this);*/}
	void Set(int v) {val = v;}
	int Get() {return val;}
	int val;
};

void Construct(CObject *o)
{
	new(o) CObject();
}

void Destruct(CObject *o)
{
	o->~CObject();
}

class CObject2
{
public:
	CObject obj;
};

void Construct2(CObject2 *o)
{
	new(o) CObject2();
}

void Destruct2(CObject2 *o)
{
	o->~CObject2();
}

CObject TestReturnObject()
{
	CObject obj;
	obj.val = 12;
	return obj;
}

CObject obj;
CObject *TestReturnObjectRef()
{
	return &obj;
}

void TestSysArgVal(CObject obj)
{
	assert(obj.val == 12);
	obj.val = 0;
}

void TestSysArgRef(CObject &obj)
{
// We're not receiving the true object, only a reference to a place holder for the output value
	assert(obj.val == 0);
	obj.val = 2;
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = false;
	int r;
	int funcId;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->RegisterObjectType("Object", sizeof(CObject), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);	
	engine->RegisterObjectBehaviour("Object", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Object", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);
	funcId = engine->RegisterObjectMethod("Object", "void Set(int)", asMETHOD(CObject, Set), asCALL_THISCALL);
	engine->RegisterObjectMethod("Object", "int Get()", asMETHOD(CObject, Get), asCALL_THISCALL);
	engine->RegisterObjectProperty("Object", "int val", offsetof(CObject, val));

	engine->RegisterObjectType("Object2", sizeof(CObject2), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectBehaviour("Object2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Object2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("Object2", "Object obj", offsetof(CObject2, obj));

	engine->RegisterGlobalFunction("Object TestReturnObject()", asFUNCTION(TestReturnObject), asCALL_CDECL);
	engine->RegisterGlobalFunction("Object &TestReturnObjectRef()", asFUNCTION(TestReturnObjectRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestSysArgVal(Object)", asFUNCTION(TestSysArgVal), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestSysArgRef(Object &out)", asFUNCTION(TestSysArgRef), asCALL_CDECL);

	engine->RegisterGlobalProperty("Object obj", &obj);

	// Test objects with no default constructor
	engine->RegisterObjectType("ObjNoConstruct", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "TestObject()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->ExecuteString(0, "ObjNoConstruct a; a = ObjNoConstruct();");
	if( r != 0 )
	{
		fail = true;
		printf("%s: Failed\n", TESTNAME);
	}

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "Object obj; float r = 0; obj = r;");
	if( r >= 0 || bout.buffer != "ExecuteString (1, 32) : Error   : Can't implicitly convert from 'float&' to 'Object&'.\n" )
	{
		printf("%s: Didn't fail to compile as expected\n", TESTNAME);
		fail = true;
	}

	// Mustn't allow registration of assignment behaviour as global behaviour
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ASSIGNMENT, "Object &f(const Object &in, const Object &in)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 )
	{
		fail = true;
	}

	// Verify that the registered types can be enumerated
	int count = engine->GetObjectTypeCount();
	if( count != 3 )
		fail = true;
	asIObjectType *type = engine->GetObjectTypeByIndex(0);
	if( strcmp(type->GetName(), "Object") != 0 )
		fail = true;
	
	// Test calling an application registered method directly with context
	ctx = engine->CreateContext();
	ctx->Prepare(funcId);
	ctx->SetObject(&obj);
	ctx->SetArgDWord(0, 42);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( obj.val != 42 )
		fail = true;
	ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

