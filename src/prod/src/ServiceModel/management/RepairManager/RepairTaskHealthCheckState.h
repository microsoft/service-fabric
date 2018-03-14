// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairTaskHealthCheckState
        {
            enum Enum
            {
                NotStarted = FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_NOT_STARTED,
                InProgress = FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_IN_PROGRESS,
                Succeeded = FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SUCCEEDED,
                Skipped = FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SKIPPED,
                TimedOut = FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_TIMEDOUT,				
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE, Enum &);
            FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE ToPublicApi(Enum const);

            DECLARE_ENUM_STRUCTURED_TRACE( RepairTaskHealthCheckState )
			
            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(NotStarted)
                ADD_ENUM_VALUE(InProgress)
                ADD_ENUM_VALUE(Succeeded)
                ADD_ENUM_VALUE(Skipped)
                ADD_ENUM_VALUE(TimedOut)                
            END_DECLARE_ENUM_SERIALIZER()			
        };
    }
}
