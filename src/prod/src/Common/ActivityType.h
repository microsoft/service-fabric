//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ActivityType
    {
        enum Enum
        {
            Unknown = 0,
            ClientReportFaultEvent = 1,
            ServiceReportFaultEvent = 2,
            ReplicaEvent = 3,
            ServicePackageEvent = 4,
            LastValidEnum = ServicePackageEvent
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        DECLARE_ENUM_STRUCTURED_TRACE(ActivityType);
    }
}
