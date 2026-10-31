#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

namespace parser
{
    using json = nlohmann::json;

    struct Config
    {
        int rows = 20;
        int cols = 10;
        int gravity_speed_ms = 500;
        int das = 133;
        int arr = 10;
        int sdf = 20;
    };

    Config loadConfig(const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "Warning: Could not open " << filepath << ". Using fallback defaults.\n";
            return Config{};
        }

        try
        {
            json j = json::parse(file);
            Config config;

            config.rows = j["grid"].value("rows", 20);
            config.cols = j["grid"].value("cols", 10);
            config.gravity_speed_ms = j["gameplay"].value("gravity_speed_ms", 800);
            config.sdf = j["gameplay"].value("sdf", 10);
            config.das = j["gameplay"].value("das", 133);
            config.arr = j["gameplay"].value("arr", 10);

            return config;
        }
        catch (const json::parse_error &e)
        {
            std::cerr << "JSON Parse error: " << e.what() << "\n";
            return Config{};
        }
    }
}
