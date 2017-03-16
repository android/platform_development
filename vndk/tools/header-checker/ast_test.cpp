void dlopen(const char *, int flags);

static int foo() {
  return 1;
}

// Dummy for test.
#define RTLD_NOW 1
int main () {
  foo();
  dlopen("foo", RTLD_NOW);
  return 0;
}
