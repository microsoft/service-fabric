// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace FullCopyMode
    {
        enum Enum : ULONG
        {
            Default = 0,
            Physical = 1,
            Logical = 2,
            Rebuild = 3,
            FirstValidEnum = Default,
            LastValidEnum = Rebuild
        };

        FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE ToPublicApi(
            __in FullCopyMode::Enum);

        Common::ErrorCode FromPublicApi(
            __in FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE,
            __out Enum &);

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(FullCopyMode)
    };
};
