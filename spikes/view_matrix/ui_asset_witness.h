#pragma once
#include <cstdint>
#include <cstddef>
namespace uiwitness {
constexpr int kMaxMovieBytes=2*1024*1024;
inline bool ArrayBounds(int count,int capacity) {
    return count>=8 && count<=kMaxMovieBytes && capacity>=count && capacity<=kMaxMovieBytes;
}
inline std::uint64_t Fingerprint(const unsigned char* bytes,std::size_t count) {
    std::uint64_t h=14695981039346656037ull;
    for(std::size_t i=0;i<count;++i){h^=bytes[i];h*=1099511628211ull;}
    return h;
}
}
