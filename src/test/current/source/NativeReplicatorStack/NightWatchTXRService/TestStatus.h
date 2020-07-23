// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    namespace TestStatus
    {
        enum Enum
        {
            Invalid = 0,
            Running = 1,
            Finished = 2,
            NotStarted = 3
        };

        std::wstring ToString(Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Running)
            ADD_ENUM_VALUE(Finished)
            ADD_ENUM_VALUE(NotStarted)
            END_DECLARE_ENUM_SERIALIZER()
    };
}
