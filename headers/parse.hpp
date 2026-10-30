#pragma once

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

namespace parser{
    using json = nlohmann::json;

    struct Config
    {
        int rows = 20;
        int cols = 10;
        int gravity_speed_ms = 800;
        int lock_delay_ms = 500;
        int das = 133;
        int arr = 10;
        int block_size = 32;
        bool show_ghost_piece = true;
    };

    Config loadConfig(const std::string &filepath);
}


