/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-05-18 15:26:15
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:23:00
 * @Description: 
 */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

namespace {

constexpr std::uint32_t HEADCODE = 0xFA55FA55;
constexpr std::uint32_t ENDCODE = 0x55FA55FA;

constexpr std::uint16_t SIG_PRO_ID = 0xBB02;
constexpr std::uint16_t DATA_PRO_ID = 0xBB03;
constexpr std::uint16_t DISP_CTRL_ID = 0xBB04;

constexpr std::uint16_t DETECTION_MSG_ID = 0xDD01;
constexpr std::uint16_t TRACK_MSG_ID = 0xEE01;
constexpr std::uint16_t TBD_TRACK_MSG_ID = 0xEE02;
constexpr std::uint16_t CO_TRACK_MSG_ID = 0xEE03;

constexpr std::uint16_t DEFAULT_DET_PORT = 8003;
constexpr std::uint16_t DEFAULT_TRACK_PORT = 8006;
constexpr std::uint16_t DEFAULT_TBD_PORT = 8010;
constexpr std::uint16_t DEFAULT_CO_PORT = 8020;
constexpr std::uint16_t DEFAULT_DET_SRC_PORT = 6003;
constexpr std::uint16_t DEFAULT_TRACK_SRC_PORT = 6006;
constexpr std::uint16_t DEFAULT_TBD_SRC_PORT = 6010;
constexpr std::uint16_t DEFAULT_CO_SRC_PORT = 6020;

constexpr float kPi = 3.14159265358979f;

#pragma pack(push, 1)

struct ProtocolFrame {
    std::uint32_t head;
    std::uint16_t srcID;
    std::uint16_t destID;
    std::uint32_t commCount;
    std::uint16_t dataLen;
};

struct ProtocolEnd {
    std::uint8_t checkCode;
    std::uint32_t end;
};

struct TrackResult {
    std::uint16_t mesID;
    std::uint16_t trackNum;
};

struct detInfo {
    float dis;
    float vel;
    float azi;
    float ele;
    float altitute;
    float amp;
    float CFARSNR;
    float statSNR;
    float aziBeam;
    float eleBeam;
    std::uint32_t disChannel;
    std::uint32_t dopChannel;
    std::uint32_t reserve;
    std::uint32_t reserve1;
};

struct trackInfo {
    std::uint16_t batch;
    std::uint16_t CPIID;
    std::uint32_t UTCtime;
    std::uint32_t nsecond;
    std::uint8_t statMethod;
    float amp;
    float SNR;
    float dis;
    float azi;
    float ele;
    float altitute;
    float vel;
    float spaceVel;
    float accelerate;
    std::uint32_t targetRecResult;
    std::uint32_t reserve1;
    std::uint32_t reserve2;
};

#pragma pack(pop)

struct Options {
    std::string targetIp = "127.0.0.1";
    std::uint16_t detectionPort = DEFAULT_DET_PORT;
    std::uint16_t trackPort = DEFAULT_TRACK_PORT;
    std::uint16_t tbdPort = DEFAULT_TBD_PORT;
    std::uint16_t cooperativePort = DEFAULT_CO_PORT;
    std::uint16_t detectionSrcPort = DEFAULT_DET_SRC_PORT;
    std::uint16_t trackSrcPort = DEFAULT_TRACK_SRC_PORT;
    std::uint16_t tbdSrcPort = DEFAULT_TBD_SRC_PORT;
    std::uint16_t cooperativeSrcPort = DEFAULT_CO_SRC_PORT;
    int fps = 20;
    int durationSeconds = 0;
    int detectionsPerFrame = 300;
    int trackBatches = 150;
    int tbdBatches = 0;
    int cooperativeBatches = 0;
    bool sendDetections = true;
    bool sendTracks = true;
    bool sendTbd = false;
    bool sendCooperative = false;
    float minRangeMeters = 50.0f;
    float maxRangeMeters = 5000.0f;
};

struct BatchState {
    std::uint16_t batchId;
    float angleDeg;
    float radiusMeters;
    float angleVelocityDeg;
    float radiusVelocityMeters;
    float elevationDeg;
    float speedMetersPerSecond;
    std::uint32_t targetRecResult;
};

void printUsage()
{
    std::cout
        << "windows_udp_stress_sender.exe [options]\n"
        << "  --target-ip <ip>           Target display host IP, default 127.0.0.1\n"
        << "  --fps <n>                  Send rate, default 20\n"
        << "  --duration <sec>           0 means run until Ctrl+C\n"
        << "  --det-per-frame <n>        Detection count per frame, default 300\n"
        << "  --track-batches <n>        Normal track batches per frame, default 150\n"
        << "  --tbd-batches <n>          TBD track batches per frame, default 0\n"
        << "  --co-batches <n>           Cooperative track batches per frame, default 0\n"
        << "  --det-port <port>          Detection UDP port, default 8003\n"
        << "  --track-port <port>        Normal track UDP port, default 8006\n"
        << "  --tbd-port <port>          TBD track UDP port, default 8010\n"
        << "  --co-port <port>           Cooperative track UDP port, default 8020\n"
        << "  --det-src-port <port>      Detection source UDP port, default 6003\n"
        << "  --track-src-port <port>    Normal track source UDP port, default 6006\n"
        << "  --tbd-src-port <port>      TBD track source UDP port, default 6010\n"
        << "  --co-src-port <port>       Cooperative source UDP port, default 6020\n"
        << "  --min-range <m>            Minimum range meters, default 50\n"
        << "  --max-range <m>            Maximum range meters, default 5000\n"
        << "  --det-only                 Send detections only\n"
        << "  --track-only               Send normal tracks only\n"
        << "  --all                      Send detections + normal/tbd/cooperative according to batch counts\n";
}

bool readIntArg(char** argv, int argc, int& index, int& value)
{
    if (index + 1 >= argc) {
        return false;
    }
    value = std::atoi(argv[++index]);
    return true;
}

bool readU16Arg(char** argv, int argc, int& index, std::uint16_t& value)
{
    int temp = 0;
    if (!readIntArg(argv, argc, index, temp)) {
        return false;
    }
    value = static_cast<std::uint16_t>(temp);
    return true;
}

bool readFloatArg(char** argv, int argc, int& index, float& value)
{
    if (index + 1 >= argc) {
        return false;
    }
    value = static_cast<float>(std::atof(argv[++index]));
    return true;
}

bool parseArgs(int argc, char** argv, Options& options)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--target-ip") {
            if (i + 1 >= argc) return false;
            options.targetIp = argv[++i];
        } else if (arg == "--fps") {
            if (!readIntArg(argv, argc, i, options.fps)) return false;
        } else if (arg == "--duration") {
            if (!readIntArg(argv, argc, i, options.durationSeconds)) return false;
        } else if (arg == "--det-per-frame") {
            if (!readIntArg(argv, argc, i, options.detectionsPerFrame)) return false;
        } else if (arg == "--track-batches") {
            if (!readIntArg(argv, argc, i, options.trackBatches)) return false;
        } else if (arg == "--tbd-batches") {
            if (!readIntArg(argv, argc, i, options.tbdBatches)) return false;
            options.sendTbd = options.tbdBatches > 0;
        } else if (arg == "--co-batches") {
            if (!readIntArg(argv, argc, i, options.cooperativeBatches)) return false;
            options.sendCooperative = options.cooperativeBatches > 0;
        } else if (arg == "--det-port") {
            if (!readU16Arg(argv, argc, i, options.detectionPort)) return false;
        } else if (arg == "--track-port") {
            if (!readU16Arg(argv, argc, i, options.trackPort)) return false;
        } else if (arg == "--tbd-port") {
            if (!readU16Arg(argv, argc, i, options.tbdPort)) return false;
        } else if (arg == "--co-port") {
            if (!readU16Arg(argv, argc, i, options.cooperativePort)) return false;
        } else if (arg == "--det-src-port") {
            if (!readU16Arg(argv, argc, i, options.detectionSrcPort)) return false;
        } else if (arg == "--track-src-port") {
            if (!readU16Arg(argv, argc, i, options.trackSrcPort)) return false;
        } else if (arg == "--tbd-src-port") {
            if (!readU16Arg(argv, argc, i, options.tbdSrcPort)) return false;
        } else if (arg == "--co-src-port") {
            if (!readU16Arg(argv, argc, i, options.cooperativeSrcPort)) return false;
        } else if (arg == "--min-range") {
            if (!readFloatArg(argv, argc, i, options.minRangeMeters)) return false;
        } else if (arg == "--max-range") {
            if (!readFloatArg(argv, argc, i, options.maxRangeMeters)) return false;
        } else if (arg == "--det-only") {
            options.sendDetections = true;
            options.sendTracks = false;
            options.sendTbd = false;
            options.sendCooperative = false;
        } else if (arg == "--track-only") {
            options.sendDetections = false;
            options.sendTracks = true;
            options.sendTbd = false;
            options.sendCooperative = false;
        } else if (arg == "--all") {
            options.sendDetections = true;
            options.sendTracks = true;
            options.sendTbd = options.tbdBatches > 0;
            options.sendCooperative = options.cooperativeBatches > 0;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }

    if (options.fps < 1) options.fps = 1;
    if (options.detectionsPerFrame < 0) options.detectionsPerFrame = 0;
    if (options.trackBatches < 0) options.trackBatches = 0;
    if (options.tbdBatches < 0) options.tbdBatches = 0;
    if (options.cooperativeBatches < 0) options.cooperativeBatches = 0;
    if (options.minRangeMeters < 1.0f) options.minRangeMeters = 1.0f;
    if (options.maxRangeMeters <= options.minRangeMeters) {
        options.maxRangeMeters = options.minRangeMeters + 1000.0f;
    }

    return true;
}

std::uint8_t calculateXor(const std::uint8_t* data, std::size_t length)
{
    std::uint8_t value = 0;
    for (std::size_t i = 0; i < length; ++i) {
        value ^= data[i];
    }
    return value;
}

template <typename T>
void appendStruct(std::vector<std::uint8_t>& buffer, const T& value)
{
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
}

sockaddr_in makeAddress(const std::string& ip, std::uint16_t port)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    return addr;
}

std::vector<BatchState> makeBatchStates(int count, float minRangeMeters, float maxRangeMeters, std::uint16_t startBatchId)
{
    std::mt19937 rng(static_cast<std::uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> radiusDist(minRangeMeters, maxRangeMeters);
    std::uniform_real_distribution<float> angleVelocityDist(0.1f, 1.2f);
    std::uniform_real_distribution<float> radiusVelocityDist(-3.0f, 3.0f);
    std::uniform_real_distribution<float> elevationDist(-8.0f, 25.0f);
    std::uniform_real_distribution<float> speedDist(10.0f, 80.0f);

    std::vector<BatchState> states;
    states.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        BatchState state{};
        state.batchId = static_cast<std::uint16_t>(startBatchId + i);
        state.angleDeg = angleDist(rng);
        state.radiusMeters = radiusDist(rng);
        state.angleVelocityDeg = angleVelocityDist(rng);
        state.radiusVelocityMeters = radiusVelocityDist(rng);
        state.elevationDeg = elevationDist(rng);
        state.speedMetersPerSecond = speedDist(rng);
        state.targetRecResult = (i % 3 == 0) ? 1u : 0u;
        states.push_back(state);
    }
    return states;
}

void advanceBatchStates(std::vector<BatchState>& states, float minRangeMeters, float maxRangeMeters)
{
    for (auto& state : states) {
        state.angleDeg += state.angleVelocityDeg;
        if (state.angleDeg >= 360.0f) {
            state.angleDeg -= 360.0f;
        }

        state.radiusMeters += state.radiusVelocityMeters;
        if (state.radiusMeters < minRangeMeters || state.radiusMeters > maxRangeMeters) {
            state.radiusVelocityMeters = -state.radiusVelocityMeters;
            state.radiusMeters = std::clamp(state.radiusMeters, minRangeMeters, maxRangeMeters);
        }
    }
}

std::vector<std::uint8_t> buildDetectionPacket(int detectionCount, std::uint32_t commCount, float minRangeMeters, float maxRangeMeters)
{
    std::mt19937 rng(static_cast<std::uint32_t>(commCount * 2654435761u));
    std::uniform_real_distribution<float> rangeDist(minRangeMeters, maxRangeMeters);
    std::uniform_real_distribution<float> azimuthDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> elevationDist(-10.0f, 30.0f);
    std::uniform_real_distribution<float> speedDist(-30.0f, 30.0f);
    std::uniform_real_distribution<float> ampDist(5.0f, 45.0f);
    std::uniform_real_distribution<float> snrDist(10.0f, 35.0f);

    const std::uint16_t payloadSize = static_cast<std::uint16_t>(
        sizeof(std::uint16_t) + 512 + sizeof(std::uint8_t) + sizeof(std::uint16_t)
        + detectionCount * static_cast<int>(sizeof(detInfo)) + sizeof(ProtocolEnd));

    std::vector<std::uint8_t> packet;
    packet.reserve(sizeof(ProtocolFrame) + payloadSize);

    ProtocolFrame frame{};
    frame.head = HEADCODE;
    frame.srcID = SIG_PRO_ID;
    frame.destID = DISP_CTRL_ID;
    frame.commCount = commCount;
    frame.dataLen = payloadSize;
    appendStruct(packet, frame);

    const std::uint16_t mesId = DETECTION_MSG_ID;
    appendStruct(packet, mesId);

    packet.insert(packet.end(), 512, 0);

    const std::uint8_t radarId = 1;
    appendStruct(packet, radarId);
    const std::uint16_t detNum = static_cast<std::uint16_t>(detectionCount);
    appendStruct(packet, detNum);

    for (int i = 0; i < detectionCount; ++i) {
        detInfo info{};
        info.dis = rangeDist(rng);
        info.vel = speedDist(rng);
        info.azi = azimuthDist(rng);
        info.ele = elevationDist(rng);
        info.altitute = std::max(0.0f, info.dis * std::sin(info.ele * kPi / 180.0f));
        info.amp = ampDist(rng);
        info.CFARSNR = snrDist(rng);
        info.statSNR = info.CFARSNR - 2.0f;
        info.aziBeam = info.azi;
        info.eleBeam = info.ele;
        info.disChannel = static_cast<std::uint32_t>(i);
        info.dopChannel = static_cast<std::uint32_t>(i % 64);
        appendStruct(packet, info);
    }

    ProtocolEnd end{};
    end.end = ENDCODE;
    end.checkCode = calculateXor(packet.data(), packet.size());
    appendStruct(packet, end);
    return packet;
}

std::vector<std::uint8_t> buildTrackPacket(
    const std::vector<BatchState>& states,
    std::uint32_t commCount,
    std::uint16_t srcId,
    std::uint16_t mesId)
{
    const std::uint16_t payloadSize = static_cast<std::uint16_t>(
        sizeof(TrackResult) + states.size() * sizeof(trackInfo) + sizeof(ProtocolEnd));

    std::vector<std::uint8_t> packet;
    packet.reserve(sizeof(ProtocolFrame) + payloadSize);

    ProtocolFrame frame{};
    frame.head = HEADCODE;
    frame.srcID = srcId;
    frame.destID = DISP_CTRL_ID;
    frame.commCount = commCount;
    frame.dataLen = payloadSize;
    appendStruct(packet, frame);

    TrackResult result{};
    result.mesID = mesId;
    result.trackNum = static_cast<std::uint16_t>(states.size());
    appendStruct(packet, result);

    const auto now = std::chrono::system_clock::now();
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() % 1000000000ll;

    for (std::size_t i = 0; i < states.size(); ++i) {
        const BatchState& state = states[i];
        trackInfo info{};
        info.batch = state.batchId;
        info.CPIID = static_cast<std::uint16_t>(commCount & 0xFFFFu);
        info.UTCtime = static_cast<std::uint32_t>(secs);
        info.nsecond = static_cast<std::uint32_t>(nanos);
        info.statMethod = 0;
        info.amp = 18.0f + static_cast<float>(i % 7);
        info.SNR = 22.0f + static_cast<float>(i % 11);
        info.dis = state.radiusMeters;
        info.azi = state.angleDeg;
        info.ele = state.elevationDeg;
        info.altitute = std::max(0.0f, state.radiusMeters * std::sin(state.elevationDeg * kPi / 180.0f));
        info.vel = state.speedMetersPerSecond;
        info.spaceVel = state.speedMetersPerSecond;
        info.accelerate = 0.5f;
        info.targetRecResult = state.targetRecResult;
        appendStruct(packet, info);
    }

    ProtocolEnd end{};
    end.end = ENDCODE;
    end.checkCode = calculateXor(packet.data(), packet.size());
    appendStruct(packet, end);
    return packet;
}

bool sendPacket(SOCKET sock, const sockaddr_in& addr, const std::vector<std::uint8_t>& packet)
{
    const int sent = sendto(sock,
                            reinterpret_cast<const char*>(packet.data()),
                            static_cast<int>(packet.size()),
                            0,
                            reinterpret_cast<const sockaddr*>(&addr),
                            sizeof(addr));
    return sent == static_cast<int>(packet.size());
}

bool bindSocketToPort(SOCKET sock, std::uint16_t srcPort)
{
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(srcPort);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    return bind(sock, reinterpret_cast<const sockaddr*>(&localAddr), sizeof(localAddr)) == 0;
}

} // namespace

int main(int argc, char** argv)
{
    Options options;
    if (!parseArgs(argc, argv, options)) {
        return 1;
    }

    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET detSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET trackSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET tbdSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET coSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (detSock == INVALID_SOCKET || trackSock == INVALID_SOCKET ||
        tbdSock == INVALID_SOCKET || coSock == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        if (detSock != INVALID_SOCKET) closesocket(detSock);
        if (trackSock != INVALID_SOCKET) closesocket(trackSock);
        if (tbdSock != INVALID_SOCKET) closesocket(tbdSock);
        if (coSock != INVALID_SOCKET) closesocket(coSock);
        WSACleanup();
        return 1;
    }

    if (!bindSocketToPort(detSock, options.detectionSrcPort)) {
        std::cerr << "Failed to bind detection source port " << options.detectionSrcPort << "\n";
        closesocket(detSock);
        closesocket(trackSock);
        closesocket(tbdSock);
        closesocket(coSock);
        WSACleanup();
        return 1;
    }
    if (!bindSocketToPort(trackSock, options.trackSrcPort)) {
        std::cerr << "Failed to bind normal track source port " << options.trackSrcPort << "\n";
        closesocket(detSock);
        closesocket(trackSock);
        closesocket(tbdSock);
        closesocket(coSock);
        WSACleanup();
        return 1;
    }
    if (!bindSocketToPort(tbdSock, options.tbdSrcPort)) {
        std::cerr << "Failed to bind TBD source port " << options.tbdSrcPort << "\n";
        closesocket(detSock);
        closesocket(trackSock);
        closesocket(tbdSock);
        closesocket(coSock);
        WSACleanup();
        return 1;
    }
    if (!bindSocketToPort(coSock, options.cooperativeSrcPort)) {
        std::cerr << "Failed to bind cooperative source port " << options.cooperativeSrcPort << "\n";
        closesocket(detSock);
        closesocket(trackSock);
        closesocket(tbdSock);
        closesocket(coSock);
        WSACleanup();
        return 1;
    }

    const sockaddr_in detAddr = makeAddress(options.targetIp, options.detectionPort);
    const sockaddr_in trackAddr = makeAddress(options.targetIp, options.trackPort);
    const sockaddr_in tbdAddr = makeAddress(options.targetIp, options.tbdPort);
    const sockaddr_in coAddr = makeAddress(options.targetIp, options.cooperativePort);

    auto normalStates = makeBatchStates(options.trackBatches, options.minRangeMeters, options.maxRangeMeters, 1);
    auto tbdStates = makeBatchStates(options.tbdBatches, options.minRangeMeters, options.maxRangeMeters, 2001);
    auto cooperativeStates = makeBatchStates(options.cooperativeBatches, options.minRangeMeters, options.maxRangeMeters, 4001);

    const auto frameInterval = std::chrono::milliseconds(std::max(1, 1000 / options.fps));
    const auto start = std::chrono::steady_clock::now();

    std::uint32_t detComm = 1;
    std::uint32_t trackComm = 1;
    std::uint64_t sentDetections = 0;
    std::uint64_t sentTrackPoints = 0;
    std::uint64_t frameCount = 0;

    std::cout << "Starting UDP stress sender\n"
              << "  target ip: " << options.targetIp << "\n"
              << "  detection port: " << options.detectionPort << "\n"
              << "  track port: " << options.trackPort << "\n"
              << "  tbd port: " << options.tbdPort << "\n"
              << "  cooperative port: " << options.cooperativePort << "\n"
              << "  detection src port: " << options.detectionSrcPort << "\n"
              << "  track src port: " << options.trackSrcPort << "\n"
              << "  tbd src port: " << options.tbdSrcPort << "\n"
              << "  cooperative src port: " << options.cooperativeSrcPort << "\n"
              << "  fps: " << options.fps << "\n"
              << "  detections/frame: " << options.detectionsPerFrame << "\n"
              << "  normal batches/frame: " << options.trackBatches << "\n"
              << "  tbd batches/frame: " << options.tbdBatches << "\n"
              << "  cooperative batches/frame: " << options.cooperativeBatches << "\n";

    while (true) {
        if (options.durationSeconds > 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start);
            if (elapsed.count() >= options.durationSeconds) {
                break;
            }
        }

        const auto tickStart = std::chrono::steady_clock::now();

        if (options.sendDetections && options.detectionsPerFrame > 0) {
            const auto packet = buildDetectionPacket(options.detectionsPerFrame, detComm++, options.minRangeMeters, options.maxRangeMeters);
            sendPacket(detSock, detAddr, packet);
            sentDetections += static_cast<std::uint64_t>(options.detectionsPerFrame);
        }

        if (options.sendTracks && !normalStates.empty()) {
            advanceBatchStates(normalStates, options.minRangeMeters, options.maxRangeMeters);
            const auto packet = buildTrackPacket(normalStates, trackComm++, DATA_PRO_ID, TRACK_MSG_ID);
            sendPacket(trackSock, trackAddr, packet);
            sentTrackPoints += static_cast<std::uint64_t>(normalStates.size());
        }

        if (options.sendTbd && !tbdStates.empty()) {
            advanceBatchStates(tbdStates, options.minRangeMeters, options.maxRangeMeters);
            const auto packet = buildTrackPacket(tbdStates, trackComm++, DATA_PRO_ID, TBD_TRACK_MSG_ID);
            sendPacket(tbdSock, tbdAddr, packet);
            sentTrackPoints += static_cast<std::uint64_t>(tbdStates.size());
        }

        if (options.sendCooperative && !cooperativeStates.empty()) {
            advanceBatchStates(cooperativeStates, options.minRangeMeters, options.maxRangeMeters);
            const auto packet = buildTrackPacket(cooperativeStates, trackComm++, DATA_PRO_ID, CO_TRACK_MSG_ID);
            sendPacket(coSock, coAddr, packet);
            sentTrackPoints += static_cast<std::uint64_t>(cooperativeStates.size());
        }

        ++frameCount;
        if (frameCount % static_cast<std::uint64_t>(options.fps) == 0) {
            std::cout << "frames=" << frameCount
                      << " det_sent=" << sentDetections
                      << " track_sent=" << sentTrackPoints
                      << "\n";
        }

        const auto elapsed = std::chrono::steady_clock::now() - tickStart;
        if (elapsed < frameInterval) {
            std::this_thread::sleep_for(frameInterval - elapsed);
        }
    }

    closesocket(detSock);
    closesocket(trackSock);
    closesocket(tbdSock);
    closesocket(coSock);
    WSACleanup();

    std::cout << "Finished. frames=" << frameCount
              << " det_sent=" << sentDetections
              << " track_sent=" << sentTrackPoints << "\n";
    return 0;
}
