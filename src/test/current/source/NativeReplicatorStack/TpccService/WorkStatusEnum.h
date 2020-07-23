// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    namespace WorkStatusEnum
    {
        enum Enum
        {
            Unknown,
            NotStarted,
            InProgress,
            Done
        };

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Unknown)
            ADD_ENUM_VALUE(NotStarted)
            ADD_ENUM_VALUE(InProgress)
            ADD_ENUM_VALUE(Done)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
