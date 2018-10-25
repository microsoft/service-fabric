// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace EntryPointType
    {
        enum Enum
        {
            None = 0,
            Exe = 1,
            DllHost = 2,
            ContainerHost = 3
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        Common::ErrorCode FromPublicApi(FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND const & publicVal, __out Enum & val);
        FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND ToPublicApi(Enum const & val);
        bool TryParseFromString(std::wstring const& string, __out Enum & val);

        DECLARE_ENUM_STRUCTURED_TRACE(EntryPointType);
    };
}
