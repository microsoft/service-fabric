// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if KTL_USER_MODE

namespace Ktl
{
    namespace Com
    {
        class KComUtility
        {
        public:
            static HRESULT ToHRESULT(NTSTATUS FromNtStatus);
        };
    }
}

#endif
