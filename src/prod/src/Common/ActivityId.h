// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ActivityId
        : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(ActivityId)

    public:

        struct Hasher
        {
            size_t operator() (ActivityId const & activityId) const 
            { 
                return activityId.Guid.GetHashCode();
            }

            bool operator() (ActivityId const & left, ActivityId const & right) const
            { 
                return (left == right);
            }
        };

    public:
        ActivityId();
        explicit ActivityId(Common::Guid const &);
        explicit ActivityId(Common::Guid &&);
        ActivityId(ActivityId const &);
        ActivityId(ActivityId &&);

        // Creates an activity id with the same GUID as passed in activity id,
        // and index shifted with shiftIndex from the passed in activity id index.
        ActivityId(ActivityId const &, uint64 shiftIndex);
        
        __declspec(property(get=get_Guid)) Common::Guid const & Guid;
        Common::Guid const & get_Guid() const { return guid_; }

        __declspec(property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return guid_ == Guid::Empty(); }

        __declspec(property(get=get_Index)) uint64 Index;
        uint64 get_Index() const { return index_; }

        bool operator < (ActivityId const &) const;
        bool operator == (ActivityId const &) const;
        bool operator <= (ActivityId const &) const;
        bool operator != (ActivityId const &) const;
        bool operator > (ActivityId const &) const;
        bool operator >= (ActivityId const &) const;

        void WriteTo(TextWriter& w, FormatOptions const& f) const;
        std::wstring ToString() const;

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;

        void IncrementIndex();

        ActivityId GetNestedActivity() const;

        static ActivityId Empty;

        FABRIC_FIELDS_02(guid_, index_);

    private:
        Common::Guid guid_;
        uint64 index_;
    };
}
