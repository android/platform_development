#include <include/core/SkRefCnt.h>

namespace dummynamespace {

	class RefCountedClass : public SkRefCnt {};

	class HwuiTest final {
	public:
		static sk_sp<RefCountedClass> returnSksp();
		static void passSksp(sk_sp<RefCountedClass> foo);

		static void passStrongPointerRef(const sk_sp<RefCountedClass>& foo);
	};
}