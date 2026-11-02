#include "core.hpp"
#include <stdexcept>
namespace tb {
void Device::connect() {
    connected = true;
    sequence = 0;
}
void Device::disconnect() {
    connected = false;
}
std::string Device::telemetry() {
    if (!connected)
        throw std::runtime_error("SIMULATED device disconnected. Click Connect first.");
    if (fail_next) {
        fail_next = false;
        throw std::runtime_error(
            "SIMULATED timeout after 250 ms budget. Link retained; click Read telemetry to retry.");
    }
    ++sequence;
    return "SIMULATED id=" + id + " seq=" + std::to_string(sequence) +
           (id == "mcu"    ? " temperature_C=26.5"
            : id == "fpga" ? " counter=1024 clock_MHz=100"
                           : " target_voltage_V=3.3 debug_link=idle");
}
std::vector<Device> discover() {
    return {
        {"mcu", "NUCLEO-F401RE (mock)", "USB virtual COM; temperature telemetry; STM32 control"},
        {"fpga", "Basys 3 (mock)", "USB UART; counter telemetry; programmable logic"},
        {"jtag", "J-Link (mock)",
         "USB debug probe; JTAG/SWD; target voltage status; NOT a compute board"}};
}
} // namespace tb
