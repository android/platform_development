
#include <memory>

namespace dummynamespace {

class CppClass {
public:
	static std::unique_ptr<CppClass> returnUniquePointer();
	static void passUniquePointer(std::unique_ptr<CppClass> foo);

	static std::shared_ptr<CppClass> returnSharedPtr();
	static void passSharedPtr(std::shared_ptr<CppClass> foo);
};

}