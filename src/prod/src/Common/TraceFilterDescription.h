// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceFilterDescription
    {
        DENY_COPY(TraceFilterDescription);

    public:
        TraceFilterDescription(
            TraceTaskCodes::Enum taskId,
            std::string const & eventName,
            LogLevel::Enum level,
            int samplingRatio);

        LogLevel::Enum GetLevel() const
        {
            return level_;
        }

        int GetSamplingRatio() const
        {
            return samplingRatio_;
        }

        bool Matches(TraceTaskCodes::Enum taskId, std::string const & eventName) const;

        int StaticCheck(TraceTaskCodes::Enum taskId, StringLiteral eventName) const;

    private:
        TraceTaskCodes::Enum taskId_;
        std::string eventName_;
        LogLevel::Enum level_;
        int samplingRatio_;
    };
}
