#include <utils/StrongPointer.h>
#include <utils/RefBase.h>

namespace dummynamespace {
	using namespace android;

	class RefCountedClass : public VirtualLightRefBase {};

	class HwuiTest final {
	public:
		static sp<RefCountedClass> returnSp();
		static void passSp(sp<RefCountedClass> foo);
		
		static void passSpRef(const sp<RefCountedClass>& foo);
	};
}