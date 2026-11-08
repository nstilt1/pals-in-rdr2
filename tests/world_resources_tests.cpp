#include "../src/world_resources.h"
#include <iostream>
#include <limits>
int main() {
    int failures = 0;
    auto check = [&](bool value, const char* label) {
        if (!value) { ++failures; std::cerr << label << '\n'; }
    };
    pal::ResourceCatalog catalog;
    check(pal::parseResourceCatalog("# original test coordinates\ncamp 1 2 3\n"
          "deposit 42 test_fragment 4 5 6 1 0\ndeposit 43 test_sphere 7 8 9 0 1\n", catalog)
          && catalog.camps.size()==1 && catalog.deposits.size()==2, "valid local catalog");
    check(pal::atRegisteredCamp(catalog,{1,2,3},{1,2,3})
          && !pal::atRegisteredCamp(catalog,{1,2,3},{20,20,3})
          && !pal::atRegisteredCamp(catalog,{20,20,3},{20,20,3}),
          "craft requires nearby fire and trusted player camp anchor");
    check(!pal::atRegisteredCamp(catalog,{std::numeric_limits<double>::quiet_NaN(),0,0},{1,2,3}),
          "invalid position cannot authorize crafting");
    for (const char* bad : {"deposit -1 a 0 0 0 1 0", "deposit 0 a 0 0 0 1 0",
             "deposit 4294967296 a 0 0 0 1 0", "deposit 1 ../a 0 0 0 1 0",
             "deposit 1 a 0 0 0 0 0", "deposit 1 a 0 0 0 1000 0",
             "deposit 1 a 0 0 0 -1 0", "deposit 1 a 0 0 0 1 0 extra",
             "deposit 1 a 0 0 0 1 0\ndeposit 1 a 1 1 1 1 0",
             "camp 0 0 9999", "camp nan 0 0", "camp 0 0", "unknown 1"}) {
        auto prior = catalog.deposits.size();
        check(!pal::parseResourceCatalog(bad,catalog) && catalog.deposits.size()==prior,
              "bad config fails atomically without replacing trusted catalog");
    }
    std::string large;
    for(int i=0;i<65;++i) large += "deposit "+std::to_string(i+1)+" a 0 0 0 1 0\n";
    check(!pal::parseResourceCatalog(large,catalog), "bounded deposit count");
    large.clear(); for(int i=0;i<33;++i) large += "camp 0 0 0\n";
    check(!pal::parseResourceCatalog(large,catalog), "bounded camp count");
    check(!pal::parseResourceCatalog(std::string(65537,' '),catalog), "bounded config size");
    return failures ? 1 : 0;
}
