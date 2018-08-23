//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace ActivityType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Enum::Unknown:
                w << L"Unknown"; return;
            case Enum::ClientReportFaultEvent:
                w << L"ClientReportFaultEvent"; return;
            case Enum::ServiceReportFaultEvent:
                w << L"ServiceReportFaultEvent"; return;
            case Enum::ServicePackageEvent:
                w << L"ServicePackageEvent"; return;
            case Enum::ReplicaEvent:
                w << L"ReplicaEvent"; return;
            default: 
                Common::Assert::CodingError("Unknown activity type.");
            };
        }

        ENUM_STRUCTURED_TRACE(ActivityType, Unknown, LastValidEnum);
    }
}
