// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    namespace TestAction
    {
        enum Enum
        {
            Invalid = 0,
            Run = 1,
            Status = 2,
        };

        std::wstring ToString(Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Run)
            ADD_ENUM_VALUE(Status)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
