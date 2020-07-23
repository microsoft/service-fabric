// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    namespace ClientOperationEnum
    {
        enum Enum
        {
            DoWork, // Schedule DoWorkOnClientRequestAsync. Mark the work status as 'InProgress'.
            CheckWorkStatus, // Check the work status. See WorkStatueEnum.h
            GetProgress, // Get the number of records in each store.
        };

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(DoWork)
            ADD_ENUM_VALUE(CheckWorkStatus)
            ADD_ENUM_VALUE(GetProgress)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
