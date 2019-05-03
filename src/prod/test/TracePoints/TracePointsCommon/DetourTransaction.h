// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class DetourTransaction : private DenyCopy
    {
    public:
        DetourTransaction();
        ~DetourTransaction() noexcept(false);

        static DetourTransaction Create();
        void Abort();
        void Commit();

    private:
        static USHORT const Started;
        static USHORT const Aborted;
        static USHORT const Committed;

        USHORT state_;
    };
}
