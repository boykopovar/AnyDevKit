#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_CHECKS_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_CHECKS_HPP

#include <stdexcept>

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<typename TException, typename TAction>
void ExpectException(TAction action) {
    bool caught = false;
    try { action(); }
    catch (const TException&) { caught = true; }
    Require(caught, "Expected exception was not thrown");
}

void CheckContainers();
void CheckMemory();
void CheckAlgorithms();
void CheckUtilities();
void CheckStrings();
void CheckNumeric();
void CheckExceptions();
void CheckCoroutines();
void CheckAtomics();
void CheckLocale();
void CheckStreams();
void CheckRegex();
void CheckFormatting();
void CheckCCompatibility();

#endif
