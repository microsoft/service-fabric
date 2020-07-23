// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class DetourManager : private DenyCopy
    {
    public:
        DetourManager(bool restore);
        ~DetourManager();
        void Attach(__in void ** oldMethod, __in void * newMethod);
        void Detach(__in void ** oldMethod, __in void * newMethod);

    private:
        bool restored_;
        DetourTransaction transaction_;

        static bool Restore(bool restore);
    };
}
