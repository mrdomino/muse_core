/* Copyright 2014 Interaxon, Inc. */

#define IX_UNUSED(x) ((void)x)

#if defined _MSC_VER

#define IX_CCALL __cdecl
#pragma section(".CRT$XCU",read)
#define IX_INITIALIZER(f) \
  static void __cdecl f(void); \
  __declspec(allocate(".CRT$XCU")) void (__cdecl*f##_)(void) = f; \
  static void __cdecl f(void)

#elif defined __GNUC__

#define IX_CCALL
#define IX_INITIALIZER(f) \
  static void f(void) __attribute__((constructor)); \
  static void f()

#endif
