// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    typedef std::map<Reliability::ConsistencyUnitId, ServiceLocationVersion> PartitionVersionMap;
        
    class ServiceLocationNotificationRequestData 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        ServiceLocationNotificationRequestData();

        ServiceLocationNotificationRequestData(
            Common::NamingUri const & name,
            PartitionVersionMap const & previousResolves,
            AddressDetectionFailureSPtr const & previousError,
            PartitionKey const & partitionKey);

        ServiceLocationNotificationRequestData(ServiceLocationNotificationRequestData const & other);
        ServiceLocationNotificationRequestData & operator = (ServiceLocationNotificationRequestData const & other);

        ServiceLocationNotificationRequestData(ServiceLocationNotificationRequestData && other);
        ServiceLocationNotificationRequestData & operator = (ServiceLocationNotificationRequestData && other);

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;   
        inline Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_PreviousResolves)) PartitionVersionMap const & PreviousResolves;
        inline PartitionVersionMap const & get_PreviousResolves() const { return previousResolves_; }

        __declspec(property(get=get_PreviousError)) Common::ErrorCodeValue::Enum PreviousError;   
        inline Common::ErrorCodeValue::Enum get_PreviousError() const { return previousError_; }

        __declspec(property(get=get_PreviousErrorVersion)) __int64 PreviousErrorVersion;   
        inline __int64 get_PreviousErrorVersion() const { return previousErrorVersion_; }

        __declspec(property(get=get_IsWholeService)) bool IsWholeService;
        inline bool get_IsWholeService() const { return partitionKey_.IsWholeService; }

        __declspec(property(get=get_Partition)) PartitionKey const & Partition; 
        inline PartitionKey const & get_Partition() const { return partitionKey_; }

        NameRangeTupleUPtr GetNameRangeTuple() const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_05(name_, previousResolves_, previousError_, previousErrorVersion_, partitionKey_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(previousResolves_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionKey_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::NamingUri name_;
        PartitionVersionMap previousResolves_;
        Common::ErrorCodeValue::Enum previousError_;
        __int64 previousErrorVersion_;
        PartitionKey partitionKey_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Naming::ServiceLocationNotificationRequestData);
DEFINE_USER_MAP_UTILITY(Reliability::ConsistencyUnitId, Naming::ServiceLocationVersion);
