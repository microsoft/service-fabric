// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairTaskState
        {
            enum Enum
            {
                Invalid = FABRIC_REPAIR_TASK_STATE_INVALID,
                Created = FABRIC_REPAIR_TASK_STATE_CREATED,
                Claimed = FABRIC_REPAIR_TASK_STATE_CLAIMED,
                Preparing = FABRIC_REPAIR_TASK_STATE_PREPARING,
                Approved = FABRIC_REPAIR_TASK_STATE_APPROVED,
                Executing = FABRIC_REPAIR_TASK_STATE_EXECUTING,
                Restoring = FABRIC_REPAIR_TASK_STATE_RESTORING,
                Completed = FABRIC_REPAIR_TASK_STATE_COMPLETED,
            };

            DEFINE_ENUM_FLAG_OPERATORS(Enum);
            DECLARE_ENUM_STRUCTURED_TRACE( RepairTaskState )

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_STATE, Enum &);
            FABRIC_REPAIR_TASK_STATE ToPublicApi(Enum const);

            bool TryParseValue(std::wstring const & input, Enum & value);
            Common::ErrorCode ParseStateMask(std::wstring const & input, Enum & value);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Created)
                ADD_ENUM_VALUE(Claimed)
                ADD_ENUM_VALUE(Preparing)
                ADD_ENUM_VALUE(Approved)
                ADD_ENUM_VALUE(Executing)
                ADD_ENUM_VALUE(Restoring)
                ADD_ENUM_VALUE(Completed)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
