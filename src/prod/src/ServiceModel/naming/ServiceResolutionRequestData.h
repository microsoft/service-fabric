// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceResolutionRequestData : public Serialization::FabricSerializable
    {
    public:
        ServiceResolutionRequestData();
        explicit ServiceResolutionRequestData(PartitionKey const & key);
        explicit ServiceResolutionRequestData(PartitionInfo const & existingData);

        ServiceResolutionRequestData(
            PartitionKey const & key, 
            ServiceLocationVersion const & version);

        ServiceResolutionRequestData(
            PartitionKey const & key, 
            ServiceLocationVersion const & version,
            Reliability::ConsistencyUnitId const &);

        ServiceResolutionRequestData(ServiceResolutionRequestData const & other);    
        ServiceResolutionRequestData & ServiceResolutionRequestData::operator = (ServiceResolutionRequestData const & other);
                
        
        __declspec(property(get=get_Key)) PartitionKey const & Key;
        inline PartitionKey get_Key() const { return key_; }
        
        __declspec(property(get=get_Version, put=put_Version)) ServiceLocationVersion const & Version;
        inline ServiceLocationVersion get_Version() const { return version_; }
        inline void put_Version(ServiceLocationVersion value) { version_ = value; }

        __declspec(property(get=get_Cuid)) Reliability::ConsistencyUnitId const & Cuid;
        inline Reliability::ConsistencyUnitId const & get_Cuid() const { return cuid_; }

        __declspec(property(get=get_IncludePsd)) bool IncludePsd;
        inline bool get_IncludePsd() const { return includePsd_; }

        void SetIncludePsd(bool value);

        void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_04(key_, version_, cuid_, includePsd_);

    private:
        PartitionKey key_;

        // Version information for previous resolution result (if any)
        //
        ServiceLocationVersion version_;
        Reliability::ConsistencyUnitId cuid_;

        // Whether or not to include PSD in reply
        // (as opposed to endpoints only).
        //
        bool includePsd_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Naming::ServiceResolutionRequestData);
