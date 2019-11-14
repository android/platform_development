
#include <memory>

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

	void passFunctionPointer(void* (*foo)(int));

	static std::unique_ptr<CppClass> returnUniquePointer();
	static void passUniquePointer(std::unique_ptr<CppClass> foo);

	static std::shared_ptr<CppClass> returnSharedPtr();
	static void passSharedPtr(std::shared_ptr<CppClass> foo);

private:
	int dummyInt_;
	CppDummyClass* dummyClass_;
};

}