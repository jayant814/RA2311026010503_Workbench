#include "core/core.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>
int passed = 0;
void check(bool good, const char *name) {
    if (!good)
        throw std::runtime_error(name);
    ++passed;
    std::cout << "PASS " << name << '\n';
}
template <class F> void rejects(F f, const char *name) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, name);
}
int main() {
    try {
        double previous = 0;
        for (int n : {10, 20, 40, 80}) {
            tb::Rod r;
            r.n = n;
            auto s = tb::solve(r);
            check(s.t.front() == 100 && s.t.back() == 20, "Dirichlet endpoints");
            check(std::abs(s.midpoint_error - 125.0 / (n * n)) < 1e-8,
                  "midpoint interpolation error q*h*h/(8*k)");
            if (previous)
                check(std::abs(previous / s.midpoint_error - 4) < 1e-6, "second-order convergence");
            previous = s.midpoint_error;
        }
        tb::Rod r;
        r.q = 0;
        auto linear = tb::solve(r);
        check(std::abs(linear.t[10] - 60) < 1e-9, "zero-source linear solution");
        r.n = 2;
        r.q = 10000;
        check(std::abs(tb::solve(r).t[1] - 185) < 1e-9, "two-element assembly");
        r.k = 0;
        rejects([&] { tb::solve(r); }, "reject zero conductivity");
        r.k = 10;
        r.L = std::nan("");
        rejects([&] { tb::solve(r); }, "reject NaN");
        auto mesh = tb::parse_stl(tb::read_text("data/rod.stl"));
        check(mesh.size() == 12, "ASCII STL sample 12 triangles");
        rejects([] { tb::parse_stl(""); }, "reject empty STL");
        rejects([] { tb::parse_stl("solid x\nfacet normal 0 0 nan\n"); }, "reject non-finite STL");
        rejects([] { tb::parse_stl("solid x\nendsolid x\n"); }, "reject no facets");
        const std::string facet = "facet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 "
                                  "0\nvertex 0 1 0\nendloop\nendfacet\n";
        std::string largeStl = "solid boundary\n";
        for (int i = 0; i < 20000; ++i)
            largeStl += facet;
        check(tb::parse_stl(largeStl + "endsolid boundary\n").size() == 20000,
              "STL 20000-triangle boundary");
        rejects([&] { tb::parse_stl(largeStl + facet + "endsolid boundary\n"); },
                "STL triangle cap");
        rejects([&] { tb::parse_stl("solid x\n" + facet); }, "STL missing endsolid");
        rejects([&] { tb::parse_stl("solid x\n" + facet + "endsolid x\njunk"); },
                "STL trailing data");
        auto nodes = tb::parse_csv(tb::read_text("data/synthetic_temperature.csv"));
        check(nodes.size() == 84, "synthetic CSV sample 84 nodes");
        const std::string head = "node_id,x_m,y_m,z_m,temperature_C\n";
        rejects([&] { tb::parse_csv(head); }, "reject header-only CSV");
        rejects([&] { tb::parse_csv(head + "1,0,0,0,nan\n"); }, "reject non-finite CSV");
        rejects([&] { tb::parse_csv(head + "1,0,0,0,30\n1,1,0,0,40\n"); }, "reject duplicate IDs");
        rejects([&] { tb::parse_csv(head + "1,0,0,0,30,\n"); }, "reject extra column");
        rejects([&] { tb::parse_csv(head + "1,0,0,0,30junk\n"); }, "reject trailing numeric junk");
        std::string big = head;
        for (int i = 0; i < 50000; ++i)
            big += std::to_string(i) + ",0,0,0,20\n";
        check(tb::parse_csv(big).size() == 50000, "CSV 50000-node boundary");
        rejects([&] { tb::parse_csv(big + "50000,0,0,0,20\n"); }, "CSV node cap");
        rejects([] { tb::parse_csv(std::string(tb::max_file_bytes + 1, 'x')); }, "8 MiB cap");
        auto a = tb::generate_cases(30, 80), b = tb::generate_cases(30, 80);
        check(a[0].k == b[0].k && a[29].q == b[29].q, "seed repeatability");
        unsigned workers = std::min(4u, std::max(1u, std::thread::hardware_concurrency()));
        check(tb::run_batch(a, 1) == tb::run_batch(a, workers), "serial parallel exact agreement");
        std::atomic<bool> cancel{true};
        rejects([&] { tb::run_batch(a, workers, &cancel); }, "cancel discards partial batch");
        rejects([] { tb::generate_cases(20000, 20000); }, "batch memory cap");
        auto g = tb::gradient({}), ng = tb::numerical_gradient({});
        for (int i = 0; i < 4; ++i)
            check(std::abs(g[i] - ng[i]) < 1e-5, "central finite-difference gradient");
        tb::Network p;
        check(tb::loss(tb::update(p)) < tb::loss(p), "first update reduces loss");
        for (int i = 0; i < 101; ++i)
            p = tb::update(p);
        check(tb::loss(p) < 1e-16, "100 subsequent steps converge");
        for (auto &d : tb::discover()) {
            rejects([&] { d.telemetry(); }, "disconnected telemetry rejected");
            d.connect();
            check(!d.telemetry().empty(), "connected telemetry");
            d.fail_next = true;
            rejects([&] { d.telemetry(); }, "injected timeout");
            check(!d.telemetry().empty(), "timeout recovery");
            d.disconnect();
            check(!d.connected, "disconnect");
        }
        std::cout << "ALL " << passed << " CHECKS PASSED\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
