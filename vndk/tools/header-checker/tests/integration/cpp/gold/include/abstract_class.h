#if GOLDEN_VTABLE_DIFF
#define VIRTUAL_FUNCTIONS \
    virtual void Listen() = 0; \
    virtual void Speak() = 0; \
    static void SpeakLouder();
#else
#define VIRTUAL_FUNCTIONS \
    virtual void Speak() = 0; \
    virtual void Listen() = 0; \
    void SpeakLouder();
#endif

#if GOLDEN_ENUM_EXTENSION
#define LOUD_SUPERLATIVES \
    Loudest = 3, \
    LouderThanLoudest = 4
#elif GOLDEN_ENUM_DIFF
#define LOUD_SUPERLATIVES \
    Loudest = -1,
#else
#define LOUD_SUPERLATIVES \
    Loudest = 3,
#endif


class SuperSpeaker {
  enum Volume {
    Loud = 1,
    Louder = 2,
    LOUD_SUPERLATIVES
  };
 public:
  static SuperSpeaker *CreateSuperSpeaker(int id);
  VIRTUAL_FUNCTIONS
  Volume SpeakLoud();
  void SpeakLoudest() { };

  virtual ~SuperSpeaker() { }
 private:
  int mSpeakderId;
};
