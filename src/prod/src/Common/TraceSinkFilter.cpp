// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    TraceSinkFilter::TraceSinkFilter()
        :   defaultLevel_(LogLevel::Info),
            defaultSamplingRatio_(0)
    {
    }

    void TraceSinkFilter::AddFilter(TraceTaskCodes::Enum taskId, std::string const & eventName, LogLevel::Enum level, int samplingRatio)
    {
        unique_ptr<TraceFilterDescription> filter = make_unique<TraceFilterDescription>(taskId, eventName, level, samplingRatio);

        for (auto it = filters_.begin(); it != filters_.end(); ++it)
        {
            if ((*it)->Matches(taskId, eventName))
            {
                *it = move(filter);
                return;
            }
        }

        filters_.push_back(move(filter));
    }

    void TraceSinkFilter::ClearFilters()
    {
        filters_.clear();
    }

    bool TraceSinkFilter::StaticCheck(LogLevel::Enum level, TraceTaskCodes::Enum taskId, StringLiteral eventName, int & samplingRatio) const
    {
        int result = -1;
        bool enable = (level <= defaultLevel_);
        samplingRatio = defaultSamplingRatio_;

        for (unique_ptr<TraceFilterDescription> const & filter:filters_)
        {
            int rank = filter->StaticCheck(taskId, eventName);
            if (rank > result)
            {
                enable = (level <= filter->GetLevel());
                samplingRatio = filter->GetSamplingRatio();
                result = rank;
            }
        }

        return enable;
    }
}
