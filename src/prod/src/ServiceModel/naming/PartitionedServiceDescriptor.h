// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    // Will contain arguments needed for creating a new partitioned reliable service
    class PartitionedServiceDescriptor : public Serialization::FabricSerializable
    {
    public:
        PartitionedServiceDescriptor();

        PartitionedServiceDescriptor(
            PartitionedServiceDescriptor const & other,
            std::wstring && nameOverride);

        PartitionedServiceDescriptor(
            PartitionedServiceDescriptor const & other,
            std::vector<byte> && initializationDataOverride);

        PartitionedServiceDescriptor(
            PartitionedServiceDescriptor const & other,
            std::wstring && nameOverride,
            ServiceModel::ServiceTypeIdentifier && typeOverride,
            std::vector<byte> && initializationDataOverride,
            std::vector<Reliability::ServiceLoadMetricDescription> && loadMetricOverride,
            uint defaultMoveCostOverride);

        PartitionedServiceDescriptor(
            PartitionedServiceDescriptor const & other);

        PartitionedServiceDescriptor(
            PartitionedServiceDescriptor && other);

        static Common::ErrorCode Create(
            Reliability::ServiceDescription const &,
            std::vector<Reliability::ConsistencyUnitDescription> const &,
            __out PartitionedServiceDescriptor &);

        static Common::ErrorCode Create(
            Reliability::ServiceDescription const & service,
            __int64 minPartitionIdInclusive,
            __int64 maxPartitionIdInclusive,
            __out PartitionedServiceDescriptor &);

        static Common::ErrorCode Create(
            Reliability::ServiceDescription const & service,
            __out PartitionedServiceDescriptor &);

        static Common::ErrorCode Create(
            Reliability::ServiceDescription const & service,
            std::vector<std::wstring> const & partitionNames,
            __out PartitionedServiceDescriptor &);

        __declspec(property(get=get_Service)) Reliability::ServiceDescription const & Service;
        Reliability::ServiceDescription const & get_Service() const { return service_; }

        __declspec(property(get=get_MutableService, put=set_MutableService)) Reliability::ServiceDescription & MutableService;
        Reliability::ServiceDescription & get_MutableService() { return service_; }
        void set_MutableService(Reliability::ServiceDescription const & value) { service_ = value; }

        __declspec(property(get=get_IsServiceGroup, put=set_IsServiceGroup)) bool IsServiceGroup;
        bool get_IsServiceGroup() const { return service_.IsServiceGroup; }
        void set_IsServiceGroup(bool isServiceGroup) { service_.IsServiceGroup = isServiceGroup; }

        __declspec(property(get=get_IsPlacementConstraintsValid)) bool IsPlacementConstraintsValid;
        bool get_IsPlacementConstraintsValid() const { return isPlacementConstraintsValid_; }

        __declspec(property(get=get_PartitionScheme)) FABRIC_PARTITION_SCHEME PartitionScheme;
        FABRIC_PARTITION_SCHEME get_PartitionScheme() const { return partitionScheme_; }
        void Test_SetPartitionScheme(FABRIC_PARTITION_SCHEME value) { partitionScheme_ = value; }

        __declspec(property(get=get_LowRange)) __int64 LowRange;
        __int64 get_LowRange() const { return lowRange_; }
        void Test_SetLowRange(__int64 value) { lowRange_ = value; }

        __declspec(property(get=get_HighRange)) __int64 HighRange;
        __int64 get_HighRange() const { return highRange_; }
        void Test_SetHighRange(__int64 value) { highRange_ = value; }

        __declspec(property(get=get_Names)) std::vector<std::wstring> const & Names;
        std::vector<std::wstring> const & get_Names() const { return partitionNames_; }
        void Test_SetNames(std::vector<std::wstring> && value) { partitionNames_ = std::move(value); }

        __declspec(property(get=get_Cuids)) std::vector<Reliability::ConsistencyUnitId> const & Cuids;
        std::vector<Reliability::ConsistencyUnitId> const & get_Cuids() const { return cuids_; }

        __declspec(property(get=get_Version, put=set_Version)) __int64 Version;
        __int64 get_Version() const { return version_; }
        void set_Version(__int64 value) { version_ = value; }

        __declspec(property(get=get_IsPlacementPolicyValid)) bool IsPlacementPolicyValid;
        bool get_IsPlacementPolicyValid() const { return isPlacementPolicyValid_; }

        void Test_UpdateCuids(std::vector<Reliability::ConsistencyUnitId> && cuids) { this->UpdateCuids(std::move(cuids)); }

        PartitionedServiceDescriptor & operator = (PartitionedServiceDescriptor const & other);
        PartitionedServiceDescriptor & operator = (PartitionedServiceDescriptor && other);

        bool operator == (PartitionedServiceDescriptor const & other) const;
        bool operator != (PartitionedServiceDescriptor const & other) const;

        Common::ErrorCode Equals(PartitionedServiceDescriptor const & other) const;

        Common::ErrorCode Validate() const;
        Common::ErrorCode ValidateSkipName() const;
        static Common::ErrorCode IsUpdateServiceAllowed(
            Naming::PartitionedServiceDescriptor const & origin,
            Naming::PartitionedServiceDescriptor const & target,
            Common::StringLiteral const &,
            std::wstring const &);

        // Note: The Public API struct FABRIC_SERVICE_DESCRIPTION does not contain CUID information. This means converting back and forth
        // across that barrier will result in different CUIDs.
        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_DESCRIPTION & serviceDescription) const;
        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_DESCRIPTION const & serviceDescription);
        Common::ErrorCode FromWrapper(ServiceModel::PartitionedServiceDescWrapper const& psdWrapper);
        void ToWrapper(ServiceModel::PartitionedServiceDescWrapper &psdWrapper) const;

        bool ServiceContains(PartitionKey const &) const;
        bool ServiceContains(PartitionInfo const &) const;
        bool ServiceContains(Reliability::ConsistencyUnitId const &) const;
        bool TryGetCuid(PartitionKey const &, __out Reliability::ConsistencyUnitId &) const;
        bool TryGetPartitionIndex(PartitionKey const &, __out int & partitionIndex) const;

        // For tooling usage - prefer instance functions in product code
        //
        static bool TryGetPartitionRange( __int64 serviceLowKey, __int64 serviceHighKey, int numPartitions, int partitionIndex, __out __int64 & low, __out __int64 & high);
        static bool TryGetPartitionIndex( __int64 serviceLowKey, __int64 serviceHighKey, int numPartitions, __int64 key, __out int & partitionIndex);

        bool TryGetPartitionInfo(Reliability::ConsistencyUnitId const &, __out PartitionInfo &) const;
        PartitionInfo GetPartitionInfo(Reliability::ConsistencyUnitId const &) const;

        FABRIC_PARTITION_KEY_TYPE GetPartitionKeyType() const;

        void SetPlacementConstraints(std::wstring &&);
        void SetPlacementPolicies(std::vector<ServiceModel::ServicePlacementPolicyDescription> &&);
        void SetLoadMetrics(std::vector<Reliability::ServiceLoadMetricDescription> &&);
        void GenerateRandomCuids();

        std::vector<Reliability::ConsistencyUnitDescription> CreateConsistencyUnitDescriptions() const;
        Common::ErrorCode AddNamedPartitions(std::vector<std::wstring> const &, __out std::vector<Reliability::ConsistencyUnitDescription> &);
        Common::ErrorCode RemoveNamedPartitions(std::vector<std::wstring> const &, __out std::vector<Reliability::ConsistencyUnitDescription> &);

        void SortNamesForUpgradeDiff();

        void WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_09(
            service_, 
            isPlacementConstraintsValid_, 
            partitionScheme_, 
            cuids_, 
            version_, 
            partitionNames_, 
            lowRange_, 
            highRange_, 
            isPlacementPolicyValid_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(service_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(cuids_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionNames_)
        END_DYNAMIC_SIZE_ESTIMATION()

    protected:
        PartitionedServiceDescriptor(
            Reliability::ServiceDescription service,
            __int64 version);

    private:
        template <typename TKey>
        class PartitionIndexCache
        {
            DENY_COPY(PartitionIndexCache)

        public:
            PartitionIndexCache();

            bool ContainsKey(TKey const &, std::vector<TKey> const &);
            bool TryGetPartitionIndex(TKey const &, std::vector<TKey> const &, __out size_t &);

        private:
            std::map<TKey, size_t> indexCache_;
            Common::RwLock indexCacheLock_;
        };

        ////////////////////////////////
        // Test constructors
        //
        PartitionedServiceDescriptor(
            Reliability::ServiceDescription const & service,
            __int64 minPartitionIdInclusive,
            __int64 maxPartitionIdInclusive,
            __int64 version = -1);

        PartitionedServiceDescriptor(
            Reliability::ServiceDescription const & service,
            __int64 version = -1);

        PartitionedServiceDescriptor(
            Reliability::ServiceDescription const & service,
            std::vector<std::wstring> const & partitionNames,
            __int64 version = -1);
        //
        ////////////////////////////////

        Common::ErrorCode ValidateServiceParameters(
            std::wstring const & serviceName,
            std::wstring const & placementConstraints,
            std::vector<Reliability::ServiceLoadMetricDescription> const & metrics,
            std::vector<Reliability::ServiceCorrelationDescription> const & correlations,
            vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies,
            std::wstring const & applicationName,
            ServiceModel::ServiceTypeIdentifier const &,
            int partitionCount,
            int targetReplicaSetSize,
            int minReplicaSetSize,
            bool isStateful,
            FABRIC_PARTITION_SCHEME scheme,
            __int64 lowRange,
            __int64 highRange,
            std::vector<std::wstring> const & partitionNames) const;

        Common::ErrorCode ValidateServiceScalingPolicy(
            std::vector<Reliability::ServiceLoadMetricDescription> const & metrics,
            vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies,
            bool isStateful,
            FABRIC_PARTITION_SCHEME scheme,
            std::vector<std::wstring> const & partitionNames) const;

        Common::ErrorCode ValidateServiceMetrics(
            std::vector<Reliability::ServiceLoadMetricDescription> const & metrics, 
            bool isStateful) const;

        Common::ErrorCode ValidateServiceCorrelations(
            std::vector<Reliability::ServiceCorrelationDescription> const & correlations, 
            std::wstring const& currentServiceName, 
            int targetReplicaSetSize) const;

        static Common::ErrorCode TraceAndGetInvalidArgumentDetails(std::wstring &&);
        static Common::ErrorCode TraceAndGetErrorDetails(Common::ErrorCodeValue::Enum, std::wstring &&);
        
        void GetPartitionRange(int partitionIndex, __out __int64 & low, __out __int64 & high) const;

        static bool ArePartitionNamesValid(std::vector<std::wstring> const & partitionNames);

        void UpdateCuids(std::vector<Reliability::ConsistencyUnitId> &&);

        Reliability::ServiceDescription service_;
        bool isPlacementConstraintsValid_;  // Flags whether placement constraints parameter was NULL when converting FromPublicApi()
        bool isPlacementPolicyValid_;  // Flags whether placement policy parameter was NULL when converting FromPublicApi()

        FABRIC_PARTITION_SCHEME partitionScheme_;
        std::vector<Reliability::ConsistencyUnitId> cuids_;
        __int64 version_;

        std::vector<std::wstring> partitionNames_;
        __int64 lowRange_;
        __int64 highRange_;

        // Cache index mapping for CUIDs and partition names to avoid linear
        // search on every partition index lookup operation. Ideally, the CUIDs
        // and partition names would already be stored in maps, but we must
        // keep the original vector representation for backwards compatibility.
        //
        // These caches are populated lazily to avoid running custom logic
        // during deserialization of this object from persisted storage.
        //
        // shared_ptr instead of unique_ptr so that the caches themselves can actually be
        // shared if PSDs are copied.
        //
        std::shared_ptr<PartitionIndexCache<Reliability::ConsistencyUnitId>> cuidsIndexCache_;
        std::shared_ptr<PartitionIndexCache<std::wstring>> partitionNamesIndexCache_;
    };

    typedef std::shared_ptr<PartitionedServiceDescriptor> PartitionedServiceDescriptorSPtr;
}
