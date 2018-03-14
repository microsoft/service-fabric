// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairTaskResult
        {
            enum Enum
            {
                Invalid = FABRIC_REPAIR_TASK_RESULT_INVALID,
                Succeeded = FABRIC_REPAIR_TASK_RESULT_SUCCEEDED,
                Cancelled = FABRIC_REPAIR_TASK_RESULT_CANCELLED,
                Interrupted = FABRIC_REPAIR_TASK_RESULT_INTERRUPTED,
                Failed = FABRIC_REPAIR_TASK_RESULT_FAILED,
                Pending = FABRIC_REPAIR_TASK_RESULT_PENDING,
            };

            DEFINE_ENUM_FLAG_OPERATORS(Enum);
            DECLARE_ENUM_STRUCTURED_TRACE( RepairTaskResult )

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_RESULT, Enum &);
            FABRIC_REPAIR_TASK_RESULT ToPublicApi(Enum const);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Succeeded)
                ADD_ENUM_VALUE(Cancelled)
                ADD_ENUM_VALUE(Interrupted)
                ADD_ENUM_VALUE(Failed)
                ADD_ENUM_VALUE(Pending)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
