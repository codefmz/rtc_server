#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

inline uint16_t htons(uint16_t value)
{
#if defined(_MSC_VER)
    return _byteswap_ushort(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#else
    return static_cast<uint16_t>((value << 8) | (value >> 8));
#endif
}

inline uint16_t ntohs(uint16_t value)
{
    return htons(value);
}

inline uint32_t htonl(uint32_t value)
{
#if defined(_MSC_VER)
    return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    return ((value & 0x000000ffu) << 24) |
           ((value & 0x0000ff00u) << 8) |
           ((value & 0x00ff0000u) >> 8) |
           ((value & 0xff000000u) >> 24);
#endif
}

inline uint32_t ntohl(uint32_t value)
{
    return htonl(value);
}
