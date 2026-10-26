// SPDX-License-Identifier: GPL-2.0-or-later
// Offline-only reader. Reuses the existing decoder, never linked into the mod.
#define main ignored_package_decoder_main
#include "../uedecompress/uedecompress.cpp"
#undef main
#include <cerrno>

int main(int argc, char** argv) {
    if (argc != 6) { puts("read_tfc_bulk private.tfc offset compressed-size raw-size new-output.bin"); return 2; }
    char* end = nullptr;
    const auto offset = _strtoui64(argv[2], &end, 0); if (!end || *end) return 2;
    const auto length = strtoul(argv[3], &end, 0); if (!end || *end || length < 24 || length > 32*1024*1024) return 2;
    const auto raw = strtoul(argv[4], &end, 0); if (!end || *end || !raw || raw > 32*1024*1024) return 2;
    FILE* f = nullptr;
    if (fopen_s(&f, argv[5], "rb") == 0) { fclose(f); puts("existing output refused"); return 1; }
    if (fopen_s(&f, argv[1], "rb") || !f) return 1;
    if (_fseeki64(f, offset, SEEK_SET)) { fclose(f); return 1; }
    std::vector<uint8_t> src(length), dst(raw);
    const auto read = fread(src.data(), 1, src.size(), f); fclose(f);
    if (read != src.size() || RdU32(src.data()) != kPackageTag) return 1;
    const auto block = RdU32(src.data()+4);
    if (!block || block > 1024*1024 || RdU32(src.data()+12) != raw) return 1;
    const auto blocks = (raw+block-1)/block;
    size_t cursor = 16 + blocks*8, total = 0;
    if (cursor > src.size()) return 1;
    for (unsigned k=0; k<blocks; ++k) {
        const auto c = RdU32(src.data()+16+k*8), u = RdU32(src.data()+20+k*8);
        if (!u || u > block || total+u > raw || cursor+c > src.size()) return 1;
        size_t used = 0; const char* error = nullptr;
        if (c == u) { memcpy(dst.data()+total,src.data()+cursor,u); used=u; }
        else if (!Lzo1xDecompress(src.data()+cursor,c,dst.data()+total,u,&used,&error)) return 1;
        if (used != u) return 1;
        total += u; cursor += c;
    }
    if (total != raw || cursor != src.size() || cursor-(16+blocks*8) != RdU32(src.data()+8)) return 1;
    if (fopen_s(&f, argv[5], "wbx") || !f) return 1;
    const bool ok = fwrite(dst.data(),1,dst.size(),f)==dst.size(); fclose(f);
    return ok ? 0 : 1;
}
