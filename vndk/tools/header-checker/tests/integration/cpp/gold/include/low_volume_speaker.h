#include "abstract_class.h"

#if GOLDEN_MEMBER_DIFF
#define CLASS_MEMBERS \
    int speaker_int; \
    float speaker_float;
#else
#define CLASS_MEMBERS \
    long long speaker_float; \
    long long *speaker_int;
#endif


class LowVolumeSpeaker : public SuperSpeaker {
 public:
  virtual void Speak() override;
  virtual void Listen() override;
 private:
  CLASS_MEMBERS
};
