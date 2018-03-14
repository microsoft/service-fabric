// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolvedServicePartition 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        ResolvedServicePartition();
       
        ResolvedServicePartition(
            bool isServiceGroup,
            Reliability::ServiceTableEntry const & locations, 
            PartitionInfo const & partitionData,
            Reliability::GenerationNumber const & generation,
            __int64 storeVersion,
            PartitionedServiceDescriptorSPtr const &);

        ResolvedServicePartition(
            ResolvedServicePartition const & other,
            Reliability::ServiceTableEntry && locations);

        ResolvedServicePartition(ResolvedServicePartition const & other);

        ResolvedServicePartition(ResolvedServicePartition && other);

        ResolvedServicePartition & operator = (ResolvedServicePartition const & other);

        ResolvedServicePartition & operator = (ResolvedServicePartition && other);
        
        bool operator == (ResolvedServicePartition const & other) const;
        bool operator != (ResolvedServicePartition const & other) const;

        bool TryGetName(__out Common::NamingUri & name) const;

        __declspec(property(get=get_Locations)) Reliability::ServiceTableEntry const & Locations;
        inline Reliability::ServiceTableEntry const & get_Locations() const { return locations_; }

        __declspec(property(get=get_PartitionData)) PartitionInfo PartitionData;
        inline PartitionInfo get_PartitionData() const { return partitionData_; }

        __declspec(property(get=get_FMVersion)) __int64 FMVersion;
        inline __int64 get_FMVersion() const { return locations_.Version; }
        
        __declspec(property(get=get_Generation)) Reliability::GenerationNumber const & Generation;
        inline Reliability::GenerationNumber get_Generation() const { return generation_; }

        __declspec(property(get=get_Psd)) PartitionedServiceDescriptorSPtr const & Psd;
        inline PartitionedServiceDescriptorSPtr const & get_Psd() const { return psd_; }

        __declspec(property(get=get_StoreVersion)) __int64 StoreVersion;
        inline __int64 get_StoreVersion() const { return storeVersion_; }

        __declspec(property(get=get_IsServiceGroup)) bool IsServiceGroup;
        inline bool get_IsServiceGroup() const { return isServiceGroup_; }

        __declspec(property(get=get_IsStateful)) bool IsStateful;
        inline bool get_IsStateful() const { return locations_.ServiceReplicaSet.IsStateful; }

        size_t GetEndpointCount() const;

        Common::ErrorCode CompareVersion(
            std::shared_ptr<ResolvedServicePartition> const & other,
            __out LONG & compareResult) const;

        Common::ErrorCode CompareVersion(
            ResolvedServicePartition const &other,
            __out LONG & compareResult) const;

        Common::ErrorCode CompareVersion(
            ResolvedServicePartitionMetadata const &metadata,
            __out LONG & compareResult) const;

        LONG CompareVersionNoValidation(
            ResolvedServicePartition const &other) const;
            
        void ToPublicApi(
            __inout Common::ScopedHeap & heap, 
            __out FABRIC_RESOLVED_SERVICE_PARTITION & resolvedServicePartition) const;

        void ToWrapper(
            __out ResolvedServicePartitionWrapper & rspWrapper);

        bool IsMoreRecent(ServiceLocationVersion const & prev);
        
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        std::wstring ToString() const;
        FABRIC_FIELDS_06(isServiceGroup_, locations_, partitionData_, storeVersion_, generation_, psd_);

        // GenerationNumber contains only int64 and NodeId, and both have 0 dynamic serialization
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(locations_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionData_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(psd_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        LONG CompareVersionNoValidation(
            Reliability::GenerationNumber const &generation,
            __int64 FMVersion) const;

        bool isServiceGroup_;
        Reliability::ServiceTableEntry locations_;
        PartitionInfo partitionData_;
        __int64 storeVersion_;
        Reliability::GenerationNumber generation_;
        PartitionedServiceDescriptorSPtr psd_;
    };
    
    typedef std::shared_ptr<ResolvedServicePartition> ResolvedServicePartitionSPtr;
}

DEFINE_USER_ARRAY_UTILITY(Naming::ResolvedServicePartition);
