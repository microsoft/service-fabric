//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace ChaosScheduleStatus
        {
            enum Enum
            {
                Invalid = 0,
                Active  = 1,
                Expired = 2,
                Pending = 3,
                Stopped = 4
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Common::ErrorCode FromPublicApi(__in FABRIC_CHAOS_SCHEDULE_STATUS const & publicStatus, __out Enum & status);
            FABRIC_CHAOS_SCHEDULE_STATUS ToPublicApi(Enum const & status);

            std::wstring ToString(ChaosScheduleStatus::Enum const & val);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Active)
                ADD_ENUM_VALUE(Expired)
                ADD_ENUM_VALUE(Pending)
                ADD_ENUM_VALUE(Stopped)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
