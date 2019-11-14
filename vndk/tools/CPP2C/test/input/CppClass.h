
class CppDummyClass {};

class CppClass {
public:
	CppClass() { dummyClass_ = new CppDummyClass(); }
	CppClass(int dummyInt) : dummyInt_(dummyInt) { CppClass(); }
	~CppClass() { delete dummyClass_; }

	int doFoo();
	static int doStaticFoo();

	int doOverloaded();
	int doOverloaded(int x);

	void noParameterName(int);

	int returnInt();
	void passInInt(int foo);

	char returnChar();
	void passInChar(char foo);

	CppDummyClass& returnRef() { return *dummyClass_;}
	void passPointer(CppDummyClass* dummy);
	void passRef(const CppDummyClass& dummy);
	void passByValue(CppDummyClass dummy);
	CppDummyClass returnByValue();

	void passFunctionPointer( void* (*foo)(int) );

private:
	int dummyInt_;
	CppDummyClass* dummyClass_;
};
