// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ContainerInfoType
    {
        enum Enum
        {
            Invalid = 0,
            Logs = 1,
            Stats = 2,
            Events = 3,
            Inspect = 4,
            RawAPI = 5,

            LAST_ENUM_PLUS_ONE = 6,
        };

        class TextWriter;
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        bool FromWString(std::wstring const & string, __out Enum & val);

        DECLARE_ENUM_STRUCTURED_TRACE(ContainerInfoType)
    };
}
