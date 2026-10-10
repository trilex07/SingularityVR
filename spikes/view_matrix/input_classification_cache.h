#pragma once
#include <windows.h>
#include <cstring>

// The audited classifier compares only GUID::Data1 (VID/PID). Keep its exact
// answer, including FALSE, until Windows reports a device topology change.
class InputClassificationCache {
    struct Entry { DWORD product; BOOL result; };
    SRWLOCK lock_ = SRWLOCK_INIT;
    Entry entries_[64]{};
    unsigned count_ = 0;
    LONG storedEpoch_ = 0;
    volatile LONG epoch_ = 0;
public:
    volatile LONG hits = 0, misses = 0;
    void Invalidate() { InterlockedIncrement(&epoch_); }
    BOOL Query(const GUID* product, BOOL (__cdecl *original)(const GUID*)) {
        if (!product) return original(product);
        AcquireSRWLockExclusive(&lock_);
        const LONG epoch = InterlockedCompareExchange(&epoch_, 0, 0);
        if (storedEpoch_ != epoch) { count_ = 0; storedEpoch_ = epoch; }
        for (unsigned i = 0; i < count_; ++i) {
            if (entries_[i].product == product->Data1) {
                const BOOL result = entries_[i].result;
                InterlockedIncrement(&hits);
                ReleaseSRWLockExclusive(&lock_);
                return result;
            }
        }
        InterlockedIncrement(&misses);
        const DWORD id = product->Data1;
        // COM's STA wait can pump messages and re-enter game input. Never hold
        // our non-recursive lock across the original WMI call.
        ReleaseSRWLockExclusive(&lock_);
        const BOOL result = original(product);
        AcquireSRWLockExclusive(&lock_);
        if (storedEpoch_ == epoch && InterlockedCompareExchange(&epoch_,0,0) == epoch) {
            bool present = false;
            for (unsigned i = 0; i < count_; ++i) if (entries_[i].product == id) present = true;
            if (!present && count_ < 64) entries_[count_++] = { id, result };
        }
        ReleaseSRWLockExclusive(&lock_);
        return result;
    }
};
