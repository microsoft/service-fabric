// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/TextWriter.h"
#include "Common/RwLock.h"

namespace Common
{
    class StackTrace
    {
        static const int MAX_FRAME = 32; // to avoid doing any allocations
        static const int SYMBOL_NAME_MAX = 128;
    public:
        __declspec(property(get=get_FrameCount)) int FrameCount;
        int get_FrameCount() const { return frameCount; }

        StackTrace() : frameCount(0) {}

        void CaptureFromContext(CONTEXT* contextRecord, bool ignoreFirstFrame);

        void CaptureCurrentPosition();

        void WriteTo(TextWriter& w, FormatOptions const &) const;

        std::wstring ToString() const;

    private:
        int frameCount;
#if !defined(PLATFORM_UNIX)
        DWORD64 address[MAX_FRAME];
#else
        void* address[MAX_FRAME];
#endif
    };
};

