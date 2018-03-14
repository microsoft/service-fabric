// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

::FABRIC_SERVICE_PARTITION_ACCESS_STATUS  AccessStatus::ConvertToPublicAccessStatus(AccessStatus::Enum toConvert)
{
    switch(toConvert)
    {
        case AccessStatus::TryAgain:
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING;
        case AccessStatus::NotPrimary:
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
        case AccessStatus::NoWriteQuorum:
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM;
        case AccessStatus::Granted:
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
        default:
            Common::Assert::CodingError("Unknown Access Status");
    }
}

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace AccessStatus
        {
            void AccessStatus::WriteToTextWriter(TextWriter & w, AccessStatus::Enum const & val)
            {
                w << ToString(val);
            }

            ENUM_STRUCTURED_TRACE(AccessStatus, TryAgain, LastValidEnum);
        }
    }
}

wstring AccessStatus::ToString(AccessStatus::Enum accessStatus)
{
    switch (accessStatus)
    {
        case TryAgain: 
            return L"TryAgain";
        case NotPrimary: 
            return L"NotPrimary";
        case NoWriteQuorum: 
            return L"NoWriteQuorum";
        case Granted: 
            return L"Granted";
        default: 
            Common::Assert::CodingError("Unknown Access Status");
    }
}

