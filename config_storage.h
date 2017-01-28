#pragma once

#include <gtkmm.h>

#include "json.hpp"
// for convenience
using json = nlohmann::json;

class ConfigCache {
public:
    ConfigCache() {
        auto cache_dir_path = Glib::build_filename(Glib::get_user_cache_dir(), Glib::get_prgname());
        auto cache_dir = Gio::File::create_for_path(cache_dir_path);

        cache_file_path = Glib::build_filename(cache_dir_path, "cache.json");

        if (!cache_dir->query_exists()) {
            cache_dir->make_directory_with_parents();
        }

        refresh();
    }

    void refresh() {
        auto cache_file = Gio::File::create_for_path(cache_file_path);
        if (!cache_file->query_exists()) {
            cache_object = json{};
            return;
        }

        char* contents = nullptr;
        gsize length = 0;
        if (!cache_file->load_contents(contents, length)) {
            throw std::runtime_error("Error loading cache file");
        }

        cache_object = json::parse(contents);
        g_free(contents);
    }

    void save() {
        auto cache_file = Gio::File::create_for_path(cache_file_path);
        const auto cache_dump = cache_object.dump();
        std::string empty;
        cache_file->replace_contents(
            cache_dump.c_str(), cache_dump.size(), empty, empty, false, Gio::FILE_CREATE_PRIVATE);
    }

    json& operator[](std::string key) {
        return cache_object[key];
    }

    json::iterator find(std::string key) {
        return cache_object.find(key);
    }

    json::iterator begin() {
        return cache_object.begin();
    }

    json::iterator end() {
        return cache_object.end();
    }

private:
    json cache_object;
    std::string cache_file_path;
};
