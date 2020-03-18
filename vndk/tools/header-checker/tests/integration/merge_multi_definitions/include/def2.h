
extern int var;

struct Struct {
  int member1;
};

struct Opaque;

void func(const struct Opaque *, const struct Struct *);
