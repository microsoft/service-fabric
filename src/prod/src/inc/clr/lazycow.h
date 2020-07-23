// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef LAZY_COW_H
#define LAZY_COW_H

#ifdef FEATURE_LAZY_COW_PAGES

#ifdef _WIN64 // due to the way we track pages, we cannot currently support 64-bit.
#error FEATURE_LAZY_COW_PAGES is only supported on 32-bit platforms.
#endif

class PEDecoder;

// If hModule is a native image, establishes copy-on-write tracking for the image.
// FreeLazyCOWPages must be called immediately before the module is unloaded.
void AllocateLazyCOWPages(PEDecoder * pImage);

// If hModule is a native image, disestablishes copy-on-write tracking for the image.
// The image must be immediately unloaded following this call.
void FreeLazyCOWPages(PEDecoder * pImage);

bool IsInReadOnlyLazyCOWPage(void* p);


// Forces the page(s) covered by the given address range to be made writable,
// if they are being tracked as copy-on-write pages.  Otherwise does nothing.
// Returns false if we could not allocate the necessary memory.
bool EnsureWritablePagesNoThrow(void* p, size_t len);

// Version for executable pages
bool EnsureWritableExecutablePagesNoThrow(void* p, size_t len);

// Throwing version of EnsureWritablePagesNoThrow 
void EnsureWritablePages(void* p, size_t len);

// Version for executable pages
void EnsureWritableExecutablePages(void* p, size_t len);

#else //FEATURE_LAZY_COW_PAGES

inline bool EnsureWritablePagesNoThrow(void* p, size_t len)
{
    return true;
}

inline bool EnsureWritableExecutablePagesNoThrow(void* p, size_t len)
{
    return true;
}

inline void EnsureWritablePages(void* p, size_t len)
{
}

inline void EnsureWritableExecutablePages(void* p, size_t len)
{
}

#endif //FEATURE_LAZY_COW_PAGES

// Typed version of EnsureWritable.  Returns p, so this can be inserted in expressions.
// Ignores any failure to allocate.  In typical cases this means that the write will AV.
// In the CLR that's OK; we handle the AV, try EnsureWritable(void*,size_t), and
// fail-fast when it fails.
template<typename T>
inline T* EnsureWritablePages(T* p)
{
    EnsureWritablePages(p, sizeof(T));
    return p;
}

template<typename T>
inline T* EnsureWritableExecutablePages(T* p)
{
    EnsureWritableExecutablePages(p, sizeof(T));
    return p;
}

#endif // LAZY_COW_H
