// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        namespace RepairTaskFlags
        {
            enum Enum
            {
                None = 0x0,
                CancelRequested = 0x1,
                AbortRequested = 0x2,
                ForcedApproval = 0x4,
            };

            DEFINE_ENUM_FLAG_OPERATORS(Enum);
            DECLARE_ENUM_STRUCTURED_TRACE(RepairTaskFlags);

            void WriteToTextWriter(Common::TextWriter &, Enum const &);

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_FLAGS, Enum &);
            FABRIC_REPAIR_TASK_FLAGS ToPublicApi(Enum const);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(None)
                ADD_ENUM_VALUE(CancelRequested)
                ADD_ENUM_VALUE(AbortRequested)
                ADD_ENUM_VALUE(ForcedApproval)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
