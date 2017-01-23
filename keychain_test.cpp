#include "keychain.h"
#include <iostream>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Keychain", "[keychain]") {
    Keychain keychain("./demo.agilekeychain", "demo");
    for (const auto& item : keychain) {
        std::cout << "Title: " << item.second.title << std::endl
                  << "UUID: " << item.second.uuid << std::endl
                  << "Notes: " << item.second.notes << std::endl;

        for (const auto& section : item.second.sections) {
            std::cout << "Section: " << section.first << std::endl;
            for (const auto& field : section.second) {
                std::cout << "\t" << field.name << ": " << field.value << std::endl;
            }
        }
    }
}
