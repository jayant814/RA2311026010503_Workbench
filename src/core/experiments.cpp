#include "core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif
namespace tb {
std::vector<Rod> generate_cases(int count, int mesh, unsigned seed) {
    if (count < 1 || count > 20000 || mesh < 2 || mesh > 20000 ||
        static_cast<long long>(count) * (mesh + 1) > 20000000)
        throw std::runtime_error(
            "Batch: 1..20000 cases, 2..20000 elements; at most 20 million stored nodal values.");
    std::mt19937 rng(seed);
    std::vector<Rod> cases;
    cases.reserve(count);
    // Explicit conversion avoids implementation-defined distribution differences.
    for (int i = 0; i < count; ++i) {
        Rod r;
        r.n = mesh;
        r.k = 5 + 20 * (rng() / 4294967295.0);
        r.q = 1000 + 19000 * (rng() / 4294967295.0);
        cases.push_back(r);
    }
    return cases;
}
Batch run_batch(const std::vector<Rod> &cases, unsigned workers, std::atomic<bool> *cancel) {
    if (workers < 1 || workers > std::max(1u, std::thread::hardware_concurrency()))
        throw std::runtime_error("Worker count exceeds available logical CPUs.");
    Batch out(cases.size());
    std::atomic<size_t> next{0};
    std::exception_ptr failure;
    std::mutex error_mutex;
    auto worker = [&] {
        try {
            for (;;) {
                if (cancel && cancel->load())
                    break;
                size_t i = next.fetch_add(1);
                if (i >= cases.size())
                    break;
                out[i] = solve(cases[i]).t;
            }
        } catch (...) {
            std::lock_guard<std::mutex> lock(error_mutex);
            if (!failure)
                failure = std::current_exception();
        }
    };
    if (workers == 1)
        worker();
    else {
        std::vector<std::thread> pool;
        try {
            for (unsigned i = 0; i < workers; ++i)
                pool.emplace_back(worker);
        } catch (...) {
            for (auto &t : pool)
                t.join();
            throw;
        }
        for (auto &t : pool)
            t.join();
    }
    if (failure)
        std::rethrow_exception(failure);
    if (cancel && cancel->load())
        throw std::runtime_error("Cancelled safely. Previous completed results retained.");
    return out;
}
std::vector<Timing> benchmark(int count, int mesh, std::atomic<bool> *cancel,
                              std::atomic<int> *progress) {
    auto cases = generate_cases(count, mesh);
    unsigned cap = std::max(1u, std::thread::hardware_concurrency());
    std::vector<unsigned> counts{1};
    if (cap >= 2)
        counts.push_back(2);
    if (cap >= 4)
        counts.push_back(4);
    else if (cap == 3)
        counts.push_back(3);
    Batch reference = run_batch(cases, 1, cancel);
    std::vector<Timing> rows;
    int completed = 0;
    for (unsigned workers : counts) {
        run_batch(cases, workers, cancel);
        Timing row{};
        row.workers = workers;
        for (int repeat = 0; repeat < 5; ++repeat) {
            auto start = std::chrono::steady_clock::now();
            auto values = run_batch(cases, workers, cancel);
            auto end = std::chrono::steady_clock::now();
            row.seconds[repeat] = std::chrono::duration<double>(end - start).count();
            for (size_t i = 0; i < values.size(); ++i)
                for (size_t j = 0; j < values[i].size(); ++j)
                    row.max_difference =
                        std::max(row.max_difference, std::abs(values[i][j] - reference[i][j]));
            if (progress)
                progress->store(++completed * 100 / (5 * static_cast<int>(counts.size())));
        }
        if (row.max_difference > 1e-8)
            throw std::runtime_error("Serial/parallel numerical agreement failed.");
        auto sorted = row.seconds;
        std::sort(sorted.begin(), sorted.end());
        row.median = sorted[2];
        rows.push_back(row);
    }
    for (auto &row : rows) {
        row.speedup = rows[0].median / row.median;
        row.efficiency = row.speedup / row.workers;
    }
    return rows;
}
std::string timing_csv(const std::vector<Timing> &rows, int count, int mesh) {
    std::ostringstream o;
    o << std::setprecision(17)
      << "workers,repeat,seconds,median_seconds,speedup,efficiency,max_difference_C,cases,elements,"
         "seed\n";
    for (const auto &r : rows)
        for (int i = 0; i < 5; ++i)
            o << r.workers << ',' << i + 1 << ',' << r.seconds[i] << ',' << r.median << ','
              << r.speedup << ',' << r.efficiency << ',' << r.max_difference << ',' << count << ','
              << mesh << ",2027\n";
    return o.str();
}
double loss(const Network &p) {
    double d = p.v * std::tanh(p.w + p.b) + p.c - 2;
    return .5 * d * d;
}
std::array<double, 4> gradient(const Network &p) {
    double h = std::tanh(p.w + p.b), d = p.v * h + p.c - 2, g = d * p.v * (1 - h * h);
    return {g, g, d * h, d};
}
std::array<double, 4> numerical_gradient(const Network &p) {
    std::array<double, 4> result;
    for (int i = 0; i < 4; ++i) {
        Network a = p, b = p;
        double *pa[] = {&a.w, &a.b, &a.v, &a.c};
        double *pb[] = {&b.w, &b.b, &b.v, &b.c};
        *pa[i] += 1e-6;
        *pb[i] -= 1e-6;
        result[i] = (loss(a) - loss(b)) / (2e-6);
    }
    return result;
}
Network update(const Network &p) {
    auto g = gradient(p);
    return {p.w - .1 * g[0], p.b - .1 * g[1], p.v - .1 * g[2], p.c - .1 * g[3]};
}
std::string training_csv() {
    Network p;
    std::ostringstream o;
    o << std::setprecision(17) << "step,loss,w,b,v,c\n";
    for (int i = 0; i <= 101; ++i) {
        o << i << ',' << loss(p) << ',' << p.w << ',' << p.b << ',' << p.v << ',' << p.c << '\n';
        p = update(p);
    }
    return o.str();
}
std::string system_info() {
    std::ostringstream o;
#ifdef _MSC_VER
    o << "Compiler: MSVC " << _MSC_VER << "\nC++: " << __cplusplus
#else
    o << "Compiler: " << __VERSION__ << "\nC++: " << __cplusplus
#endif
      << "\nLogical CPUs (thread API): " << std::thread::hardware_concurrency() << "\n";
#ifdef _WIN32
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    GlobalMemoryStatusEx(&m);
    o << "RAM bytes: " << m.ullTotalPhys << "\n";
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    std::vector<unsigned char> buf(length);
    unsigned cores = 0;
    if (GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buf.data()), &length)) {
        for (size_t i = 0; i < length;) {
            auto *p = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buf.data() + i);
            ++cores;
            i += p->Size;
        }
    }
    o << "Physical cores: " << cores << "\n";
    HKEY key;
    char cpu[256]{};
    DWORD bytes = sizeof(cpu);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
                      KEY_READ, &key) == ERROR_SUCCESS) {
        RegQueryValueExA(key, "ProcessorNameString", nullptr, nullptr,
                         reinterpret_cast<BYTE *>(cpu), &bytes);
        RegCloseKey(key);
    }
    o << "CPU: " << cpu << "\nPlatform: Windows\n";
#else
    std::ifstream cpu("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpu, line))
        if (line.find("model name") == 0) {
            o << line << '\n';
            break;
        }
    o << "Physical core topology: see environment.txt (lscpu)\nRAM bytes (host-visible): "
      << static_cast<long long>(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGESIZE)
      << "\nPlatform: Linux container; shared host, not Windows\n";
#endif
    o << "Bundled GNU Release flags: -O3 -DNDEBUG; other builds must record their actual "
         "flags.\nWarm-up: reference solve plus one "
         "untimed batch per worker count\nFive repetitions; steady_clock; excludes generation, "
         "verification, IO and UI. Includes allocation, thread startup and joining.\n";
    return o.str();
}
void export_evidence(const std::filesystem::path &folder, int count, int mesh) {
    std::filesystem::create_directories(folder);
    std::ostringstream convergence;
    convergence << std::setprecision(17) << "elements,h_m,max_midpoint_error_C,predicted_error_C\n";
    for (int n : {10, 20, 40, 80}) {
        Rod r;
        r.n = n;
        auto s = solve(r);
        write_text(folder / ("thermal_" + std::to_string(n) + ".csv"), thermal_csv(r, s));
        convergence << n << ',' << r.L / n << ',' << s.midpoint_error << ','
                    << r.q * r.L * r.L / (8 * r.k * n * n) << '\n';
    }
    write_text(folder / "convergence.csv", convergence.str());
    write_text(folder / "training.csv", training_csv());
    auto g = gradient({}), ng = numerical_gradient({});
    std::ostringstream gradients;
    gradients << std::setprecision(17) << "parameter,analytic,finite_difference,absolute_error\n";
    const char *names[] = {"w", "b", "v", "c"};
    for (int i = 0; i < 4; ++i)
        gradients << names[i] << ',' << g[i] << ',' << ng[i] << ',' << std::abs(g[i] - ng[i])
                  << '\n';
    write_text(folder / "gradients.csv", gradients.str());
    write_text(folder / "performance.csv", timing_csv(benchmark(count, mesh), count, mesh));
    write_text(folder / "system.txt", system_info());
    auto devices = discover();
    std::ostringstream log;
    for (auto &d : devices) {
        log << d.name << ": " << d.capabilities << '\n';
        d.connect();
        log << d.telemetry() << '\n';
        d.fail_next = true;
        try {
            d.telemetry();
        } catch (const std::exception &e) {
            log << e.what() << '\n';
        }
        log << d.telemetry() << '\n';
        d.disconnect();
    }
    write_text(folder / "hardware.log", log.str());
}
} // namespace tb
