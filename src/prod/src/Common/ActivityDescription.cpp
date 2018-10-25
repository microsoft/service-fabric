//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;

    ActivityDescription ActivityDescription::Empty(ActivityId::Empty, ActivityType::Enum::Unknown);

    ActivityDescription::ActivityDescription()
        : activityId_(ActivityId::Empty)
        , activityType_(Common::ActivityType::Enum::Unknown)
    {
    }

    ActivityDescription::ActivityDescription(Common::ActivityId const & activityId, Common::ActivityType::Enum const & activityType)
        : activityId_(activityId)
        , activityType_(activityType)
    {
    }

    ActivityDescription::ActivityDescription(Common::ActivityId && activityId, Common::ActivityType::Enum && activityType)
        : activityId_(move(activityId))
        , activityType_(move(activityType))
    {
    }

    ActivityDescription::ActivityDescription(ActivityDescription const & other)
        : activityId_(other.activityId_)
        , activityType_(other.activityType_)
    {
    }

    ActivityDescription::ActivityDescription(ActivityDescription && other)
        : activityId_(move(other.activityId_))
        , activityType_(move(other.activityType_))
    {
    }

    void ActivityDescription::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("{0}:{1}", activityId_, activityType_);
    }

    wstring ActivityDescription::ToString() const
    {
        return wformatString("{0}:{1}", activityId_, activityType_);
    }

    string ActivityDescription::AddField(TraceEvent & traceEvent, std::string const & name)
    {
        std::string format = "{0}:{1}";

        size_t index = 0;
        traceEvent.AddEventField<Common::ActivityId>(format, name+".activityId", index);
        traceEvent.AddEventField<Common::ActivityType::Trace>(format, name+".activityType", index);

        return format;
    }

    void ActivityDescription::FillEventData(TraceEventContext & context) const
    {
        context.Write<Common::ActivityId>(activityId_);
        context.WriteCopy(static_cast<uint>(activityType_));
    }
}
