#ifndef ANYDEVKIT_WINDOWS_ENTRY_HPP
#define ANYDEVKIT_WINDOWS_ENTRY_HPP

extern "C" {

void AnyDevKitEntry();
int AnyDevKitInvokeMain(int argc, char** argv);
int AnyDevKitRegisterDestructor(void (*callback)());
void __main();

}

#endif
