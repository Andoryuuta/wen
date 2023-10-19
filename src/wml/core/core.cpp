#include "core.h"

WML::Core& WML::Core::GetInstance() {
    static WML::Core instance;
    return instance;
}