/**
 * @file analizador.cpp
 * @brief C++20 plugin for processing IoT-like sensor readings inside the sandbox.
 *
 * This module implements the function `analizar_datos()`, loaded dynamically
 * via `dlopen()` by the C sandbox controller. The plugin processes an input file
 * containing one sensor reading per line, in the format:
 *
 *      <timestamp_ms> <sensor_value>
 *
 * It computes:
 *  - minimum value
 *  - maximum value
 *  - average value
 *  - number of valid readings
 *
 * It validates input format strictly, reports malformed lines, and returns
 * specific error codes. Output is written to stdout so the sandbox can capture it.
 *
 * @note Compile this file as a shared object (.so) using:
 *       g++ -std=gnu++20 -fPIC -shared analizador.cpp -o libanalizador.so
 *
 * @note No external libraries are required.
 */

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

/**
 * @brief Status codes returned by the plugin.
 *
 * These map directly to the integer values required by the external
 * C-compatible ABI used by the sandbox controller.
 */
enum class PluginStatus : int
{
    Success = 0,           ///< Processing completed successfully.
    FileNotAccessible = 1, ///< Input file missing, unreadable, or invalid.
    InvalidFormat = 2,     ///< A line is malformed or cannot be parsed.
    NoValidReadings = 3    ///< File contained no valid sensor readings.
};

/**
 * @brief Attempts to parse a number of type T from a string_view using std::from_chars.
 *
 * This function avoids locale overhead and is significantly faster and more
 * reliable than using stringstreams for numeric parsing.
 *
 * @tparam T Numeric type to parse (int, long long, double, etc.)
 * @param text The string to parse.
 * @return std::optional<T> Contains the parsed value on success, or std::nullopt on failure.
 */
template<typename T>
static std::optional<T> parse_number(std::string_view text)
{
    T value {};
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec != std::errc {} || ptr != end)
    {
        return std::nullopt; // Malformed or trailing garbage
    }

    return value;
}

/**
 * @brief Main entry point for the plugin. Processes a file of IoT sensor readings.
 *
 * @param input_path Path to the input file inside the sandbox environment.
 * @return int Status code (see PluginStatus):
 *      - 0: success
 *      - 1: file not accessible
 *      - 2: malformed line / invalid format
 *      - 3: no valid readings
 */
extern "C" int analizar_datos(const char* input_path)
{
    if (input_path == nullptr)
    {
        std::fprintf(stderr, "Error: null input file path provided\n");
        return static_cast<int>(PluginStatus::FileNotAccessible);
    }

    std::filesystem::path path(input_path);

    // Validate existence and type
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    {
        std::fprintf(stderr, "Error: invalid file: %s\n", input_path);
        return static_cast<int>(PluginStatus::FileNotAccessible);
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::fprintf(stderr, "Error: could not open %s (%s)\n", input_path, std::strerror(errno));
        return static_cast<int>(PluginStatus::FileNotAccessible);
    }

    double min_value = std::numeric_limits<double>::infinity();
    double max_value = -std::numeric_limits<double>::infinity();
    double sum_values = 0.0;
    int valid_count = 0;

    std::string line;

    // Process line by line
    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::string_view line_view(line);

        // Split timestamp and value
        std::size_t space_pos = line_view.find(' ');
        if (space_pos == std::string_view::npos)
        {
            std::fprintf(stderr, "Error: missing space separator in line: %s\n", line.c_str());
            return static_cast<int>(PluginStatus::InvalidFormat);
        }

        std::string_view timestamp_str = line_view.substr(0, space_pos);
        std::string_view value_str = line_view.substr(space_pos + 1);

        // Parse timestamp
        auto timestamp = parse_number<long long>(timestamp_str);
        if (!timestamp.has_value())
        {
            std::fprintf(stderr, "Error: invalid timestamp in line: %s\n", line.c_str());
            return static_cast<int>(PluginStatus::InvalidFormat);
        }

        // Parse sensor value
        auto sensor_value_parsed = parse_number<double>(value_str);
        if (!sensor_value_parsed.has_value())
        {
            std::fprintf(stderr, "Error: invalid sensor value in line: %s\n", line.c_str());
            return static_cast<int>(PluginStatus::InvalidFormat);
        }

        double sensor_value = *sensor_value_parsed;

        // Update metrics
        min_value = std::min(sensor_value, min_value);
        max_value = std::max(sensor_value, max_value);

        sum_values += sensor_value;
        valid_count++;
    }

    if (valid_count == 0)
    {
        std::fprintf(stderr, "Error: file contained no valid sensor readings\n");
        return static_cast<int>(PluginStatus::NoValidReadings);
    }

    double average = sum_values / static_cast<double>(valid_count);

    // Output to stdout (sandbox will capture this)
    std::printf("readings=%d min=%.2f max=%.2f avg=%.2f\n", valid_count, min_value, max_value, average);

    return static_cast<int>(PluginStatus::Success);
}
