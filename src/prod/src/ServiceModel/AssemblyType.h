// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace DllHostHostedDllKind
    {
        enum Enum
        {
            Invalid = 0,
            Unmanaged = 1,
            Managed = 2,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_HOSTED_DLL_KIND const & publicVal, __out Enum & val);
        FABRIC_DLLHOST_HOSTED_DLL_KIND ToPublicApi(Enum const & val);
    };
}
