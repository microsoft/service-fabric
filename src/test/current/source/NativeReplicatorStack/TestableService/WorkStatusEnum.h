// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    namespace WorkStatusEnum
    {
        enum Enum
        {
            Unknown,
            NotStarted,
            Faulted,
            InProgress,
            Done
        };

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Unknown)
            ADD_ENUM_VALUE(NotStarted)
            ADD_ENUM_VALUE(Faulted)
            ADD_ENUM_VALUE(InProgress)
            ADD_ENUM_VALUE(Done)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
