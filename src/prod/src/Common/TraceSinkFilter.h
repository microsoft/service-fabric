// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TraceSinkFilter
    {
    public:
        TraceSinkFilter();

        void SetDefaultLevel(LogLevel::Enum level)
        {
            defaultLevel_ = level;
        }

        void SetDefaultSamplingRatio(int samplingRatio)
        {
            defaultSamplingRatio_ = samplingRatio;
        }

        void AddFilter(TraceTaskCodes::Enum taskId, std::string const & eventName, LogLevel::Enum level, int samplingRatio);

        void ClearFilters();

        bool StaticCheck(LogLevel::Enum level, TraceTaskCodes::Enum taskId, StringLiteral eventName, int & samplingRatio) const;

    private:
        std::vector<std::unique_ptr<TraceFilterDescription>> filters_;
        LogLevel::Enum defaultLevel_;
        int defaultSamplingRatio_;
    };
}
