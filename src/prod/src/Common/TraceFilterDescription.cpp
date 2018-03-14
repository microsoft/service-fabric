// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    TraceFilterDescription::TraceFilterDescription(
        TraceTaskCodes::Enum taskId,
        std::string const & eventName,
        LogLevel::Enum level,
        int samplingRatio)
        :   taskId_(taskId),
            eventName_(eventName),
            level_(level),
            samplingRatio_(samplingRatio)
    {
    }

    bool TraceFilterDescription::Matches(TraceTaskCodes::Enum taskId, string const & eventName) const
    {
        return ((taskId_ == taskId) && (eventName_ == eventName));
    }

    int TraceFilterDescription::StaticCheck(TraceTaskCodes::Enum taskId, StringLiteral eventName) const
    {
        if (taskId_ != taskId)
        {
            return -1;
        }

        if (eventName_.size() == 0)
        {
            return 1;
        }

        if (eventName != eventName_)
        {
            return -1;
        }

        return 2;
    }
}
