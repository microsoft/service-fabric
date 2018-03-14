// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationVersion : public Serialization::FabricSerializable
    {
    public:
        ApplicationVersion();
        ApplicationVersion(RolloutVersion const & value);
        ApplicationVersion(ApplicationVersion const & other);
        ApplicationVersion(ApplicationVersion && other);
        virtual ~ApplicationVersion();

        static const ApplicationVersion Zero;

        __declspec(property(get=get_Value)) RolloutVersion const & RolloutVersionValue;
        RolloutVersion const & get_Value() const { return value_; }

        ApplicationVersion const & operator = (ApplicationVersion const & other);
        ApplicationVersion const & operator = (ApplicationVersion && other);

        bool operator == (ApplicationVersion const & other) const;
        bool operator != (ApplicationVersion const & other) const;

        int compare(ApplicationVersion const & other) const;
        bool operator < (ApplicationVersion const & other) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & applicationVersionString, __out ApplicationVersion & applicationVersion);

        FABRIC_FIELDS_01(value_);

        const static ApplicationVersion Invalid;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {   
            std::string format = "{0}";
            size_t index = 0;

            traceEvent.AddEventField<ServiceModel::RolloutVersion>(format, name + ".rolloutVersion", index);
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<ServiceModel::RolloutVersion>(value_);
        }

    private:
        RolloutVersion value_;
    };
}
