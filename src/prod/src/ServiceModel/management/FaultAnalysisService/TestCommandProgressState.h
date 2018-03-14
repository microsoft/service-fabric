// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace TestCommandProgressState
        {
            enum Enum
            {
                Invalid = 0,
                Running = 1,
                RollingBack = 2,
                Completed = 3,
                Faulted = 4,
                Cancelled = 5,
                ForceCancelled = 6
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            TestCommandProgressState::Enum FromPublicApi(FABRIC_TEST_COMMAND_PROGRESS_STATE const &);
            FABRIC_TEST_COMMAND_PROGRESS_STATE ToPublicApi(Enum const &);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Running)
                ADD_ENUM_VALUE(RollingBack)
                ADD_ENUM_VALUE(Completed)
                ADD_ENUM_VALUE(Faulted)
                ADD_ENUM_VALUE(Cancelled)
                ADD_ENUM_VALUE(ForceCancelled)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
