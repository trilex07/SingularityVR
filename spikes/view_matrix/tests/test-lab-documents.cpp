#include <windows.h>
#include <cassert>
#include <cstdio>

static void Log(const char*, ...) {}
#include "../lab_documents.h"

int main() {
    wchar_t baseline[MAX_PATH]{}, appData[MAX_PATH]{}, expected[MAX_PATH]{};
    assert(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, baseline)));
    assert(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appData)));
    assert(GetTempPathW(MAX_PATH, expected) > 0);
    assert(wcscat_s(expected, L"Singularity-Quest2-20261127\\documents-lift-checkpoint") == 0);
    assert(_wcsicmp(expected, baseline) != 0);
    assert(lab_documents::Install());

    wchar_t actual[MAX_PATH]{};
    assert(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE,
                                    nullptr, SHGFP_TYPE_CURRENT, actual)));
    assert(_wcsicmp(expected, actual) == 0);
    assert(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, actual)));
    assert(_wcsicmp(appData, actual) == 0);
    assert(SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, actual)));
    assert(_wcsicmp(expected, actual) == 0);
    assert(lab_documents::redirects == 2);
    printf("PASS: Documents redirected by the host import, LocalAppData delegated\n");
}
