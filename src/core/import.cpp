#include "core.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace tb {
namespace {
double number(const std::string &s) {
    size_t used = 0;
    double v;
    try {
        v = std::stod(s, &used);
    } catch (...) {
        throw std::runtime_error("Invalid number: " + s.substr(0, 40));
    }
    if (used != s.size() || !std::isfinite(v) || std::abs(v) > 1e12)
        throw std::runtime_error("Numbers must be finite, fully numeric and within +/-1e12.");
    return v;
}
void expect(std::istream &s, const std::string &token) {
    std::string got;
    if (!(s >> got) || got != token)
        throw std::runtime_error("STL expected '" + token + "'. Check ASCII STL facet syntax.");
}
Vec3 vec(std::istream &s) {
    std::string a, b, c;
    if (!(s >> a >> b >> c))
        throw std::runtime_error("STL vertex requires 3 coordinates.");
    return {number(a), number(b), number(c)};
}
} // namespace
std::string read_text(const std::filesystem::path &path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        throw std::runtime_error("Cannot open file. Check the path and read permissions.");
    auto size = f.tellg();
    if (size <= 0)
        throw std::runtime_error("The file is empty.");
    if (size > static_cast<std::streamoff>(max_file_bytes))
        throw std::runtime_error("File exceeds the 8 MiB prototype limit.");
    std::string s(static_cast<size_t>(size), '\0');
    f.seekg(0);
    if (!f.read(s.data(), size))
        throw std::runtime_error("File read failed.");
    return s;
}
void write_text(const std::filesystem::path &path, const std::string &data) {
    // Commit a completed buffer via a sibling temporary file; never expose a partial CSV.
    auto temp = path;
    temp += ".tmp";
    {
        std::ofstream f(temp, std::ios::binary);
        if (!f)
            throw std::runtime_error("Cannot write output. Choose a writable folder.");
        f << data;
        f.flush();
        if (!f)
            throw std::runtime_error("Output write failed; check free disk space.");
    }
    std::error_code ec;
    std::filesystem::rename(temp, path, ec);
    if (ec) { // Windows rename cannot replace an existing destination: preserve it instead.
        std::filesystem::remove(temp);
        throw std::runtime_error("Output exists or cannot be renamed. Choose a new filename.");
    }
}
std::vector<Triangle> parse_stl(const std::string &text) {
    if (text.empty())
        throw std::runtime_error("Empty STL file.");
    if (text.size() > max_file_bytes)
        throw std::runtime_error("STL exceeds 8 MiB.");
    std::istringstream s(text);
    expect(s, "solid");
    std::string line, word;
    std::getline(s, line);
    std::vector<Triangle> out;
    bool ended = false;
    while (s >> word) {
        if (word == "endsolid") {
            std::getline(s, line);
            ended = true;
            break;
        }
        if (word != "facet")
            throw std::runtime_error("Expected facet or endsolid; binary STL is unsupported.");
        expect(s, "normal");
        vec(s);
        expect(s, "outer");
        expect(s, "loop");
        Triangle t;
        for (auto &v : t.v) {
            expect(s, "vertex");
            v = vec(s);
        }
        expect(s, "endloop");
        expect(s, "endfacet");
        Vec3 a{t.v[1].x - t.v[0].x, t.v[1].y - t.v[0].y, t.v[1].z - t.v[0].z},
            b{t.v[2].x - t.v[0].x, t.v[2].y - t.v[0].y, t.v[2].z - t.v[0].z};
        double cx = a.y * b.z - a.z * b.y, cy = a.z * b.x - a.x * b.z, cz = a.x * b.y - a.y * b.x;
        if (cx * cx + cy * cy + cz * cz == 0)
            throw std::runtime_error("Degenerate zero-area STL triangle.");
        out.push_back(t);
        if (out.size() > 20000)
            throw std::runtime_error("Maximum 20000 triangles supported.");
    }
    if (!ended || out.empty())
        throw std::runtime_error("STL needs at least one triangle and endsolid.");
    if (s >> word)
        throw std::runtime_error("Unexpected data after endsolid.");
    return out;
}
std::vector<Node> parse_csv(const std::string &text) {
    if (text.size() > max_file_bytes)
        throw std::runtime_error("CSV exceeds 8 MiB.");
    std::istringstream s(text);
    std::string line;
    std::getline(s, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line != "node_id,x_m,y_m,z_m,temperature_C")
        throw std::runtime_error(
            "CSV header must be node_id,x_m,y_m,z_m,temperature_C (UTF-8, no BOM).");
    std::vector<Node> out;
    std::unordered_set<long long> ids;
    int row = 1;
    while (std::getline(s, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            throw std::runtime_error("Empty CSV row " + std::to_string(row));
        try {
            std::array<std::string, 5> cols;
            std::istringstream r(line);
            for (auto &col : cols)
                if (!std::getline(r, col, ','))
                    throw std::runtime_error("Expected five columns.");
            std::string extra;
            if (std::getline(r, extra, ',') || line.back() == ',')
                throw std::runtime_error("Expected exactly five columns.");
            size_t used;
            long long id = std::stoll(cols[0], &used);
            if (used != cols[0].size() || id < 0 || !ids.insert(id).second)
                throw std::runtime_error("Node IDs must be unique nonnegative integers.");
            out.push_back(
                {id, {number(cols[1]), number(cols[2]), number(cols[3])}, number(cols[4])});
            if (out.size() > 50000)
                throw std::runtime_error("Maximum 50000 nodes supported.");
        } catch (const std::exception &e) {
            throw std::runtime_error("CSV row " + std::to_string(row) + ": " + e.what());
        }
    }
    if (out.empty())
        throw std::runtime_error("CSV contains a header but no nodes.");
    return out;
}
} // namespace tb
