
namespace dummynamespace {

enum class CppEnum {
	kFirstElement, kSecondElement, kThirdElement,
};

class CppDummyClass {};	

class CppClass {
public:
	CppClass();
	CppClass(int dummyInt);
	~CppClass();

	int doFoo();
	static int doStaticFoo();

	int doOverloaded();
	int doOverloaded(int x);

	void noParameterName(int);

	int returnInt();
	void passInInt(int foo);

	static CppEnum returnEnum();
	static void passInEnum(CppEnum foo);

	char returnChar();
	void passInChar(char foo);

	CppDummyClass& returnRef();
	void passPointer(CppDummyClass* dummy);
	void passRef(const CppDummyClass& dummy);
	void passByValue(CppDummyClass dummy);
	CppDummyClass returnByValue();

	void passFunctionPointer( void* (*foo)(int) );

private:
	int dummyInt_;
	CppDummyClass* dummyClass_;
};

}