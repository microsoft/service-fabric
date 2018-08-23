//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Common
{
    class ActivityDescription
        : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(ActivityDescription)

    public:
        ActivityDescription();
        explicit ActivityDescription(Common::ActivityId const &, Common::ActivityType::Enum const &);
        explicit ActivityDescription(Common::ActivityId &&, Common::ActivityType::Enum &&);
        ActivityDescription(ActivityDescription const &);
        ActivityDescription(ActivityDescription &&);

        __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
        Common::ActivityId const & get_ActivityId() const { return activityId_; }

        __declspec(property(get = get_ActivityType)) Common::ActivityType::Enum const & ActivityType;
        Common::ActivityType::Enum const & get_ActivityType() const { return activityType_; }

        void WriteTo(TextWriter& w, FormatOptions const& f) const;
        std::wstring ToString() const;

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;

        static ActivityDescription Empty;

        FABRIC_FIELDS_02(activityId_, activityType_);

    private:
        Common::ActivityId activityId_;
        Common::ActivityType::Enum activityType_;
    };
}
