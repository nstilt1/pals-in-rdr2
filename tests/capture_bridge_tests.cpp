#include <windows.h>
#include <cmath>
#include <iostream>
#include <limits>
#include "../bridge/CaptureKitBridge.h"

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!library) { std::cerr << "DLL load failed: " << GetLastError(); return 2; }
    auto version = reinterpret_cast<decltype(&ckb_api_version)>(GetProcAddress(library,"ckb_api_version"));
    auto chance = reinterpret_cast<decltype(&ckb_v1_capture_chance)>(GetProcAddress(library,"ckb_v1_capture_chance"));
    auto shakes = reinterpret_cast<decltype(&ckb_v1_judge_shakes)>(GetProcAddress(library,"ckb_v1_judge_shakes"));
    int failures = 0;
    const auto check = [&](bool ok, const char* name) { if (!ok) {std::cerr << name << '\n'; ++failures;} };
    if (!version || !chance || !shakes) { FreeLibrary(library); return 2; }
    check(version()==1,"versioned C exports available");
    static_assert(sizeof(CkbInputs)==56 && sizeof(CkbRules)==24 && sizeof(CkbBreakdown)==24);
    CkbRules rules{0.8,10.0,1.2};
    CkbInputs input{8.0,3,0,100.0,100.0,1.0,0.0,0,0};
    CkbBreakdown result{-99,-99,-99};
    check(chance(&rules,&input,&result)==0 && std::abs(result.chance-0.15)<1e-10,"upstream formula through C ABI");
    input.hp=10;
    check(chance(&rules,&input,&result)==0 && result.chance>0.15,"injury increases chance");
    input.uncapturable=1;
    check(chance(&rules,&input,&result)==0 && result.chance==0,"protected eligibility flag");
    input.uncapturable=0; input.hp=0; result.chance=-99;
    check(chance(&rules,&input,&result)==-2 && result.chance==-99,"reject dead target without changing output");
    input.hp=std::numeric_limits<double>::quiet_NaN();
    check(chance(&rules,&input,&result)==-2,"reject NaN input");
    check(chance(nullptr,&input,&result)==-1,"reject null config");
    double weights[]{0.2,0.3,0.5}, rolls[]{0.1,0.1,0.1}; uint32_t passed=99;
    check(shakes(1,weights,rolls,3,&passed)==0 && passed==3,"certain capture passes all shakes");
    check(shakes(0,weights,rolls,3,&passed)==0 && passed==0,"zero chance passes none");
    weights[0]=0.5; passed=99;
    check(shakes(.5,weights,rolls,3,&passed)==-2 && passed==99,"invalid weights do not change output");
    check(shakes(.5,weights,rolls,9,&passed)==-2,"count bound checked before reading arrays");
    FreeLibrary(library);
    return failures ? 1 : 0;
}
