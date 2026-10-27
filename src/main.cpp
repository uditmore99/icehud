#include "telemetry_parser.hpp"
#include "hud_renderer.hpp"
#include "attitude_smoother.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

class SerialPort {
public:
    ~SerialPort() { closePort(); }

    bool openPort(const std::string& port_name) {
#ifdef _WIN32
        std::string full_name = "\\\\.\\" + port_name;
        h_serial_ = CreateFileA(full_name.c_str(), GENERIC_READ, 0, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h_serial_ == INVALID_HANDLE_VALUE) return false;

        DCB dcb = {0};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(h_serial_, &dcb)) return false;
        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.Parity   = NOPARITY;
        if (!SetCommState(h_serial_, &dcb)) return false;

        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout         = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier  = 0;
        timeouts.ReadTotalTimeoutConstant    = 0;
        SetCommTimeouts(h_serial_, &timeouts);
        return true;
#else
        fd_ = open(port_name.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) return false;
        struct termios tty {};
        if (tcgetattr(fd_, &tty) != 0) return false;
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
        tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
        tty.c_iflag = IGNPAR;
        tty.c_oflag = 0;
        tty.c_lflag = 0;
        tcsetattr(fd_, TCSANOW, &tty);
        return true;
#endif
    }

    int readBytes(uint8_t* buf, int max_len) {
#ifdef _WIN32
        if (h_serial_ == INVALID_HANDLE_VALUE) return 0;
        DWORD bytes_read = 0;
        if (ReadFile(h_serial_, buf, max_len, &bytes_read, nullptr)) {
            return static_cast<int>(bytes_read);
        }
        return 0;
#else
        if (fd_ < 0) return 0;
        int n = static_cast<int>(read(fd_, buf, max_len));
        return (n > 0) ? n : 0;
#endif
    }

    void closePort() {
#ifdef _WIN32
        if (h_serial_ != INVALID_HANDLE_VALUE) {
            CloseHandle(h_serial_);
            h_serial_ = INVALID_HANDLE_VALUE;
        }
#else
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
#endif
    }

private:
#ifdef _WIN32
    HANDLE h_serial_ = INVALID_HANDLE_VALUE;
#else
    int fd_ = -1;
#endif
};

int main(int argc, char* argv[]) {
    bool sim_mode = true;
    std::string port_name;

    if (argc >= 2 && std::string(argv[1]) != "--sim") {
        port_name = argv[1];
        sim_mode  = false;
    }

    SerialPort serial;
    if (!sim_mode) {
        if (!serial.openPort(port_name)) {
            std::printf("[WARN] Could not open %s, falling back to --sim mode.\n", port_name.c_str());
            sim_mode = true;
        } else {
            std::printf("[INFO] Connected to FPGA UART on %s @ 115200 8N1\n", port_name.c_str());
        }
    }

    HudRenderer hud(900, 650);
    if (!hud.init("Lattice iCE40UP5k FPGA - Aviation HUD Telemetry Frontend")) {
        return 1;
    }

    HudState state;
    AttitudeSmoother attitude_smoother;
    std::vector<uint8_t> rx_fifo;
    bool running = true;
    uint32_t start_ticks = SDL_GetTicks();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        const double t_sec = (SDL_GetTicks() - start_ticks) * 0.001;
        state.sim_heading_deg  = std::fmod(285.0 + 8.0 * std::sin(t_sec * 0.35) + 360.0, 360.0);
        state.sim_altitude_ft  = 12500.0 + 180.0 * std::sin(t_sec * 0.5);

        if (sim_mode) {
            // Generate synthetic raw IMU frame and pass through TelemetryParser
            const double pitch_rad = (12.0 * std::sin(t_sec * 1.1)) * (3.14159265 / 180.0);
            const double bank_rad  = (25.0 * std::cos(t_sec * 0.8)) * (3.14159265 / 180.0);
            const int16_t sim_ax = static_cast<int16_t>(-16384.0 * std::sin(pitch_rad));
            const int16_t sim_ay = static_cast<int16_t>( 16384.0 * std::cos(pitch_rad) * std::sin(bank_rad));
            const int16_t sim_az = static_cast<int16_t>( 16384.0 * std::cos(pitch_rad) * std::cos(bank_rad));

            uint8_t frame[9] = {
                0xAA, 0x55,
                static_cast<uint8_t>((sim_ax >> 8) & 0xFF), static_cast<uint8_t>(sim_ax & 0xFF),
                static_cast<uint8_t>((sim_ay >> 8) & 0xFF), static_cast<uint8_t>(sim_ay & 0xFF),
                static_cast<uint8_t>((sim_az >> 8) & 0xFF), static_cast<uint8_t>(sim_az & 0xFF),
                0
            };
            frame[8] = TelemetryParser::computeXorChecksum(&frame[2]);
            if (TelemetryParser::parseFrame(frame, 9, state)) {
                attitude_smoother.apply(state);
            }
        } else {
            uint8_t tmp[64];
            int n = serial.readBytes(tmp, sizeof(tmp));
            if (n > 0) {
                rx_fifo.insert(rx_fifo.end(), tmp, tmp + n);
            }
            while (rx_fifo.size() >= TelemetryParser::FRAME_SIZE) {
                if (rx_fifo[0] == 0xAA && rx_fifo[1] == 0x55) {
                    if (TelemetryParser::parseFrame(rx_fifo.data(), TelemetryParser::FRAME_SIZE, state)) {
                        attitude_smoother.apply(state);
                    }
                    rx_fifo.erase(rx_fifo.begin(), rx_fifo.begin() + TelemetryParser::FRAME_SIZE);
                } else {
                    rx_fifo.erase(rx_fifo.begin());
                }
            }
        }

        hud.render(state, sim_mode);
        SDL_Delay(16);
    }

    return 0;
}
