#include "abstract_class.h"

class HighVolumeSpeaker : public SuperSpeaker {
 public:
  virtual void Speak() override;
  virtual void Listen() override;
};
