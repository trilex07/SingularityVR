// SPDX-License-Identifier: GPL-2.0-or-later
// Offline-only wrapper around this repository's GPL LZO decoder. Not linked into the mod.
#define main original_whole_package_main
#include "../uedecompress/uedecompress.cpp"
#undef main
#include <algorithm>

int main(int argc, char** argv) {
    if (argc != 3) { puts("usage: unpack_ui_chunks input.xxx output.u (outside Git)"); return 2; }
    std::vector<uint8_t> src;
    if (!ReadFile(argv[1], src) || src.size() < 128 || RdU32(src.data()) != kPackageTag) return 2;
    // Bounded discovery of a UE3 summary compression array. Each record is
    // logical offset/length + physical offset/length, followed by a nested block container.
    size_t table = 0; unsigned count = 0, matches = 0;
    for (size_t p = 12; p + 24 <= std::min<size_t>(src.size(), 256); ++p) {
        if (RdU32(&src[p]) != 2) continue; // COMPRESS_LZO; no guessed codecs
        const unsigned n = RdU32(&src[p+4]);
        if (!n || n > 32 || p + 8 + n*16 > src.size()) continue;
        bool good = true;
        for (unsigned j=0; j<n; ++j) {
            const auto* r = &src[p+8+j*16];
            const uint64_t off=RdU32(r+8), len=RdU32(r+12);
            if (len<16 || off+len>src.size() || RdU32(&src[size_t(off)])!=kPackageTag) {good=false;break;}
        }
        if (good) {table=p;count=n;++matches;}
    }
    if (matches != 1) { printf("REFUSE: %u candidate compression arrays\n", matches); return 1; }
    const size_t firstLogical=RdU32(&src[table+8]);
    if (firstLogical < table+8 || firstLogical>src.size()) return 1;
    const size_t physicalHeader=firstLogical+count*16;
    if(physicalHeader>src.size() || table+8+count*16>physicalHeader)return 1;
    std::vector<uint8_t> dst(src.begin(),src.begin()+table+8);
    dst.insert(dst.end(),src.begin()+table+8+count*16,src.begin()+physicalHeader);
    if(dst.size()!=firstLogical)return 1;
    for (unsigned j=0;j<count;++j) {
        const auto* r=&src[table+8+j*16];
        const size_t logical=RdU32(r), unc=RdU32(r+4), physical=RdU32(r+8), clen=RdU32(r+12);
        if (logical!=dst.size() || unc>128*1024*1024) return 1;
        const auto* h=&src[physical]; const unsigned block=RdU32(h+4);
        if (!block || block>1024*1024 || RdU32(h+12)!=unc) return 1;
        const unsigned blocks=unsigned((unc+block-1)/block);
        size_t cursor=physical+16+blocks*8; size_t total=0;
        if (cursor>physical+clen) return 1;
        dst.resize(logical+unc);
        for(unsigned k=0;k<blocks;++k) {
            const size_t c=RdU32(h+16+k*8), u=RdU32(h+20+k*8);
            if (!u || u>block || total+u>unc || cursor+c>physical+clen) return 1;
            size_t used=0; const char* err=nullptr;
            if(c==u) {memcpy(&dst[logical+total],&src[cursor],u);used=u;}
            else if(!Lzo1xDecompress(&src[cursor],c,&dst[logical+total],u,&used,&err)) {
                printf("ERROR chunk=%u block=%u %s\n",j,k,err);return 1;
            }
            if(used!=u)return 1;
            total+=u;cursor+=c;
        }
        if(total!=unc || cursor!=physical+clen || cursor-(physical+16+blocks*8)!=RdU32(h+8))return 1;
        printf("chunk=%u table=0x%zX logical=0x%zX physical=0x%zX blocks=%u unc=%zu verified\n",
               j,table,logical,physical,blocks,unc);
    }
    // Chunk records are physically inserted into the summary, but logical name/export
    // offsets exclude them. Only the flags/count remain in the reconstructed summary.
    memset(&dst[table],0,8);
    FILE* f=nullptr;
    if(fopen_s(&f,argv[2],"wb") || !f)return 1;
    const bool ok=fwrite(dst.data(),1,dst.size(),f)==dst.size();fclose(f);
    printf("logical package=%zu bytes; input retained unchanged\n",dst.size());
    return ok?0:1;
}
