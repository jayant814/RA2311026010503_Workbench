#pragma once
#include <array>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>
namespace tb {
struct Rod {
    int n = 20;
    double L = 1, A = 1e-4, k = 10, q = 10000, left = 100, right = 20;
};
struct Solution {
    std::vector<double> x, t;
    double midpoint_error = 0;
};
void validate(const Rod &r);
double exact(const Rod &r, double x);
Solution solve(const Rod &r);
struct Vec3 {
    double x, y, z;
};
struct Triangle {
    std::array<Vec3, 3> v;
};
struct Node {
    long long id;
    Vec3 p;
    double temperature;
};
constexpr size_t max_file_bytes = 8 * 1024 * 1024;
std::string read_text(const std::filesystem::path &path);
std::vector<Triangle> parse_stl(const std::string &text);
std::vector<Node> parse_csv(const std::string &text);
void write_text(const std::filesystem::path &path, const std::string &data);
std::string thermal_csv(const Rod &r, const Solution &s);
std::vector<Rod> generate_cases(int count, int mesh, unsigned seed = 2027);
using Batch = std::vector<std::vector<double>>;
Batch run_batch(const std::vector<Rod> &cases, unsigned workers,
                std::atomic<bool> *cancel = nullptr);
struct Timing {
    unsigned workers;
    std::array<double, 5> seconds;
    double median, speedup, efficiency, max_difference;
};
std::vector<Timing> benchmark(int count, int mesh, std::atomic<bool> *cancel = nullptr,
                              std::atomic<int> *progress = nullptr);
std::string timing_csv(const std::vector<Timing> &rows, int count, int mesh);
struct Network {
    double w = .5, b = 0, v = 1, c = 0;
};
double loss(const Network &p);
std::array<double, 4> gradient(const Network &p);
std::array<double, 4> numerical_gradient(const Network &p);
Network update(const Network &p);
std::string training_csv();
struct Device {
    std::string id, name, capabilities;
    bool connected = false;
    bool fail_next = false;
    unsigned sequence = 0;
    void connect();
    void disconnect();
    std::string telemetry();
};
std::vector<Device> discover();
std::string system_info();
void export_evidence(const std::filesystem::path &folder, int count = 1000, int mesh = 4000);
} // namespace tb
