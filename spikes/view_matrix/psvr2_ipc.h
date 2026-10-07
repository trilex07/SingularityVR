#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// SingularityVR PSVR2 IPC wire protocol (clean-room).
//
// Adapted from the KHARVOX design (MIT, (c) 2026 CactusVRStudios):
//   src/psvr2/Psvr2IpcProtocol.h  (magic/version/message envelope,
//   Hello-first auth, monotonic sequence, packed-uint64 dedup codec)
//   src/psvr2/Psvr2IpcClient.cpp   (write-only pipe worker, forward-on-change)
//   src/psvr2/Psvr2BridgeMain.cpp  (pipe server, session state, safe apply)
// See docs/REFERENCE_VR_IMPLEMENTATIONS.md. No KHARVOX code is copied; the
// magic, version policy and payload layout below are original. One deliberate
// fix versus KHARVOX: a SINGLE protocol version shared by game and bridge
// (KHARVOX shipped launcher-C# v1 against C++ v3).
//
// Layout (all integers little-endian, byte-packed, no struct padding):
//   Header (20 bytes): magic u32 | version u16 | type u8 | reserved u8 |
//                      sequence u32 | timeMs u32 | payloadBytes u16 | reserved u16
//   (timeMs is u32 session-relative milliseconds; wraps after ~49 days.)
//   Hello payload: token bytes (1..kMaxTokenBytes, raw, no NUL required)
//   TriggerState payload (16 bytes): effect u8 | hand u8 | start u8 | end u8 |
//     strength u8 | position u8 | amplitude u8 | frequency u8 |
//     startStrength u8 | endStrength u8 | reason u8 | reserved[5]
//   Shutdown payload: empty.
// Max message: 20 + 64 = 84 bytes (well under one pipe write).

namespace psvr2ipc {

constexpr std::uint32_t kMagic = 0x32565353u; // "SSV2" LE (Singularity Sense V2)
constexpr std::uint16_t kVersion = 1;
constexpr std::size_t kMaxTokenBytes = 64;
constexpr std::size_t kHeaderBytes = 20;
constexpr std::size_t kTriggerBytes = 16;
constexpr std::size_t kMaxMessageBytes = kHeaderBytes + kMaxTokenBytes;

enum class MsgType : std::uint8_t { Hello = 1, TriggerState = 2, Shutdown = 3 };

inline void WriteU16(std::uint8_t* p, std::uint16_t v) {
    p[0] = (std::uint8_t)(v & 0xFF);
    p[1] = (std::uint8_t)((v >> 8) & 0xFF);
}
inline void WriteU32(std::uint8_t* p, std::uint32_t v) {
    p[0] = (std::uint8_t)(v & 0xFF);
    p[1] = (std::uint8_t)((v >> 8) & 0xFF);
    p[2] = (std::uint8_t)((v >> 16) & 0xFF);
    p[3] = (std::uint8_t)((v >> 24) & 0xFF);
}
inline void WriteU64(std::uint8_t* p, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) p[i] = (std::uint8_t)((v >> (8 * i)) & 0xFF);
}
inline std::uint16_t ReadU16(const std::uint8_t* p) {
    return (std::uint16_t)(p[0] | ((std::uint16_t)p[1] << 8));
}
inline std::uint32_t ReadU32(const std::uint8_t* p) {
    return (std::uint32_t)p[0] | ((std::uint32_t)p[1] << 8) | ((std::uint32_t)p[2] << 16) |
           ((std::uint32_t)p[3] << 24);
}
inline std::uint64_t ReadU64(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= (std::uint64_t)p[i] << (8 * i);
    return v;
}

// Pipe names look like \\.\pipe\SingularityPsvr2_<pid>_<tag>: printable
// ASCII, bounded length, no traversal.
inline bool PipeNameValid(const char* name, std::size_t len) {
    if (!name || len < 10 || len > 128) return false;
    for (std::size_t i = 0; i < len; ++i) {
        const char c = name[i];
        if (c == '\0') return i >= 10; // NUL terminates; must already be long enough
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '\\' || c == '.'))
            return false;
    }
    return true;
}
inline bool TokenValid(const std::uint8_t* token, std::size_t len) {
    if (!token || len == 0 || len > kMaxTokenBytes) return false;
    for (std::size_t i = 0; i < len; ++i)
        if (!((token[i] >= 'a' && token[i] <= 'z') || (token[i] >= 'A' && token[i] <= 'Z') ||
              (token[i] >= '0' && token[i] <= '9') || token[i] == '_' || token[i] == '-' ||
              token[i] == '+' || token[i] == '/'))
            return false;
    return true;
}

// Fixed 8-byte effect descriptor shared by client pack and bridge apply.
// Mirrors psvr2trigger::Pack field order (effect, hand, start, end,
// strength, position, amplitude, frequency) plus slope strengths.
struct EffectDesc {
    std::uint8_t effect = 0, hand = 0;
    std::uint8_t start = 0, end = 0, strength = 0;
    std::uint8_t position = 0, amplitude = 0, frequency = 0;
    std::uint8_t startStrength = 0, endStrength = 0;
    bool operator==(const EffectDesc& o) const {
        return std::memcmp(this, &o, sizeof(*this)) == 0;
    }
    bool IsOff() const { return effect == 0; }
};

inline std::size_t EncodeHeader(std::uint8_t out[kHeaderBytes], MsgType type, std::uint32_t sequence,
                                std::uint32_t timeMs, std::size_t payloadBytes) {
    WriteU32(out + 0, kMagic);
    WriteU16(out + 4, kVersion);
    out[6] = (std::uint8_t)type;
    out[7] = 0;
    WriteU32(out + 8, sequence);
    WriteU32(out + 12, timeMs);
    WriteU16(out + 16, (std::uint16_t)payloadBytes);
    WriteU16(out + 18, 0);
    return kHeaderBytes;
}

// Full message encode: header + payload length prefix handled by caller.
// Returns total bytes, 0 on invalid input.
inline std::size_t EncodeHello(std::uint8_t out[kMaxMessageBytes], std::uint32_t sequence,
                               std::uint32_t timeMs, const std::uint8_t* token,
                               std::size_t tokenLen) {
    if (!TokenValid(token, tokenLen)) return 0;
    EncodeHeader(out, MsgType::Hello, sequence, timeMs, tokenLen);
    std::memcpy(out + 20, token, tokenLen);
    return kHeaderBytes + tokenLen;
}

inline std::size_t EncodeTrigger(std::uint8_t out[kMaxMessageBytes], std::uint32_t sequence,
                                 std::uint32_t timeMs, const EffectDesc& fx) {
    EncodeHeader(out, MsgType::TriggerState, sequence, timeMs, kTriggerBytes);
    out[20] = fx.effect;
    out[21] = fx.hand;
    out[22] = fx.start;
    out[23] = fx.end;
    out[24] = fx.strength;
    out[25] = fx.position;
    out[26] = fx.amplitude;
    out[27] = fx.frequency;
    out[28] = fx.startStrength;
    out[29] = fx.endStrength;
    out[30] = 0;
    out[31] = out[32] = out[33] = out[34] = out[35] = 0;
    return kHeaderBytes + kTriggerBytes;
}

inline std::size_t EncodeShutdown(std::uint8_t out[kMaxMessageBytes], std::uint32_t sequence,
                                  std::uint32_t timeMs) {
    EncodeHeader(out, MsgType::Shutdown, sequence, timeMs, 0);
    return kHeaderBytes;
}

struct Decoded {
    MsgType type = MsgType::Hello;
    std::uint32_t sequence = 0;
    std::uint32_t timeMs = 0;
    EffectDesc fx;
    const std::uint8_t* token = nullptr;
    std::size_t tokenLen = 0;
};

// Pure validator for one message buffer. 0 = ok, <0 = reject reason.
inline int Decode(const std::uint8_t* data, std::size_t len, Decoded& out) {
    if (!data || len < kHeaderBytes) return -1;
    if (ReadU32(data + 0) != kMagic) return -2;
    if (ReadU16(data + 4) != kVersion) return -3;
    const std::uint8_t t = data[6];
    if (t < 1 || t > 3) return -4;
    if (data[7] != 0) return -5;
    const std::size_t payload = ReadU16(data + 16);
    if (data[18] != 0 || data[19] != 0) return -6;
    if (kHeaderBytes + payload != len) return -6;
    out.type = (MsgType)t;
    out.sequence = ReadU32(data + 8);
    out.timeMs = ReadU32(data + 12);
    out.token = nullptr;
    out.tokenLen = 0;
    if (out.type == MsgType::Hello) {
        if (payload == 0 || payload > kMaxTokenBytes) return -7;
        if (!TokenValid(data + 20, payload)) return -8;
        out.token = data + 20;
        out.tokenLen = payload;
    } else if (out.type == MsgType::TriggerState) {
        if (payload != kTriggerBytes) return -9;
        out.fx.effect = data[20];
        out.fx.hand = data[21];
        out.fx.start = data[22];
        out.fx.end = data[23];
        out.fx.strength = data[24];
        out.fx.position = data[25];
        out.fx.amplitude = data[26];
        out.fx.frequency = data[27];
        out.fx.startStrength = data[28];
        out.fx.endStrength = data[29];
        if (out.fx.effect > 4) return -10;
        if (out.fx.hand > 1) return -11;
    } else {
        if (payload != 0) return -12;
    }
    return 0;
}

// Bridge-side per-connection state: Hello-first auth with constant-time
// token compare, monotonic sequence enforcement. Pure + unit-testable.
class ServerSession {
public:
    ServerSession(const std::uint8_t* expectToken, std::size_t tokenLen) {
        validCfg_ = TokenValid(expectToken, tokenLen);
        tokenLen_ = validCfg_ ? tokenLen : 0;
        if (validCfg_) std::memcpy(token_, expectToken, tokenLen);
    }
    // Returns 0 on accept; Decoded is valid. <0 rejects (caller drops connection).
    int Accept(const std::uint8_t* data, std::size_t len, Decoded& out) {
        if (!validCfg_) return -100;
        if (Decode(data, len, out) != 0) return -101;
        if (!authed_) {
            if (out.type != MsgType::Hello) return -102; // auth first
            if (out.tokenLen != tokenLen_) return -103;
            unsigned diff = 0;
            for (std::size_t i = 0; i < tokenLen_; ++i)
                diff |= (unsigned)(out.token[i] ^ token_[i]);
            if (diff != 0) return -104;
            authed_ = true;
            lastSeq_ = out.sequence;
            return 0;
        }
        if (out.sequence <= lastSeq_) return -105; // replay / reorder
        if (out.type == MsgType::Hello) return -106; // re-hello not allowed (seq not consumed)
        lastSeq_ = out.sequence;
        return 0;
    }
    bool Authed() const { return authed_; }

private:
    bool validCfg_ = false, authed_ = false;
    std::uint32_t lastSeq_ = 0;
    std::uint8_t token_[kMaxTokenBytes] = {0};
    std::size_t tokenLen_ = 0;
};

} // namespace psvr2ipc
