// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace ChaosStatus
        {
            enum Enum
            {
                Invalid = 0,
                Running = 1,
                Stopped = 2
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Common::ErrorCode FromPublicApi(__in FABRIC_CHAOS_STATUS const & publicStatus, __out Enum & status);
            FABRIC_CHAOS_STATUS ToPublicApi(Enum const & status);
            std::wstring ToString(ChaosStatus::Enum const & val);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Running)
                ADD_ENUM_VALUE(Stopped)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
