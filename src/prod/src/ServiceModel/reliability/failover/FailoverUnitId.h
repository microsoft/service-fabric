// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct FailoverUnitId 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        FailoverUnitId()
            : guid_(ConsistencyUnitId::CreateNonreservedGuid())
        {
        }

        explicit FailoverUnitId(Common::Guid guid)
            : guid_(guid)
        {
        }

        FailoverUnitId(FailoverUnitId const& other)
            : guid_(other.guid_)
        {
        }

        __declspec(property(get=get_IsFM)) bool IsFM;
        bool get_IsFM() const;

        __declspec (property(get=get_IsInvalid)) bool IsInvalid;
        bool get_IsInvalid() const { return guid_ == Common::Guid::Empty(); }

        _declspec (property(get=get_Guid)) Common::Guid const & Guid;
        Common::Guid const & get_Guid() const { return guid_; }

        bool operator == (FailoverUnitId const& other) const
        {
            return (guid_ == other.guid_);
        }

        bool operator == (Common::Guid const & other) const
        {
            return guid_ == other;
        }

        bool operator != (FailoverUnitId const& other) const
        {
            return !(*this == other);
        }

        bool operator != (Common::Guid const & other) const
        {
            return !(*this == other);
        }

        bool operator < (FailoverUnitId const& other) const
        {
            return (guid_ < other.guid_);
        }

        bool operator >= (FailoverUnitId const& other) const
        {
            return !(*this < other);
        }

        bool operator > (FailoverUnitId const& other) const
        {
            return (other < *this);
        }

        bool operator <= (FailoverUnitId const& other) const
        {
            return !(*this > other);
        }

        std::wstring FailoverUnitId::ToString() const
        {
            return guid_.ToString();
        }

        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
        {
            writer.Write("{0}", guid_);
        }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {            
            traceEvent.AddField<Common::Guid>(name + ".guid");            
            return "{0}";
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<Common::Guid>(guid_);            
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Id, guid_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(guid_);

    private:
        Common::Guid guid_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::FailoverUnitId);
