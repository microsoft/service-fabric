// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    namespace ClientOperationEnum
    {
        enum Enum
        {
            ScheduleWork, // Schedule DoWorkOnClientRequestAsync. Mark the work status as 'InProgress'.
            CheckWorkStatus, // Check the work status. See WorkStatueEnum.h
            GetWorkResult, // Get the number of records in each store.

			ReportFaultTransient,
			ReportFaultPermanent,

			Test, // for test purpose
        };

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(ScheduleWork)
            ADD_ENUM_VALUE(CheckWorkStatus)
            ADD_ENUM_VALUE(GetWorkResult)
			ADD_ENUM_VALUE(ReportFaultTransient)
			ADD_ENUM_VALUE(ReportFaultPermanent)
			ADD_ENUM_VALUE(Test)
        END_DECLARE_ENUM_SERIALIZER()
    }
}
