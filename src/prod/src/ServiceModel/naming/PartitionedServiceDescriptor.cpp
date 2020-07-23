// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace Reliability;

StringLiteral const TraceComponent("PartitionedServiceDescriptor");

namespace Naming
{
    INITIALIZE_SIZE_ESTIMATION(PartitionedServiceDescriptor)

    PartitionedServiceDescriptor::PartitionedServiceDescriptor()
      : service_()
      , isPlacementConstraintsValid_(false)
      , partitionScheme_(FABRIC_PARTITION_SCHEME_INVALID)
      , cuids_()
      , version_(Constants::UninitializedVersion)
      , partitionNames_()
      , lowRange_(Constants::UninitializedVersion)
      , highRange_(Constants::UninitializedVersion)
      , isPlacementPolicyValid_(false)
      , cuidsIndexCache_(make_shared<PartitionIndexCache<ConsistencyUnitId>>())
      , partitionNamesIndexCache_(make_shared<PartitionIndexCache<wstring>>())
    {
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        Reliability::ServiceDescription service,
        __int64 version)
      : service_(service)
      , isPlacementConstraintsValid_(true)
      , partitionScheme_(FABRIC_PARTITION_SCHEME_INVALID)
      , cuids_()
      , version_(version)
      , partitionNames_()
      , lowRange_(0)
      , highRange_(0)
      , isPlacementPolicyValid_(true)
      , cuidsIndexCache_(make_shared<PartitionIndexCache<ConsistencyUnitId>>())
      , partitionNamesIndexCache_(make_shared<PartitionIndexCache<wstring>>())
    {
        GenerateRandomCuids();
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        PartitionedServiceDescriptor const & other,
        wstring && nameOverride)
      : service_(other.service_, move(nameOverride))
      , isPlacementConstraintsValid_(other.isPlacementConstraintsValid_)
      , partitionScheme_(other.partitionScheme_)
      , lowRange_(other.lowRange_)
      , highRange_(other.highRange_)
      , partitionNames_(other.partitionNames_)
      , cuids_(other.cuids_)
      , version_(other.version_)
      , isPlacementPolicyValid_(other.isPlacementPolicyValid_)
      , cuidsIndexCache_(other.cuidsIndexCache_)
      , partitionNamesIndexCache_(other.partitionNamesIndexCache_)
    {
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        PartitionedServiceDescriptor const & other,
        std::vector<byte> && initializationDataOverride)
      : service_(other.service_, move(initializationDataOverride))
      , isPlacementConstraintsValid_(other.isPlacementConstraintsValid_)
      , partitionScheme_(other.partitionScheme_)
      , lowRange_(other.lowRange_)
      , highRange_(other.highRange_)
      , partitionNames_(other.partitionNames_)
      , cuids_(other.cuids_)
      , version_(other.version_)
      , isPlacementPolicyValid_(other.isPlacementPolicyValid_)
      , cuidsIndexCache_(other.cuidsIndexCache_)
      , partitionNamesIndexCache_(other.partitionNamesIndexCache_)
    {
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        PartitionedServiceDescriptor const & other,
        wstring && nameOverride,
        ServiceModel::ServiceTypeIdentifier && typeOverride,
        vector<byte> && initializationDataOverride,
        std::vector<Reliability::ServiceLoadMetricDescription> && loadMetricOverride,
        uint defaultMoveCostOverride)
      : service_(other.service_, move(nameOverride), move(typeOverride), move(initializationDataOverride), move(loadMetricOverride), defaultMoveCostOverride)
      , isPlacementConstraintsValid_(other.isPlacementConstraintsValid_)
      , partitionScheme_(other.partitionScheme_)
      , lowRange_(other.lowRange_)
      , highRange_(other.highRange_)
      , partitionNames_(other.partitionNames_)
      , cuids_(other.cuids_)
      , version_(other.version_)
      , isPlacementPolicyValid_(other.isPlacementPolicyValid_)
      , cuidsIndexCache_(other.cuidsIndexCache_)
      , partitionNamesIndexCache_(other.partitionNamesIndexCache_)
    {
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(PartitionedServiceDescriptor const & other)
      : service_(other.service_)
      , isPlacementConstraintsValid_(other.isPlacementConstraintsValid_)
      , partitionScheme_(other.partitionScheme_)
      , lowRange_(other.lowRange_)
      , highRange_(other.highRange_)
      , partitionNames_(other.partitionNames_)
      , cuids_(other.cuids_)
      , version_(other.version_)
      , isPlacementPolicyValid_(other.isPlacementPolicyValid_)
      , cuidsIndexCache_(other.cuidsIndexCache_)
      , partitionNamesIndexCache_(other.partitionNamesIndexCache_)
    {
    }


    PartitionedServiceDescriptor::PartitionedServiceDescriptor(PartitionedServiceDescriptor && other)
      : service_(move(other.service_))
      , isPlacementConstraintsValid_(move(other.isPlacementConstraintsValid_))
      , partitionScheme_(move(other.partitionScheme_))
      , lowRange_(move(other.lowRange_))
      , highRange_(move(other.highRange_))
      , partitionNames_(move(other.partitionNames_))
      , cuids_(move(other.cuids_))
      , version_(move(other.version_))
      , isPlacementPolicyValid_(move(other.isPlacementPolicyValid_))
      , cuidsIndexCache_(move(other.cuidsIndexCache_))
      , partitionNamesIndexCache_(move(other.partitionNamesIndexCache_))
    {
    }

    PartitionedServiceDescriptor & PartitionedServiceDescriptor::operator = (PartitionedServiceDescriptor const & other)
    {
        if (this != &other)
        {
            service_ = other.service_;
            isPlacementConstraintsValid_ = other.isPlacementConstraintsValid_;
            partitionScheme_ = other.partitionScheme_;
            lowRange_ = other.lowRange_;
            highRange_ = other.highRange_;
            partitionNames_ = other.partitionNames_;
            cuids_ = other.cuids_;
            version_ = other.version_;
            isPlacementPolicyValid_ = other.isPlacementPolicyValid_;
            cuidsIndexCache_ = other.cuidsIndexCache_;
            partitionNamesIndexCache_ = other.partitionNamesIndexCache_;
        }
        return *this;
    }

    PartitionedServiceDescriptor & PartitionedServiceDescriptor::operator = (PartitionedServiceDescriptor && other)
    {
        if (this != &other)
        {
            service_ = move(other.service_);
            isPlacementConstraintsValid_ = move(other.isPlacementConstraintsValid_);
            partitionScheme_ = move(other.partitionScheme_);
            lowRange_ = move(other.lowRange_);
            highRange_ = move(other.highRange_);
            partitionNames_ = move(other.partitionNames_);
            cuids_ = move(other.cuids_);
            version_ = move(other.version_);
            isPlacementPolicyValid_ = move(other.isPlacementPolicyValid_);
            cuidsIndexCache_ = move(other.cuidsIndexCache_);
            partitionNamesIndexCache_ = move(other.partitionNamesIndexCache_);
        }
        return *this;
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        Reliability::ServiceDescription const & service,
        __int64 minPartitionIdInclusive,
        __int64 maxPartitionIdInclusive,
        __int64 version)
      : service_(service)
      , isPlacementConstraintsValid_(true)
      , partitionScheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
      , lowRange_(minPartitionIdInclusive)
      , highRange_(maxPartitionIdInclusive)
      , partitionNames_()
      , cuids_()
      , version_(version)
      , isPlacementPolicyValid_(true)
      , cuidsIndexCache_(make_shared<PartitionIndexCache<ConsistencyUnitId>>())
      , partitionNamesIndexCache_(make_shared<PartitionIndexCache<wstring>>())
    {
        GenerateRandomCuids();
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        Reliability::ServiceDescription const & service,
        __int64 version)
      : service_(service)
      , isPlacementConstraintsValid_(true)
      , partitionScheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON)
      , partitionNames_()
      , lowRange_(0)
      , highRange_(0)
      , cuids_()
      , version_(version)
      , isPlacementPolicyValid_(true)
      , cuidsIndexCache_(make_shared<PartitionIndexCache<ConsistencyUnitId>>())
      , partitionNamesIndexCache_(make_shared<PartitionIndexCache<wstring>>())
    {
        GenerateRandomCuids();
    }

    PartitionedServiceDescriptor::PartitionedServiceDescriptor(
        Reliability::ServiceDescription const & service,
        vector<wstring> const & partitionNames,
        __int64 version)
      : service_(service)
      , isPlacementConstraintsValid_(true)
      , partitionScheme_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED)
      , partitionNames_(partitionNames)
      , lowRange_(0)
      , highRange_(0)
      , cuids_()
      , version_(version)
      , isPlacementPolicyValid_(true)
      , cuidsIndexCache_(make_shared<PartitionIndexCache<ConsistencyUnitId>>())
      , partitionNamesIndexCache_(make_shared<PartitionIndexCache<wstring>>())
    {
        GenerateRandomCuids();
    }

    ErrorCode PartitionedServiceDescriptor::Create(
        ServiceDescription const & serviceDescription,
        vector<ConsistencyUnitDescription> const & inputDescriptions,
        __out PartitionedServiceDescriptor & result)
    {
        if (inputDescriptions.empty())
        {
            Trace.WriteWarning(TraceComponent, "List of CU descriptions cannot be empty");
            return ErrorCodeValue::InvalidArgument;
        }

        if (inputDescriptions.size() != static_cast<size_t>(serviceDescription.PartitionCount))
        {
            Trace.WriteWarning(
                TraceComponent, 
                "Number of CU descriptions does not match partition count: {0} != {1}",
                inputDescriptions.size(),
                serviceDescription.PartitionCount);
            return ErrorCodeValue::InvalidArgument;
        }


        // A list of INT64_RANGE partitions must be ordered according to their ranges for TryGetCuid() to work
        // properly, so we sort the CU descriptions before building the list. This protects against rebuilding
        // from a list of unsorted CUIDs returned by the FM when repairing or recovering from Naming data loss.
        //
        vector<ConsistencyUnitDescription> cuDescriptions = inputDescriptions;
        if (cuDescriptions[0].PartitionKind == FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE)
        {
            sort(cuDescriptions.begin(), cuDescriptions.end(), ConsistencyUnitDescription::CompareForSort);
        }

        __int64 lowRange(MAXINT64);
        __int64 highRange(MININT64);
        vector<wstring> partitionNames;
        vector<ConsistencyUnitId> cuids;

        FABRIC_SERVICE_PARTITION_KIND partitionKind = cuDescriptions[0].PartitionKind;

        for (auto iter = cuDescriptions.begin(); iter != cuDescriptions.end(); ++iter)
        {
            ConsistencyUnitDescription const & cu = (*iter);
            if (cu.PartitionKind != partitionKind)
            {
                Trace.WriteWarning(
                    TraceComponent, 
                    "Partition key type mismatch: [{0}, {1}] != [{2}, {3}]",
                    cuDescriptions[0].ConsistencyUnitId,
                    static_cast<int>(partitionKind),
                    cu.ConsistencyUnitId,
                    static_cast<int>(cu.PartitionKind));

                return ErrorCodeValue::InvalidArgument;
            }
        
            switch (cu.PartitionKind)
            {
                case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                    break;

                case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                    if (cu.LowKey < lowRange)
                    {
                        lowRange = cu.LowKey;
                    }

                    if (cu.HighKey > highRange)
                    {
                        highRange = cu.HighKey;
                    }
                    break;

                case FABRIC_SERVICE_PARTITION_KIND_NAMED:
                    partitionNames.push_back(cu.PartitionName);
                    break;

                default:
                    Trace.WriteWarning(
                        TraceComponent, 
                        "Unrecognized partition kind {0}",
                        static_cast<int>(cu.PartitionKind));
                    return ErrorCodeValue::InvalidArgument;
            }

            cuids.push_back(cu.ConsistencyUnitId);
        }

        PartitionedServiceDescriptor psd;
        
        switch (cuDescriptions[0].PartitionKind)
        {
            case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                psd = PartitionedServiceDescriptor(serviceDescription);
                break;

            case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                psd = PartitionedServiceDescriptor(serviceDescription, lowRange, highRange);
                break;

            case FABRIC_SERVICE_PARTITION_KIND_NAMED:
                psd = PartitionedServiceDescriptor(serviceDescription, partitionNames);
                break;

            default:
                // assert instead of returning error since we should have already
                // validated the scheme when recovering the CUIDs
                //
                Assert::CodingError(
                    "Unrecognized partition kind {0}",
                    static_cast<int>(cuDescriptions[0].PartitionKind));
        }

        psd.UpdateCuids(move(cuids));

        auto error = psd.Validate();
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceComponent, 
                "Unable to validate PSD [{0}]",
                psd);
            return error;
        }

        result = std::move(psd);

        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::Create(
        ServiceDescription const & service,
        __int64 minPartitionIdInclusive,
        __int64 maxPartitionIdInclusive,
        __out PartitionedServiceDescriptor & result)
    {
        PartitionedServiceDescriptor psd(service, minPartitionIdInclusive, maxPartitionIdInclusive);

        auto error = psd.ValidateSkipName();
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceComponent, 
                "Unable to validate PSD [{0}]",
                psd);
            return error;
        }

        result = move(psd);
        
        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::Create(
        ServiceDescription const & service,
        __out PartitionedServiceDescriptor & result)
    {
        PartitionedServiceDescriptor psd(service);

        auto error = psd.ValidateSkipName();
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceComponent, 
                "Unable to validate PSD [{0}]",
                psd);
            return error;
        }

        result = move(psd);

        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::Create(
        ServiceDescription const & service,
        std::vector<std::wstring> const & partitionNames,
        __out PartitionedServiceDescriptor & result)
    {
        PartitionedServiceDescriptor psd(service, partitionNames);

        auto error = psd.ValidateSkipName();
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(
                TraceComponent, 
                "Unable to validate PSD [{0}]",
                psd);
            return error;
        }

        result = move(psd);

        return ErrorCodeValue::Success;
    }

    PartitionInfo PartitionedServiceDescriptor::GetPartitionInfo(Reliability::ConsistencyUnitId const & cuid) const
    {
        PartitionInfo result;
        bool success = this->TryGetPartitionInfo(cuid, result);
        if (!success)
        {
            Assert::CodingError("TryGetPartitionInfo() failed");
        }

        return result;
    }

    bool PartitionedServiceDescriptor::TryGetPartitionInfo(
        Reliability::ConsistencyUnitId const & cuid,
        __out PartitionInfo & result) const
    {
        switch (partitionScheme_)
        {
        case FABRIC_PARTITION_SCHEME_SINGLETON:
            result = PartitionInfo();
            return true;

        case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
        {
            size_t indexOfCuid;
            bool success = cuidsIndexCache_->TryGetPartitionIndex(cuid, cuids_, indexOfCuid);
            if (success)
            {
                __int64 partitionLowKey;
                __int64 partitionHighKey;
                this->GetPartitionRange(static_cast<int>(indexOfCuid), partitionLowKey, partitionHighKey);
                result = PartitionInfo(partitionLowKey, partitionHighKey);
            }
            else
            {
                Trace.WriteWarning(TraceComponent, "{0} not found in CUID list {1}", cuid, cuids_);
            }
            
            return success;
        }
        case FABRIC_PARTITION_SCHEME_NAMED:            
        {
            size_t indexOfCuid;
            bool success = cuidsIndexCache_->TryGetPartitionIndex(cuid, cuids_, indexOfCuid);
            if (success)
            {
                result = PartitionInfo(partitionNames_[indexOfCuid]);
            }
            else
            {
                Trace.WriteWarning(TraceComponent, "{0} not found in CUID list {1}", cuid, cuids_);
            }
            
            return success;
        }
        default:
            Trace.WriteError(TraceComponent, "Unknown FABRIC_PARTITION_SCHEME {0}", static_cast<int>(partitionScheme_));
        }

        return false;
    }

    FABRIC_PARTITION_KEY_TYPE PartitionedServiceDescriptor::GetPartitionKeyType() const
    {
        switch (partitionScheme_)
        {
        case FABRIC_PARTITION_SCHEME_SINGLETON:
            return ::FABRIC_PARTITION_KEY_TYPE_NONE;
        case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE: 
            return ::FABRIC_PARTITION_KEY_TYPE_INT64;
        case FABRIC_PARTITION_SCHEME_NAMED:
            return ::FABRIC_PARTITION_KEY_TYPE_STRING;
        default: 
            Assert::CodingError("Unknown FABRIC_PARTITION_SCHEME {0}", static_cast<int>(partitionScheme_));
        }
    }

    bool PartitionedServiceDescriptor::ServiceContains(PartitionKey const & key) const
    {
        switch (partitionScheme_)
        {
        case FABRIC_PARTITION_SCHEME_SINGLETON:
            return key.KeyType == FABRIC_PARTITION_KEY_TYPE_NONE;

        case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE: 
            if (key.KeyType != FABRIC_PARTITION_KEY_TYPE_INT64)
            {
                return false;
            }
            return key.Int64Key >= lowRange_ && key.Int64Key <= highRange_;

        case FABRIC_PARTITION_SCHEME_NAMED:
            if (key.KeyType != FABRIC_PARTITION_KEY_TYPE_STRING)
            {
                return false;
            }
            return partitionNamesIndexCache_->ContainsKey(key.StringKey, partitionNames_);

        default:
            Assert::CodingError("Unknown FABRIC_PARTITION_SCHEME {0}", static_cast<int>(partitionScheme_));
        }
    }

    bool PartitionedServiceDescriptor::ServiceContains(PartitionInfo const & info) const
    {
        if (partitionScheme_ != info.PartitionScheme)
        {
            return false;
        }

        switch (partitionScheme_)
        {
        case FABRIC_PARTITION_SCHEME_SINGLETON:
            return true;
        case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE: 
            return (lowRange_ <= info.LowKey && info.HighKey <= highRange_);
        case FABRIC_PARTITION_SCHEME_NAMED:
            return partitionNamesIndexCache_->ContainsKey(info.PartitionName, partitionNames_);

        default:
            Assert::CodingError("Unknown FABRIC_PARTITION_SCHEME {0}", static_cast<int>(partitionScheme_));
        }
    }

    bool PartitionedServiceDescriptor::ServiceContains(ConsistencyUnitId const & cuid) const
    {
        return cuidsIndexCache_->ContainsKey(cuid, cuids_);
    }

    bool PartitionedServiceDescriptor::TryGetCuid(
        PartitionKey const & key, 
        __out Reliability::ConsistencyUnitId & cuid) const
    {
        int partitionIndex;
        if (TryGetPartitionIndex(key, partitionIndex))
        {
            cuid = cuids_[partitionIndex];

            return true;
        }

        return false;
    }

    void PartitionedServiceDescriptor::WriteTo(TextWriter& writer, FormatOptions const&) const
    {
        writer.Write(
            "{0} : ({1}, {2}) {3} - [{4},{5}] {6}",
            service_,
            static_cast<int>(partitionScheme_),
            version_,
            cuids_,
            lowRange_,
            highRange_,
            partitionNames_);
    }

    void PartitionedServiceDescriptor::WriteToEtw(uint16 contextSequenceId) const
    {
        ServiceModelEventSource::Trace->PSDTrace(
            contextSequenceId,
            service_,
            static_cast<int>(partitionScheme_),
            version_,
            wformatString(cuids_),
            lowRange_,
            highRange_,
            wformatString(partitionNames_)); 
    }

    wstring PartitionedServiceDescriptor::ToString() const
    {
        wstring result;
        StringWriter(result).Write(*this);
        return result;
    }

    ErrorCode PartitionedServiceDescriptor::Validate() const
    {
        NamingUri nameUri;
        if (!NamingUri::TryParse(service_.Name, nameUri))
        {
            return TraceAndGetErrorDetails(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}", GET_NS_RC( Invalid_Uri ), 
                service_.Name));
        }
        
        return this->ValidateSkipName();
    }

    ErrorCode PartitionedServiceDescriptor::ValidateSkipName() const
    {
        return ValidateServiceParameters(
            service_.Name,
            service_.PlacementConstraints,
            service_.Metrics,
            service_.Correlations,
            service_.ScalingPolicies,
            service_.ApplicationName,
            service_.Type,
            service_.PartitionCount,
            service_.TargetReplicaSetSize,
            service_.MinReplicaSetSize,
            service_.IsStateful,
            partitionScheme_,
            lowRange_,
            highRange_,
            partitionNames_);
    }

    ErrorCode PartitionedServiceDescriptor::IsUpdateServiceAllowed(
        PartitionedServiceDescriptor const & origin,
        PartitionedServiceDescriptor const & target,
        StringLiteral const & traceComponent,
        std::wstring const & traceId)
    {
        if (target.Service.IsStateful != origin.Service.IsStateful)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} service kind doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Kind), target.Service.Name));
        }

        if (target.Service.IsStateful && target.Service.HasPersistedState != origin.Service.HasPersistedState)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} has persistent state doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Has_Persisted_State), target.Service.Name));
        }

        if (target.Service.IsServiceGroup != origin.Service.IsServiceGroup)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} is service group doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Is_Service_Group), target.Service.Name));
        }

        if (target.PartitionScheme != origin.PartitionScheme)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} partition scheme doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Partition_Scheme), target.Service.Name));
        }

        if (target.LowRange != origin.LowRange || target.HighRange != origin.HighRange)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} partition range doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Partition_Range), target.Service.Name));
        }

        // Only repartitioning of named partitions is currently supported
        //
        if (target.Service.PartitionCount != origin.Service.PartitionCount && 
            target.PartitionScheme != FABRIC_PARTITION_SCHEME_NAMED)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} partition count doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_Partition_Count), target.Service.Name));
        }

        if (target.Service.PackageActivationMode != origin.Service.PackageActivationMode)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} ServicePackageActivationMode doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument, 
                wformatString(GET_NS_RC(Update_Service_ServicePackageActivationMode), 
                    target.Service.Name));
        }

        if (target.Service.ServiceDnsName != origin.Service.ServiceDnsName)
        {
            Trace.WriteWarning(
                traceComponent,
                "{0} ServiceDnsName doesn't match: {1} -> {2}",
                traceId,
                origin,
                target);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_NS_RC(Update_Service_ServiceDnsName),
                    target.Service.Name));
        }

        return ErrorCode::Success();
    }

    // NOTE: Removes service type qualification
    void PartitionedServiceDescriptor::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SERVICE_DESCRIPTION & serviceDescription) const
    {
        ULONG const initDataSize = static_cast<const ULONG>(Service.InitializationData.size());
        auto initData = heap.AddArray<BYTE>(initDataSize);
        
        for (size_t i = 0; i < initDataSize; i++)
        {
            initData[i] = Service.InitializationData[i];
        }
        
        if (Service.IsStateful)
        {
            auto statefulDescription = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION>();

            serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
            serviceDescription.Value = statefulDescription.GetRawPointer();
            statefulDescription->ApplicationName = heap.AddString(Service.ApplicationName);
            statefulDescription->ServiceName = heap.AddString(Service.Name);
            statefulDescription->ServiceTypeName = heap.AddString(Service.Type.ServiceTypeName);
            statefulDescription->InitializationData = initData.GetRawArray();
            statefulDescription->InitializationDataSize = initDataSize;
            statefulDescription->PartitionScheme = PartitionScheme;
            statefulDescription->TargetReplicaSetSize = Service.TargetReplicaSetSize;
            statefulDescription->MinReplicaSetSize = Service.MinReplicaSetSize;
            statefulDescription->PlacementConstraints = heap.AddString(Service.PlacementConstraints);
            

            size_t correlationCount = Service.Correlations.size();
            statefulDescription->CorrelationCount = static_cast<ULONG>(correlationCount);
            if (correlationCount > 0)
            {
                auto correlations = heap.AddArray<FABRIC_SERVICE_CORRELATION_DESCRIPTION>(correlationCount);
                statefulDescription->Correlations = correlations.GetRawArray();
                for (size_t correlationIndex = 0; correlationIndex < correlationCount; ++correlationIndex)
                {
                    correlations[correlationIndex].ServiceName = heap.AddString(Service.Correlations[correlationIndex].ServiceName);
                    correlations[correlationIndex].Scheme = Service.Correlations[correlationIndex].Scheme;
                }
            }
            else
            {
                statefulDescription->Correlations = nullptr;
            }

            statefulDescription->HasPersistedState = Service.HasPersistedState;

            auto statefulDescriptionEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1>();
            statefulDescription->Reserved = statefulDescriptionEx1.GetRawPointer();

            // policy description
            size_t policyCount = Service.PlacementPolicies.size();
            if (policyCount > 0)
            {
                auto policyDescription = heap.AddItem<FABRIC_SERVICE_PLACEMENT_POLICY_LIST>();
                statefulDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
                policyDescription->PolicyCount = static_cast<ULONG>(policyCount);

                auto policies = heap.AddArray<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION>(policyCount);
                policyDescription->Policies = policies.GetRawArray();
                for (size_t policyIndex = 0; policyIndex < policyCount; ++policyIndex)
                {
                    Service.PlacementPolicies[policyIndex].ToPublicApi(heap, policies[policyIndex]);
                }
            }
            else
            {
                statefulDescriptionEx1->PolicyList = nullptr;
            }

            auto failoverSettings = heap.AddItem<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS>();
            auto failoverSettingsEx1 = heap.AddItem<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1>();
            failoverSettings->Reserved = failoverSettingsEx1.GetRawPointer();
            statefulDescriptionEx1->FailoverSettings = failoverSettings.GetRawPointer();

            if (Service.ReplicaRestartWaitDuration >= TimeSpan::Zero)
            {
                failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
                failoverSettings->ReplicaRestartWaitDurationSeconds = static_cast<DWORD>(Service.ReplicaRestartWaitDuration.TotalSeconds());
            }

            if (Service.QuorumLossWaitDuration >= TimeSpan::Zero)
            {
                failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
                failoverSettings->QuorumLossWaitDurationSeconds = static_cast<DWORD>(Service.QuorumLossWaitDuration.TotalSeconds());
            }

            if (Service.StandByReplicaKeepDuration >= TimeSpan::Zero)
            {
                failoverSettings->Flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
                failoverSettingsEx1->StandByReplicaKeepDurationSeconds = static_cast<DWORD>(Service.StandByReplicaKeepDuration.TotalSeconds());
            }

            if (PartitionScheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
            {
                auto partitionDescription = heap.AddItem<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION>();
                partitionDescription->PartitionCount = Service.PartitionCount;
                partitionDescription->LowKey = lowRange_;
                partitionDescription->HighKey = highRange_;
                partitionDescription->Reserved = NULL;
                statefulDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
            }
            else if (PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED)
            {
                auto partitionDescription = heap.AddItem<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION>();
                partitionDescription->PartitionCount = Service.PartitionCount;
                auto pNames = heap.AddArray<LPCWSTR>(Service.PartitionCount);
                for (size_t i = 0; i < static_cast<size_t>(Service.PartitionCount); ++i)
                {
                    pNames[i] = heap.AddString(partitionNames_[i]);
                }

                partitionDescription->Names = pNames.GetRawArray();
                partitionDescription->Reserved = NULL;
                statefulDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
            }
            else
            {
                statefulDescription->PartitionSchemeDescription = NULL;
            }

            size_t metricCount = Service.Metrics.size();
            statefulDescription->MetricCount = static_cast<ULONG>(metricCount);
            if (metricCount > 0)
            {
                auto metrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(metricCount);
                statefulDescription->Metrics = metrics.GetRawArray();
                for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
                {
                    metrics[metricIndex].Name = heap.AddString(Service.Metrics[metricIndex].Name);
                    metrics[metricIndex].Weight = Service.Metrics[metricIndex].Weight;
                    metrics[metricIndex].PrimaryDefaultLoad = Service.Metrics[metricIndex].PrimaryDefaultLoad;
                    metrics[metricIndex].SecondaryDefaultLoad = Service.Metrics[metricIndex].SecondaryDefaultLoad;
                }
            }
            else
            {
                statefulDescription->Metrics = nullptr;
            }

            auto statefulDescriptionEx2 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2>();
            statefulDescriptionEx1->Reserved = statefulDescriptionEx2.GetRawPointer();
            statefulDescriptionEx2->IsDefaultMoveCostSpecified = true;
            statefulDescriptionEx2->DefaultMoveCost = static_cast<FABRIC_MOVE_COST> (Service.DefaultMoveCost);

            auto statefulDescriptionEx3 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3>();
            statefulDescriptionEx2->Reserved = statefulDescriptionEx3.GetRawPointer();
            statefulDescriptionEx3->ServicePackageActivationMode = ServicePackageActivationMode::ToPublicApi(Service.PackageActivationMode);
            statefulDescriptionEx3->ServiceDnsName = heap.AddString(Service.ServiceDnsName);

	    auto statefulDescriptionEx4 = heap.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4>();
            statefulDescriptionEx3->Reserved = statefulDescriptionEx4.GetRawPointer();

            size_t scalingPolicyCount = Service.ScalingPolicies.size();
            if (scalingPolicyCount > 0)
            {
                statefulDescriptionEx4->ScalingPolicyCount = static_cast<ULONG>(scalingPolicyCount);
                auto spArray = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY>(scalingPolicyCount);
                statefulDescriptionEx4->ServiceScalingPolicies = spArray.GetRawArray();
                for (size_t spIndex = 0; spIndex < scalingPolicyCount; ++spIndex)
                {
                    Service.ScalingPolicies[spIndex].ToPublicApi(heap, spArray[spIndex]);
                }
            }
            else
            {
                statefulDescriptionEx4->ScalingPolicyCount = 0;
                statefulDescriptionEx4->ServiceScalingPolicies = nullptr;
	    }
        }
        else
        {
            auto statelessDescription = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION>();

            serviceDescription.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
            serviceDescription.Value = statelessDescription.GetRawPointer();
            statelessDescription->ApplicationName = heap.AddString(Service.ApplicationName);
            statelessDescription->ServiceName = heap.AddString(Service.Name);
            statelessDescription->ServiceTypeName = heap.AddString(Service.Type.ServiceTypeName);
            statelessDescription->InitializationData = initData.GetRawArray();
            statelessDescription->InitializationDataSize = initDataSize;
            statelessDescription->PartitionScheme = PartitionScheme;
            statelessDescription->InstanceCount = Service.TargetReplicaSetSize;
            statelessDescription->PlacementConstraints = heap.AddString(Service.PlacementConstraints);
            statelessDescription->Reserved = NULL;

            size_t correlationCount = Service.Correlations.size();
            statelessDescription->CorrelationCount = static_cast<ULONG>(correlationCount);
            if (correlationCount > 0)
            {
                auto correlations = heap.AddArray<FABRIC_SERVICE_CORRELATION_DESCRIPTION>(correlationCount);
                statelessDescription->Correlations = correlations.GetRawArray();
                for (size_t correlationIndex = 0; correlationIndex < correlationCount; ++correlationIndex)
                {
                    correlations[correlationIndex].ServiceName = heap.AddString(Service.Correlations[correlationIndex].ServiceName);
                    correlations[correlationIndex].Scheme = Service.Correlations[correlationIndex].Scheme;
                }
            }
            else
            {
                statelessDescription->Correlations = nullptr;
            }

            if (PartitionScheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE)
            {
                auto partitionDescription = heap.AddItem<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION>();
                partitionDescription->PartitionCount = Service.PartitionCount;
                partitionDescription->LowKey = lowRange_;
                partitionDescription->HighKey = highRange_;
                statelessDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
            }
            else if (PartitionScheme == FABRIC_PARTITION_SCHEME_NAMED)
            {
                auto partitionDescription = heap.AddItem<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION>();
                partitionDescription->PartitionCount = Service.PartitionCount;
                auto pNames = heap.AddArray<LPCWSTR>(Service.PartitionCount);
                for (size_t i = 0; i < static_cast<size_t>(Service.PartitionCount); ++i)
                {
                    pNames[i] = heap.AddString(partitionNames_[i]);
                }

                partitionDescription->Names = pNames.GetRawArray();
                statelessDescription->PartitionSchemeDescription = partitionDescription.GetRawPointer();
            }
            else
            {
                statelessDescription->PartitionSchemeDescription = NULL;
            }

            size_t metricCount = Service.Metrics.size();
            statelessDescription->MetricCount = static_cast<ULONG>(metricCount);
            if (metricCount > 0)
            {
                auto metrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(metricCount);
                statelessDescription->Metrics = metrics.GetRawArray();
                for (size_t metricIndex = 0; metricIndex < metricCount; ++metricIndex)
                {
                    metrics[metricIndex].Name = heap.AddString(Service.Metrics[metricIndex].Name);
                    metrics[metricIndex].Weight = Service.Metrics[metricIndex].Weight;
                    metrics[metricIndex].PrimaryDefaultLoad = Service.Metrics[metricIndex].PrimaryDefaultLoad;
                    metrics[metricIndex].SecondaryDefaultLoad = Service.Metrics[metricIndex].SecondaryDefaultLoad;
                }
            }
            else
            {
                statelessDescription->Metrics = nullptr;
            }

            auto statelessDescriptionEx1 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1>();
            statelessDescription->Reserved = statelessDescriptionEx1.GetRawPointer();

            // policy description
            size_t policyCount = Service.PlacementPolicies.size();
            if (policyCount > 0)
            {
                auto policyDescription = heap.AddItem<FABRIC_SERVICE_PLACEMENT_POLICY_LIST>();
                statelessDescriptionEx1->PolicyList = policyDescription.GetRawPointer();
                policyDescription->PolicyCount = static_cast<ULONG>(policyCount);

                auto policies = heap.AddArray<FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION>(policyCount);
                policyDescription->Policies = policies.GetRawArray();
                for (size_t policyIndex = 0; policyIndex < policyCount; ++policyIndex)
                {
                    Service.PlacementPolicies[policyIndex].ToPublicApi(heap, policies[policyIndex]);
                }
            }
            else
            {
                statelessDescriptionEx1->PolicyList = nullptr;
            }

            auto statelessDescriptionEx2 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2>();
            statelessDescriptionEx1->Reserved = statelessDescriptionEx2.GetRawPointer();
            statelessDescriptionEx2->IsDefaultMoveCostSpecified = true;
            statelessDescriptionEx2->DefaultMoveCost = static_cast<FABRIC_MOVE_COST> (Service.DefaultMoveCost);

            auto statelessDescriptionEx3 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3>();
            statelessDescriptionEx2->Reserved = statelessDescriptionEx3.GetRawPointer();
            statelessDescriptionEx3->ServicePackageActivationMode = ServicePackageActivationMode::ToPublicApi(Service.PackageActivationMode);
            statelessDescriptionEx3->ServiceDnsName = heap.AddString(Service.ServiceDnsName);

	    auto statelessDescriptionEx4 = heap.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4>();
            statelessDescriptionEx3->Reserved = statelessDescriptionEx4.GetRawPointer();

            size_t scalingPolicyCount = Service.ScalingPolicies.size();
            if (scalingPolicyCount > 0)
            {
                statelessDescriptionEx4->ScalingPolicyCount = static_cast<ULONG>(scalingPolicyCount);
                auto spArray = heap.AddArray<FABRIC_SERVICE_SCALING_POLICY> (scalingPolicyCount);
                statelessDescriptionEx4->ServiceScalingPolicies = spArray.GetRawArray();
                for (size_t spIndex = 0; spIndex < scalingPolicyCount; ++spIndex)
                {
                    Service.ScalingPolicies[spIndex].ToPublicApi(heap, spArray[spIndex]);
                }
            }
            else
            {
                statelessDescriptionEx4->ScalingPolicyCount = 0;
                statelessDescriptionEx4->ServiceScalingPolicies = nullptr;
            }
        }
    }

    void PartitionedServiceDescriptor::ToWrapper(PartitionedServiceDescWrapper & psdWrapper) const
    {
        PartitionSchemeDescription::Enum scheme;
        
        switch (partitionScheme_)
        {
            case FABRIC_PARTITION_SCHEME_NAMED: scheme = PartitionSchemeDescription::Named; break;
            case FABRIC_PARTITION_SCHEME_SINGLETON: scheme = PartitionSchemeDescription::Singleton; break;
            case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE: scheme = PartitionSchemeDescription::UniformInt64Range; break;
            default: scheme = PartitionSchemeDescription::Invalid;
        }

        uint flags = FABRIC_STATEFUL_SERVICE_SETTINGS_NONE;

        if (service_.ReplicaRestartWaitDuration >= TimeSpan::Zero)
        {
            flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION;
        }

        if (service_.QuorumLossWaitDuration >= TimeSpan::Zero)
        {
            flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION;
        }

        if (service_.StandByReplicaKeepDuration >= TimeSpan::Zero)
        {
            flags |= FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION;
        }

        PartitionedServiceDescWrapper wrapper(
            service_.IsStateful ? FABRIC_SERVICE_KIND_STATEFUL : FABRIC_SERVICE_KIND_STATELESS,
            service_.ApplicationName,
            service_.Name,
            service_.Type.ServiceTypeName,
            service_.InitializationData,
            scheme,
            service_.PartitionCount,
            lowRange_,
            highRange_,
            partitionNames_,
            service_.TargetReplicaSetSize,
            service_.TargetReplicaSetSize,
            service_.IsStateful ? service_.MinReplicaSetSize : 1,
            service_.IsStateful ? service_.HasPersistedState : false,
            service_.PlacementConstraints,
            service_.Correlations,
            service_.Metrics,
            Service.PlacementPolicies,
            static_cast<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS>(flags),
            static_cast<DWORD>(service_.ReplicaRestartWaitDuration.TotalSeconds()),
            static_cast<DWORD>(service_.QuorumLossWaitDuration.TotalSeconds()),
            static_cast<DWORD>(service_.StandByReplicaKeepDuration.TotalSeconds()),
            static_cast<FABRIC_MOVE_COST>(service_.DefaultMoveCost),
            service_.PackageActivationMode,
            service_.ServiceDnsName,
            service_.ScalingPolicies);

        wrapper.IsServiceGroup = IsServiceGroup;

        psdWrapper = move(wrapper);
    }

    Common::ErrorCode PartitionedServiceDescriptor::FromWrapper(ServiceModel::PartitionedServiceDescWrapper const& psdWrapper)
    {
        int maxNamingUriLength = CommonConfig::GetConfig().MaxNamingUriLength;
        if (psdWrapper.ServiceName.length() > maxNamingUriLength)
        {
            return TraceAndGetErrorDetails(
                ErrorCodeValue::InvalidArgument, 
                wformatString("{0} ({1},{2},{3})", GET_COM_RC(String_Too_Long), L"ServiceName", psdWrapper.ServiceName.length(), maxNamingUriLength));
        }

        ServiceTypeIdentifier typeIdentifier;

        auto error = ServiceTypeIdentifier::FromString(psdWrapper.ServiceTypeName, typeIdentifier);
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "Could not parse ServiceTypeName '{0}'", psdWrapper.ServiceTypeName);
            return error;
        }

        std::vector<Reliability::ServiceCorrelationDescription> correlations;
        for (size_t i = 0; i < psdWrapper.Correlations.size(); i++)
        {
            if (psdWrapper.Correlations[i].ServiceName.size() > CommonConfig::GetConfig().MaxNamingUriLength)
            {
                return TraceAndGetErrorDetails(
                    ErrorCodeValue::InvalidArgument,
                    wformatString("{0} ({1} \"{2}\",{3}, {4})", GET_COM_RC(String_Too_Long), L"Correlations.ServiceName", psdWrapper.Correlations[i].ServiceName, psdWrapper.ServiceName.length(), maxNamingUriLength));
            }
            
            NamingUri correlationServiceNameUri;
            if (!NamingUri::TryParse(psdWrapper.Correlations[i].ServiceName, correlationServiceNameUri))
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC(Invalid_Correlation_Service),
                    psdWrapper.Correlations[i].ServiceName));
            }

            // strip of the fragment (i.e. service group member names)
            if (!correlationServiceNameUri.Fragment.empty())
            {
                correlationServiceNameUri = NamingUri(correlationServiceNameUri.Path);
            }

            correlations.push_back(Reliability::ServiceCorrelationDescription(
                correlationServiceNameUri.Name, 
                psdWrapper.Correlations[i].Scheme));
        }
        
        isPlacementConstraintsValid_ = (!psdWrapper.PlacementConstraints.empty());

        NamingUri nameUri;
        if (!NamingUri::TryParse(psdWrapper.ServiceName, nameUri))
        {
            return TraceAndGetErrorDetails(
                ErrorCodeValue::InvalidNameUri, 
                wformatString("{0} {1}", GET_NS_RC(Invalid_Uri), psdWrapper.ServiceName));
        }

        //
        // TargetReplicaSize is a overloaded member used for both stateless and stateful services.
        //
        LONG targetReplicaSize = psdWrapper.TargetReplicaSetSize;
        LONG minReplicaSetSize = psdWrapper.MinReplicaSetSize;
        if (psdWrapper.ServiceKind == FABRIC_SERVICE_KIND_STATELESS)
        {
            targetReplicaSize = psdWrapper.InstanceCount;
            minReplicaSetSize = 1;
        }

        isPlacementPolicyValid_ = (!psdWrapper.PlacementPolicies.empty());

        int partitionCount = psdWrapper.PartitionCount;
        FABRIC_PARTITION_SCHEME scheme;
        switch (psdWrapper.PartitionScheme)
        {
            case PartitionSchemeDescription::Named: scheme = FABRIC_PARTITION_SCHEME_NAMED; break;
            case PartitionSchemeDescription::UniformInt64Range: scheme = FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE; break;
            case PartitionSchemeDescription::Singleton: scheme = FABRIC_PARTITION_SCHEME_SINGLETON; break;
            default: scheme = FABRIC_PARTITION_SCHEME_INVALID;
        }

        if (scheme == FABRIC_PARTITION_SCHEME_SINGLETON)
        {
            partitionCount = 1;
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_NAMED)
        {
            partitionCount = static_cast<int>(psdWrapper.PartitionNames.size());
        }

        error = ValidateServiceParameters(
            psdWrapper.ServiceName,
            psdWrapper.PlacementConstraints,
            psdWrapper.Metrics,
            correlations,
            psdWrapper.ScalingPolicies,
            psdWrapper.ApplicationName,
            typeIdentifier,
            partitionCount,
            targetReplicaSize,
            minReplicaSetSize,
            (psdWrapper.ServiceKind == FABRIC_SERVICE_KIND_STATEFUL),
            scheme,
            psdWrapper.LowKeyInt64,
            psdWrapper.HighKeyInt64,
            psdWrapper.PartitionNames);

        if (!error.IsSuccess())
        {
            return error;
        }

        TimeSpan replicaRestartWaitDuration(TimeSpan::MinValue);
        if ((psdWrapper.Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) != 0)
        {
            replicaRestartWaitDuration = TimeSpan::FromSeconds(psdWrapper.ReplicaRestartWaitDurationSeconds);
        }

        TimeSpan quorumLossWaitDuration(TimeSpan::MinValue);
        if ((psdWrapper.Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) != 0)
        {
            quorumLossWaitDuration = TimeSpan::FromSeconds(psdWrapper.QuorumLossWaitDurationSeconds);
        }

        TimeSpan standByReplicaKeepDuration(TimeSpan::MinValue);
        if ((psdWrapper.Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)
        {
            standByReplicaKeepDuration = TimeSpan::FromSeconds(psdWrapper.StandByReplicaKeepDurationSeconds);
        }

        // If default move cost is specified in the wrapper we use it, otherwise we set it to low
        FABRIC_MOVE_COST defaultMoveCost = psdWrapper.IsDefaultMoveCostSpecified ? psdWrapper.DefaultMoveCost : FABRIC_MOVE_COST_LOW;

        Reliability::ServiceDescription description(
            nameUri.ToString(),
            0, // instance
            0, // UpdateVersion
            partitionCount,
            targetReplicaSize,
            minReplicaSetSize,
            (psdWrapper.ServiceKind == FABRIC_SERVICE_KIND_STATEFUL),
            psdWrapper.HasPersistedState,
            replicaRestartWaitDuration,
            quorumLossWaitDuration,
            standByReplicaKeepDuration,
            typeIdentifier,
            correlations,
            psdWrapper.PlacementConstraints,
            0, // ScaleoutCount
            psdWrapper.Metrics,
            defaultMoveCost, // DefaultMoveCost
            psdWrapper.InitializationData,
            psdWrapper.ApplicationName,
            psdWrapper.PlacementPolicies,
            psdWrapper.PackageActivationMode,
            psdWrapper.ServiceDnsName,
            psdWrapper.ScalingPolicies);

        service_ = move(description);
        partitionScheme_ = scheme;            
        lowRange_ = psdWrapper.LowKeyInt64;
        highRange_ = psdWrapper.HighKeyInt64;
        partitionNames_ = psdWrapper.PartitionNames;
        IsServiceGroup = psdWrapper.IsServiceGroup;

        GenerateRandomCuids();

        return ErrorCode::Success();
    }

    Common::ErrorCode PartitionedServiceDescriptor::FromPublicApi(FABRIC_SERVICE_DESCRIPTION const & serviceDescription)
    {
        if (serviceDescription.Value == NULL) 
        { 
            Trace.WriteWarning(TraceComponent, "Invalid NULL parameter: serviceDescription.Description");
            return ErrorCode::FromHResult(E_POINTER); 
        }

        std::wstring applicationName;
        std::wstring name;
        ServiceTypeIdentifier typeIdentifier;
        int partitionCount;
        int targetReplicaSetSize;
        int minReplicaSetSize;
        bool isStateful;
        bool hasPersistedState;
        TimeSpan replicaRestartWaitDuration(TimeSpan::MinValue);
        TimeSpan quorumLossWaitDuration(TimeSpan::MinValue);
        TimeSpan standByReplicaKeepDuration(TimeSpan::MinValue);
        std::vector<BYTE> initializationData;
        std::vector<Reliability::ServiceCorrelationDescription> correlations;
        std::wstring placementConstraints;
        std::vector<Reliability::ServiceLoadMetricDescription> metrics;
        std::vector<ServiceModel::ServicePlacementPolicyDescription> placementPolicies;
        FABRIC_MOVE_COST defaultMoveCost = FABRIC_MOVE_COST_LOW;
        ServicePackageActivationMode::Enum servicePackageActivationMode = ServicePackageActivationMode::SharedProcess;

        FABRIC_PARTITION_SCHEME partitionScheme;
        void * partitionDescription;
        __int64 lowKeyInt64 = -1;
        __int64 highKeyInt64 = -1;
        std::vector<std::wstring> partitionNames;

        std::wstring serviceDnsName;

        std::vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;

        switch (serviceDescription.Kind)
        {
        case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
        {
            auto stateless = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION*>(
                serviceDescription.Value);

            HRESULT hr = StringUtility::LpcwstrToWstring(stateless->ServiceName, false /*acceptNull*/, ParameterValidator::MinStringSize, CommonConfig::GetConfig().MaxNamingUriLength, name);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            hr = StringUtility::LpcwstrToWstring(stateless->ApplicationName, true /*acceptNull*/, applicationName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            wstring serviceTypeName;
            hr = StringUtility::LpcwstrToWstring(stateless->ServiceTypeName, false /*acceptNull*/, serviceTypeName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            ErrorCode error = ServiceTypeIdentifier::FromString(serviceTypeName, typeIdentifier);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "Could not parse ServiceTypeName '{0}'", stateless->ServiceTypeName);
                return error;
            }

            if (stateless->CorrelationCount > 0 && stateless->Correlations == NULL)
            {
                Trace.WriteWarning(TraceComponent, "Invalid NULL parameter: Service correlations");
                return ErrorCode::FromHResult(E_POINTER);
            }
            for (ULONG i = 0; i < stateless->CorrelationCount; i++)
            {
                wstring correlationServiceName;
                hr = StringUtility::LpcwstrToWstring(stateless->Correlations[i].ServiceName, true /*acceptNull*/, 0, CommonConfig::GetConfig().MaxNamingUriLength, correlationServiceName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                NamingUri correlationServiceNameUri;
                if (!NamingUri::TryParse(correlationServiceName, correlationServiceNameUri))
                {
                    Trace.WriteWarning(TraceComponent, "Could not parse ServiceName '{0}' in correlation description", correlationServiceName);
                    return ErrorCode(ErrorCodeValue::InvalidArgument);
                }

                // strip of the fragment (i.e. service group member names)
                if (!correlationServiceNameUri.Fragment.empty())
                {
                    correlationServiceNameUri = NamingUri(correlationServiceNameUri.Path);
                }

                correlations.push_back(Reliability::ServiceCorrelationDescription(
                    correlationServiceNameUri.Name,
                    stateless->Correlations[i].Scheme));
            }

            isPlacementConstraintsValid_ = (stateless->PlacementConstraints != nullptr);

            hr = StringUtility::LpcwstrToWstring(stateless->PlacementConstraints, true /*acceptNull*/, placementConstraints);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            if (stateless->MetricCount > 0 && stateless->Metrics == NULL)
            {
                Trace.WriteWarning(TraceComponent, "Invalid NULL parameter: Service description metrics");
                return ErrorCode::FromHResult(E_POINTER);
            }

            for (ULONG i = 0; i < stateless->MetricCount; i++)
            {
                wstring metricsName;
                hr = StringUtility::LpcwstrToWstring(stateless->Metrics[i].Name, true /*acceptNull*/, metricsName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                metrics.push_back(Reliability::ServiceLoadMetricDescription(
                    move(metricsName),
                    stateless->Metrics[i].Weight,
                    stateless->Metrics[i].PrimaryDefaultLoad,
                    stateless->Metrics[i].SecondaryDefaultLoad));
            }

            initializationData = std::vector<byte>(
                stateless->InitializationData,
                stateless->InitializationData + stateless->InitializationDataSize);
            targetReplicaSetSize = stateless->InstanceCount;
            minReplicaSetSize = 1;
            isStateful = false;
            hasPersistedState = false;
            partitionScheme = stateless->PartitionScheme;
            partitionDescription = stateless->PartitionSchemeDescription;

            if (stateless->Reserved == NULL)
            {
                break;
            }

            auto statelessEx1 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX1*>(stateless->Reserved);
            if (statelessEx1->PolicyList != NULL)
            {
                isPlacementPolicyValid_ = true;
                auto pList = statelessEx1->PolicyList;
                if (pList->PolicyCount > 0 && pList->Policies == NULL)
                {
                    Trace.WriteWarning("PartitionedServiceDescription", "Invalid NULL parameter: Service placement policy");
                    return ErrorCode::FromHResult(E_POINTER);
                }

                for (ULONG i = 0; i < pList->PolicyCount; i++)
                {
                    std::wstring domainName;
                    FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                    ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                    placementPolicies.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
                }
            }

            if (statelessEx1->Reserved == NULL)
            {
                break;
            }

            auto statelessEx2 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX2*>(statelessEx1->Reserved);
            if (statelessEx2->IsDefaultMoveCostSpecified)
            {
                defaultMoveCost = statelessEx2->DefaultMoveCost;
            }

            if (statelessEx2->Reserved == NULL)
            {
                break;
            }

            auto statelessEx3 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX3*>(statelessEx2->Reserved);

            error = ServicePackageActivationMode::FromPublicApi(statelessEx3->ServicePackageActivationMode, servicePackageActivationMode);
            if (!error.IsSuccess())
            {
                Trace.WriteWarning(
                    TraceComponent,
                    "ReplicaIsolationLevel::FromPublicApi failed. statelessEx3->ServicePackageActivationMode='{0}'",
                    static_cast<ULONG>(statelessEx3->ServicePackageActivationMode));

		 return error;
            }

            hr = StringUtility::LpcwstrToWstring(statelessEx3->ServiceDnsName, true /*acceptNull*/, serviceDnsName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
           
            if (statelessEx3->Reserved == NULL)
            {
                break;
            }

            auto statelessEx4 = reinterpret_cast<FABRIC_STATELESS_SERVICE_DESCRIPTION_EX4*>(statelessEx3->Reserved);

            if (statelessEx4->ScalingPolicyCount > 1)
            {
                // Currently, only one scaling policy is allowed per service.
                // Vector is there for future uses (when services could have multiple scaling policies).
                return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statelessEx4->ScalingPolicyCount));
            }

            for (ULONG i = 0; i < statelessEx4->ScalingPolicyCount; i++)
            {
                Reliability::ServiceScalingPolicyDescription scalingDescription;
                auto scalingError = scalingDescription.FromPublicApi(statelessEx4->ServiceScalingPolicies[i]);
                if (!scalingError.IsSuccess())
                {
                    return scalingError;
                }
                scalingPolicies.push_back(move(scalingDescription));
            }

            break;
        }

        case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
            {
                auto stateful = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION*>(
                    serviceDescription.Value);

                HRESULT hr = StringUtility::LpcwstrToWstring(stateful->ServiceName, false /*acceptNull*/, ParameterValidator::MinStringSize, CommonConfig::GetConfig().MaxNamingUriLength, name);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                hr = StringUtility::LpcwstrToWstring(stateful->ApplicationName, true /*acceptNull*/, applicationName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                                
                wstring serviceTypeName;
                hr = StringUtility::LpcwstrToWstring(stateful->ServiceTypeName, false /*acceptNull*/, serviceTypeName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                
                ErrorCode error = ServiceTypeIdentifier::FromString(serviceTypeName, typeIdentifier);
                if (!error.IsSuccess())
                {
                    Trace.WriteWarning(TraceComponent, "Could not parse ServiceTypeName '{0}'", stateful->ServiceTypeName);
                    return error;
                }

                if (stateful->CorrelationCount > 0 && stateful->Correlations == NULL) 
                { 
                    Trace.WriteWarning(TraceComponent, "Invalid NULL parameter: Service correlations");
                    return ErrorCode::FromHResult(E_POINTER); 
                }
                for(ULONG i = 0; i < stateful->CorrelationCount; i++)
                {
                    wstring correlationServiceName;
                    hr = StringUtility::LpcwstrToWstring(stateful->Correlations[i].ServiceName, true /*acceptNull*/, 0, CommonConfig::GetConfig().MaxNamingUriLength, correlationServiceName);
                    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                    NamingUri correlationServiceNameUri;
                    if (!NamingUri::TryParse(correlationServiceName, correlationServiceNameUri))
                    {
                        Trace.WriteWarning(TraceComponent, "Could not parse ServiceName '{0}' in correlation description", correlationServiceName);
                        return ErrorCode(ErrorCodeValue::InvalidArgument);
                    }
                    
                    // strip of the fragment (i.e. service group member names)
                    if (!correlationServiceNameUri.Fragment.empty())
                    {
                        correlationServiceNameUri = NamingUri(correlationServiceNameUri.Path);
                    }

                    correlations.push_back(Reliability::ServiceCorrelationDescription(
                        correlationServiceNameUri.Name, 
                        stateful->Correlations[i].Scheme));
                }

                isPlacementConstraintsValid_ = (stateful->PlacementConstraints != nullptr);

                hr = StringUtility::LpcwstrToWstring(stateful->PlacementConstraints, true /*acceptNull*/, placementConstraints);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                
                if (stateful->MetricCount > 0 && stateful->Metrics == NULL) 
                { 
                    Trace.WriteWarning(TraceComponent, "Invalid NULL parameter: Service description metrics");
                    return ErrorCode::FromHResult(E_POINTER);
                }

                for(ULONG i = 0; i < stateful->MetricCount; i++)
                {
                    wstring metricsName;
                    hr = StringUtility::LpcwstrToWstring(stateful->Metrics[i].Name, true /*acceptNull*/, metricsName);
                    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                    metrics.push_back(Reliability::ServiceLoadMetricDescription(
                        move(metricsName), 
                        stateful->Metrics[i].Weight,
                        stateful->Metrics[i].PrimaryDefaultLoad,
                        stateful->Metrics[i].SecondaryDefaultLoad));
                }

                initializationData = std::vector<byte>(
                    stateful->InitializationData,
                    stateful->InitializationData + stateful->InitializationDataSize);
                targetReplicaSetSize = stateful->TargetReplicaSetSize;
                minReplicaSetSize = stateful->MinReplicaSetSize;
                isStateful = true;
                hasPersistedState = stateful->HasPersistedState ? true : false;
                partitionScheme = stateful->PartitionScheme;
                partitionDescription = stateful->PartitionSchemeDescription;

                if (stateful->Reserved == NULL)
                {
                    break;
                }

                auto statefulEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX1*>(stateful->Reserved);

                if (statefulEx1->PolicyList != NULL)
                {
                    isPlacementPolicyValid_ = true;
                    auto pList = statefulEx1->PolicyList;
                    if (pList->PolicyCount > 0 && pList->Policies == NULL)
                    {
                        Trace.WriteWarning("PartitionedServiceDescription", "Invalid NULL parameter: Service placement policy");
                        return ErrorCode::FromHResult(E_POINTER);
                    }

                    for (ULONG i = 0; i < pList->PolicyCount; i++)
                    {
                        std::wstring domainName;
                        FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION & policyDesc = pList->Policies[i];
                        ServicePlacementPolicyHelper::PolicyDescriptionToDomainName(policyDesc, domainName);
                        placementPolicies.push_back(ServiceModel::ServicePlacementPolicyDescription(move(domainName), policyDesc.Type));
                    }
                }

                if (statefulEx1->FailoverSettings != NULL)
                {
                    auto failoverSettings = statefulEx1->FailoverSettings;

                    if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_REPLICA_RESTART_WAIT_DURATION) != 0)
                    {
                        replicaRestartWaitDuration = TimeSpan::FromSeconds(failoverSettings->ReplicaRestartWaitDurationSeconds);
                    }

                    if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_QUORUM_LOSS_WAIT_DURATION) != 0)
                    {
                        quorumLossWaitDuration = TimeSpan::FromSeconds(failoverSettings->QuorumLossWaitDurationSeconds);
                    }

                    if (failoverSettings->Reserved != NULL)
                    {
                        auto failoverSettingsEx1 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_EX1*>(failoverSettings->Reserved);
                        if (failoverSettingsEx1 == NULL)
                        {
                            return ErrorCode::FromHResult(E_INVALIDARG);
                        }

                        if ((failoverSettings->Flags & FABRIC_STATEFUL_SERVICE_SETTINGS_STANDBY_REPLICA_KEEP_DURATION) != 0)
                        {
                            standByReplicaKeepDuration = TimeSpan::FromSeconds(failoverSettingsEx1->StandByReplicaKeepDurationSeconds);
                        }

                    }
                }

                if (statefulEx1->Reserved == NULL)
                {
                    break;
                }

                auto statefulEx2 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX2*>(statefulEx1->Reserved);
                if (statefulEx2->IsDefaultMoveCostSpecified)
                {
                    defaultMoveCost = statefulEx2->DefaultMoveCost;
                }

                if (statefulEx2->Reserved == NULL)
                {
                    break;
                }

                auto statefulEx3 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX3*>(statefulEx2->Reserved);

                error = ServicePackageActivationMode::FromPublicApi(statefulEx3->ServicePackageActivationMode, servicePackageActivationMode);
                if (!error.IsSuccess())
                {
                    Trace.WriteWarning(
                        TraceComponent,
                        "ReplicaIsolationLevel::FromPublicApi failed. statefulEx3->ServicePackageActivationMode='{0}'",
                        static_cast<ULONG>(statefulEx3->ServicePackageActivationMode));

                    return error;
                }

                hr = StringUtility::LpcwstrToWstring(statefulEx3->ServiceDnsName, true /*acceptNull*/, serviceDnsName);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

                if (statefulEx3->Reserved == NULL)
                {
                    break;
                }

                auto statefulEx4 = reinterpret_cast<FABRIC_STATEFUL_SERVICE_DESCRIPTION_EX4*>(statefulEx3->Reserved);

                if (statefulEx4->ScalingPolicyCount > 1)
                {
                    // Currently, only one scaling policy is allowed per service.
                    // Vector is there for future uses (when services could have multiple scaling policies).
                    return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), statefulEx4->ScalingPolicyCount));
                }

                for (ULONG i = 0; i < statefulEx4->ScalingPolicyCount; i++)
                {
                    Reliability::ServiceScalingPolicyDescription scalingDescription;
                    auto scalingError = scalingDescription.FromPublicApi(statefulEx4->ServiceScalingPolicies[i]);
                    if (!scalingError.IsSuccess())
                    {
                        return scalingError;
                    }
                    scalingPolicies.push_back(move(scalingDescription));
                }
            }
            break;

        default: 
            return ErrorCode::FromHResult(E_INVALIDARG);
        }

        switch (partitionScheme)
        {
        case FABRIC_PARTITION_SCHEME_SINGLETON:
            partitionCount = 1;
            break;

        case FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
            {
                if (partitionDescription == NULL)
                {
                    Trace.WriteWarning(
                        TraceComponent, 
                        "Partition description cannot be NULL for service: type = {0} name = {1}",
                        typeIdentifier,
                        name);

                    return ErrorCode::FromHResult(E_POINTER);
                }

                auto d = reinterpret_cast<FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION*>(partitionDescription);
                partitionCount = d->PartitionCount;
                lowKeyInt64 = d->LowKey;
                highKeyInt64 = d->HighKey;
                break;
            }

        case FABRIC_PARTITION_SCHEME_NAMED:
            {
                if (partitionDescription == NULL)
                {
                    Trace.WriteWarning(
                        TraceComponent, 
                        "Partition description cannot be NULL for service: type = {0} name = {1}",
                        typeIdentifier,
                        name);

                    return ErrorCode::FromHResult(E_POINTER);
                }

                auto d = reinterpret_cast<FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION*>(partitionDescription);
                partitionCount = d->PartitionCount;
                auto hr = StringUtility::FromLPCWSTRArray(partitionCount, d->Names, partitionNames);
                if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
                break;
            }

        default:
            return (partitionDescription != NULL) ? ErrorCode::FromHResult(E_INVALIDARG) : ErrorCode::FromHResult(E_POINTER);
        }

        NamingUri nameUri;
        if (!NamingUri::TryParse(name, nameUri))
        {
            return TraceAndGetErrorDetails(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}", GET_NS_RC(Invalid_Uri), name));
        }

        auto error = ValidateServiceParameters(
            name,
            placementConstraints,
            metrics,
            correlations,
            scalingPolicies,
            applicationName,
            typeIdentifier,
            partitionCount,
            targetReplicaSetSize,
            minReplicaSetSize,
            isStateful,
            partitionScheme,
            lowKeyInt64,
            highKeyInt64,
            partitionNames);

        if (!error.IsSuccess())
        {
            return error;
        }

        Reliability::ServiceDescription description(
            nameUri.ToString(),
            0, // instance
            0, // UpdateVersion
            partitionCount,
            targetReplicaSetSize,
            minReplicaSetSize,
            isStateful,
            hasPersistedState,
            replicaRestartWaitDuration,
            quorumLossWaitDuration,
            standByReplicaKeepDuration,
            typeIdentifier,
            correlations,
            placementConstraints,
            0, // ScaleoutCount
            metrics,
            defaultMoveCost, // DefaultMoveCost
            initializationData,
            applicationName,
            placementPolicies,
            servicePackageActivationMode,
            serviceDnsName,
            scalingPolicies);

        service_ = move(description);
        partitionScheme_ = partitionScheme;
        lowRange_ = lowKeyInt64;
        highRange_ = highKeyInt64;
        partitionNames_ = partitionNames;

        GenerateRandomCuids();

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode PartitionedServiceDescriptor::ValidateServiceParameters(
        wstring const& serviceName,
        wstring const & placementConstraints,
        vector<Reliability::ServiceLoadMetricDescription> const & metrics,
        vector<Reliability::ServiceCorrelationDescription> const & correlations,
        vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies,
        wstring const & applicationName,
        ServiceTypeIdentifier const & typeIdentifier,
        int partitionCount,
        int targetReplicaSetSize,
        int minReplicaSetSize,
        bool isStateful, 
        FABRIC_PARTITION_SCHEME scheme,
        __int64 lowRange,
        __int64 highRange,
        vector<wstring> const & partitionNames) const
    {
        auto error = ValidateServiceMetrics(metrics, isStateful);
        if (!error.IsSuccess()) { return error; }

        error = ValidateServiceCorrelations(correlations, serviceName, targetReplicaSetSize);
        if (!error.IsSuccess()) { return error; }

        error = ValidateServiceScalingPolicy(
            metrics,
            scalingPolicies,
            isStateful,
            scheme,
            partitionNames);

        if (!error.IsSuccess())
        {
            return error;
        }

        if (!typeIdentifier.IsSystemServiceType() && !typeIdentifier.IsTombStoneServiceType())
        {
            if (!typeIdentifier.IsAdhocServiceType() && applicationName.empty())
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Missing_Application_Name ), 
                    typeIdentifier));
            }

            // Do not validate against: (typeIdentifier.IsAdhocServiceType() && !applicationName.empty())
            // since managed services are first created at the client with unqualified service type names.
            // The CM will internally qualify the service type name.
        }

        if (!Expression::Build(placementConstraints))
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Placement ), 
                placementConstraints));
        }
        else if (partitionCount < 1)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Partition_Count ), 
                partitionCount));
        }
        else if (targetReplicaSetSize < 1 && (targetReplicaSetSize != -1 || isStateful))
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Target_Replicas ), 
                targetReplicaSetSize));
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_SINGLETON && partitionCount != 1)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Singleton ), 
                partitionCount));
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE && lowRange > highRange)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} [{1}, {2}]", GET_NS_RC( Invalid_Range ), 
                lowRange, 
                highRange));
        }
#if !defined(PLATFORM_UNIX)
        else if (scheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE && (((highRange - lowRange) + 1) > 0) && // if subtraction overflows, the range is big enough
            (((highRange - lowRange) + 1) < static_cast<__int64>(partitionCount)))
#else
        else if (scheme == FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE &&    // (((highRange - lowRange) + 1) > 0) is optimized to "js (highRange - lowRange)"
            (static_cast<uint64_t>(highRange - lowRange) < static_cast<uint64_t>(partitionCount - 1)))
#endif
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} [{1}, {2}] ({3})", GET_NS_RC( Invalid_Range_Count ),
                lowRange,
                highRange,
                partitionCount));
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_NAMED && static_cast<__int64>(partitionNames.size()) != partitionCount)
        { 
            return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1} {2}", GET_NS_RC( Invalid_Names_Count ),
                partitionNames,
                partitionCount));
        }
        else if (scheme == FABRIC_PARTITION_SCHEME_NAMED && !ArePartitionNamesValid(partitionNames))
        {
            return TraceAndGetInvalidArgumentDetails(wformatString(GET_NS_RC( Invalid_Partition_Names ), partitionNames));
        }
        else if (isStateful)
        {
            if (minReplicaSetSize < 1)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Minimum_Replicas ), 
                    minReplicaSetSize));
            }
            else if (minReplicaSetSize > targetReplicaSetSize)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} ({1}, {2})", GET_NS_RC( Invalid_Minimum_Target ),
                    minReplicaSetSize,
                    targetReplicaSetSize));
            }
        }

        return error;
    }

    Common::ErrorCode PartitionedServiceDescriptor::ValidateServiceScalingPolicy(
        std::vector<Reliability::ServiceLoadMetricDescription> const & metrics,
        vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies,
        bool isStateful,
        FABRIC_PARTITION_SCHEME scheme,
        std::vector<std::wstring> const & partitionNames) const
    {
        if (scalingPolicies.size() == 0)
        {
            return ErrorCodeValue::Success;
        }

        if (scalingPolicies.size() > 1)
        {
            // Only 1 scaling policy is allowed. Vector is there for future uses
            // when services will be allowed to have more than 1 scaling policy.
            return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Scaling_Count), scalingPolicies.size()));
        }

        auto scalingPolicy = scalingPolicies[0];

        auto error = scalingPolicy.Validate(isStateful);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (scalingPolicy.Mechanism != nullptr && scalingPolicy.Mechanism->Kind == ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
        {
            // This scaling mechanism will add or remove partitions to scale the service.
            // This is allowed only with FABRIC_PARTITION_SCHEME_NAMED
            // Expectation is that partition names will be "0", "1", ..., "N-1" for N partitions.
            if (scheme != FABRIC_PARTITION_SCHEME_NAMED)
            {
                return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Named_Partitions));
            }
            if (partitionNames.size() > 0)
            {
                // Other validation will make sure that there are no duplicate names.
                uint64 minValue = UINT_MAX;
                uint64 maxValue = 0;
                for (auto name : partitionNames)
                {
                    uint64 value;
                    if (!Common::TryParseUInt64(name, value))
                    {
                        return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Named_Partitions));
                    }
                    if (value < minValue)
                    {
                        minValue = value;
                    }
                    if (value > maxValue)
                    {
                        maxValue = value;
                    }
                }
                if (minValue != 0 || maxValue != partitionNames.size() - 1)
                {
                    return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Named_Partitions));
                }
            }
        }

        // Check the metrics: it can be RG metric or it has to be defined in the description.
        wstring metricName = L"";
        if (scalingPolicy.Trigger != nullptr)
        {
            metricName = scalingPolicy.Trigger->GetMetricName();
        }
        if (metricName != L"")
        {
            if (   metricName != Constants::SystemMetricNameCpuCores
                && metricName != Constants::SystemMetricNameMemoryInMB)
            {
                bool found = false;
                for (auto & metric : metrics)
                {
                    if (metric.Name == metricName)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    return TraceAndGetErrorDetails(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_Metric_Name), metricName));
                }
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::ValidateServiceMetrics(
        vector<Reliability::ServiceLoadMetricDescription> const & metrics, 
        bool isStateful) const
    {
        for (auto it = metrics.begin(); it != metrics.end(); it++)
        {
            if (it->Name.empty())
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0}", GET_NS_RC( Invalid_Metric )));
            }

            if (it->Weight < FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO ||
                it->Weight > FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} [{1}, {2}] ({3}, {4})", GET_NS_RC( Invalid_Metric_Range ), 
                    static_cast<int>(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO),
                    static_cast<int>(FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH),
                    it->Name, 
                    static_cast<int>(it->Weight)));
            }

            if (!isStateful && it->SecondaryDefaultLoad > 0u)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} ({1}, {2})", GET_NS_RC( Invalid_Metric_Load ), 
                    it->Name, 
                    it->SecondaryDefaultLoad));
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::ValidateServiceCorrelations(
        vector<Reliability::ServiceCorrelationDescription> const & correlations,
        wstring const& currentServiceName, 
        int targetReplicaSetSize) const
    {
        if (correlations.empty())
        {
            return ErrorCodeValue::Success;
        }
        else if (correlations.size() > 1)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString("{0}", GET_NS_RC( Invalid_Affinity_Count )));
        }
        else
        {
            Reliability::ServiceCorrelationDescription const& corr = correlations[0];

            if (corr.Scheme != FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY && 
                corr.Scheme != FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY && 
                corr.Scheme != FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Correlation_Scheme ), 
                    static_cast<int>(corr.Scheme)));
            }

            NamingUri correlatedServiceUri;
            if (corr.ServiceName.empty() || 
                currentServiceName == corr.ServiceName || 
                (!SystemServiceApplicationNameHelper::IsInternalServiceName(corr.ServiceName) && !NamingUri::TryParse(corr.ServiceName, correlatedServiceUri)))
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} {1}", GET_NS_RC( Invalid_Correlation_Service ), 
                    corr.ServiceName));
            }

            if (targetReplicaSetSize == -1)
            {
                return TraceAndGetInvalidArgumentDetails(wformatString("{0} ({1}, {2})", GET_NS_RC( Invalid_Correlation ),
                    currentServiceName, 
                    corr.ServiceName));
            }

            return ErrorCodeValue::Success;
        }
    }

    ErrorCode PartitionedServiceDescriptor::TraceAndGetInvalidArgumentDetails(std::wstring && msg)
    {
        return TraceAndGetErrorDetails(ErrorCodeValue::InvalidArgument, move(msg));
    }

    ErrorCode PartitionedServiceDescriptor::TraceAndGetErrorDetails(ErrorCodeValue::Enum errorCode, std::wstring && msg)
    {
        Trace.WriteWarning(TraceComponent, "{0}", msg);

        return ErrorCode(errorCode, move(msg));
    }

    void PartitionedServiceDescriptor::GenerateRandomCuids()
    {
        cuids_.clear();
        for (int i = 0; i < service_.PartitionCount; ++i)
        {
            cuids_.push_back(Reliability::ConsistencyUnitId());
        }
    }

    vector<ConsistencyUnitDescription> PartitionedServiceDescriptor::CreateConsistencyUnitDescriptions() const
    {
        vector<ConsistencyUnitDescription> result;

        for (size_t ix = 0; ix < this->Cuids.size(); ++ix)
        {
            auto partitionInfo = this->GetPartitionInfo(this->Cuids[ix]);

            switch (partitionInfo.PartitionScheme)
            {
            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON:
                result.push_back(ConsistencyUnitDescription(this->Cuids[ix]));
                break;

            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                result.push_back(ConsistencyUnitDescription(this->Cuids[ix], partitionInfo.LowKey, partitionInfo.HighKey));
                break;

            case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
                result.push_back(ConsistencyUnitDescription(this->Cuids[ix], partitionInfo.PartitionName));
                break;

            default:
                Assert::CodingError("Unexpected partition key type {0}", partitionInfo.PartitionScheme);
            }
        }

        return result;
    }

    void PartitionedServiceDescriptor::UpdateCuids(vector<ConsistencyUnitId> && cuids)
    {
        cuids_ = move(cuids);
    }

    ErrorCode PartitionedServiceDescriptor::AddNamedPartitions(vector<wstring> const & names, __out vector<ConsistencyUnitDescription> & cuidResults)
    {
        if (partitionScheme_ != FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString(GET_NS_RC( Update_Unsupported_Scheme ), partitionScheme_)); 
        }

        if (names.size() > (numeric_limits<int>::max() - service_.PartitionCount))
        {
            return TraceAndGetInvalidArgumentDetails(wformatString(GET_NS_RC( Exceeded_Max_Partition_Count ), numeric_limits<int>::max(), names.size() + service_.PartitionCount)); 
        }

        cuidResults.clear();

        for (auto const & name : names)
        {
            auto partitionId = ConsistencyUnitId(Guid::NewGuid());

            cuids_.push_back(partitionId);
            partitionNames_.push_back(name);
            cuidResults.push_back(ConsistencyUnitDescription(partitionId, name));
        }

        if (partitionNames_.size() > numeric_limits<int>::max())
        {
            TRACE_LEVEL_AND_TESTASSERT(Trace.WriteError, TraceComponent, "Number of partition names {0} exceeds max limit {1}", partitionNames_.size(), numeric_limits<int>::max());

            return TraceAndGetInvalidArgumentDetails(wformatString(GET_NS_RC( Exceeded_Max_Partition_Count ), numeric_limits<int>::max(), partitionNames_.size()));
        }

        service_.PartitionCount = static_cast<int>(partitionNames_.size());

        return ErrorCodeValue::Success;
    }

    ErrorCode PartitionedServiceDescriptor::RemoveNamedPartitions(vector<wstring> const & names, __out vector<ConsistencyUnitDescription> & cuidResults)
    {
        if (partitionScheme_ != FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED)
        {
            return TraceAndGetInvalidArgumentDetails(wformatString(GET_NS_RC( Update_Unsupported_Scheme ), partitionScheme_)); 
        }

        cuidResults.clear();

        for (auto const & name : names)
        {
            bool found = false;

            for (auto ix=0; ix<partitionNames_.size(); ++ix)
            {
                if (name == partitionNames_[ix])
                {
                    auto cuidIter = (cuids_.begin() + ix);
                    auto partitionId = *cuidIter;

                    cuids_.erase(cuids_.begin() + ix);
                    partitionNames_.erase(partitionNames_.begin() + ix);
                    cuidResults.push_back(ConsistencyUnitDescription(partitionId, name));

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return TraceAndGetErrorDetails(ErrorCodeValue::InvalidArgument, wformatString(GET_NS_RC( Named_Partition_Not_Found ), name)); 
            }
        }

        service_.PartitionCount = static_cast<int>(partitionNames_.size());

        return ErrorCodeValue::Success;
    }

    void PartitionedServiceDescriptor::SortNamesForUpgradeDiff()
    {
        // The resulting list of sorted partition names must not
        // be persisted for services that already exist as this
        // will corrupt the existing name to CUID mapping
        //
        sort(partitionNames_.begin(), partitionNames_.end());
    }

    bool PartitionedServiceDescriptor::ArePartitionNamesValid(vector<wstring> const & partitionNames)
    {
        if (partitionNames.empty())
        {
            return false;
        }

        for (auto it = partitionNames.begin(); it != partitionNames.end(); ++it)
        {
            if (it->empty())
            {
                return false;
            }

            auto duplicateIter = find(it + 1, partitionNames.end(), *it);
            if (duplicateIter != partitionNames.end())
            {
                return false;
            }
        }

        return true;
    }

    void PartitionedServiceDescriptor::SetPlacementConstraints(wstring && value)
    {
        isPlacementConstraintsValid_ = true;
        service_.PlacementConstraints = move(value);
    }

    void PartitionedServiceDescriptor::SetPlacementPolicies(std::vector<ServiceModel::ServicePlacementPolicyDescription> && value)
    {
        isPlacementPolicyValid_ = true;
        service_.PlacementPolicies = move(value);
    }
    
    void PartitionedServiceDescriptor::SetLoadMetrics(std::vector<Reliability::ServiceLoadMetricDescription>&& value)
    {
        service_.Metrics = move(value);
    }

    bool PartitionedServiceDescriptor::operator == (PartitionedServiceDescriptor const & other) const
    {
        auto error = Equals(other);
        return error.IsSuccess();
    }

    bool PartitionedServiceDescriptor::operator != (PartitionedServiceDescriptor const & other) const
    {
        return !(*this == other);
    }

    ErrorCode PartitionedServiceDescriptor::Equals(PartitionedServiceDescriptor const & other) const
    {
        
        // Do not compare CUIDs because they are generated internally when a new
        // instance is constructed and do not cross the public API boundary.
        //

        if (partitionNames_.size() != other.partitionNames_.size())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_NS_RC(PartitionName_Count_Changed), partitionNames_.size(), other.partitionNames_.size()));
        }

        // check if this.partitionNames_ is a subset of other.partitionNames_
        for (auto iter = partitionNames_.begin(); iter != partitionNames_.end(); ++iter)
        {
            auto findIter = find(other.partitionNames_.begin(), other.partitionNames_.end(), *iter);
            if (findIter == other.partitionNames_.end())
            {
                return ErrorCode(
                    ErrorCodeValue::InvalidArgument,
                    wformatString(
                        GET_NS_RC(PartitionName_Removed), *iter));
            }
        }

        // check if other.partitionNames_ is a subset of this.partitionNames_
        for (auto iter = other.partitionNames_.begin(); iter != other.partitionNames_.end(); ++iter)
        {
            auto findIter = find(partitionNames_.begin(), partitionNames_.end(), *iter);
            if (findIter == partitionNames_.end())
            {
                return ErrorCode(
                    ErrorCodeValue::InvalidArgument,
                    wformatString(
                        GET_NS_RC(PartitionName_Inserted), *iter));
            }
        }

        if (service_.IsServiceGroup && other.service_.IsServiceGroup)
        {
            // ServiceGroup PSD contains the service member identifiers in its initialization data. 
            // These should be ignored during comparison.

            // Check whether the SDs are equal without the SG init data
            ServiceDescription first(service_, move(vector<byte>()));
            ServiceDescription second(other.service_, move(vector<byte>()));

            auto error = first.Equals(second);
            if (!error.IsSuccess())
            {
                return error;
            }

            // Check whether the SG init datas are equal ignoring the service member identifiers
            error = ServiceModel::CServiceGroupDescription::Equals(service_.InitializationData, other.service_.InitializationData, TRUE);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
        else
        {
            auto error = service_.Equals(other.service_);
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        if (partitionScheme_ != other.partitionScheme_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_NS_RC(PartitionScheme_Changed), partitionScheme_, other.partitionScheme_));
        }

        if (version_ != other.version_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_NS_RC(PSD_Version_Changed), version_, other.version_));
        }

        if (lowRange_ != other.lowRange_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_NS_RC(PSD_LowRange_Changed), lowRange_, other.lowRange_));
        }

        if (highRange_ != other.highRange_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_NS_RC(PSD_HighRange_Changed), highRange_, other.highRange_));
        }

        return ErrorCode::Success();
    }

    void PartitionedServiceDescriptor::GetPartitionRange(int partitionIndex, __out __int64 & partitionLowKey, __out __int64 & partitionHighKey) const
    {
        if (!TryGetPartitionRange(lowRange_, highRange_, service_.PartitionCount, partitionIndex, partitionLowKey, partitionHighKey))
        {
            Assert::CodingError("Invalid argument in GetPartitionRange: ServiceLowKey={0}, ServiceHighKey={1}, NumPartitions={2}, PartitionIndex={3}",
                lowRange_,
                highRange_,
                service_.PartitionCount,
                partitionIndex);
        }
    }

    bool PartitionedServiceDescriptor::TryGetPartitionIndex(
        PartitionKey const & key, 
        __out int & partitionIndex) const
    {
        switch (key.KeyType)
        {
            case FABRIC_PARTITION_KEY_TYPE_NONE:
            {
                if (partitionScheme_ != FABRIC_PARTITION_SCHEME_SINGLETON)
                {
                    return false;
                }

                partitionIndex = 0;
                return true;
            }

            case FABRIC_PARTITION_KEY_TYPE_INT64:
            {
                if (partitionScheme_ != FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE) 
                {
                    return false;
                }

                return TryGetPartitionIndex(
                    lowRange_, 
                    highRange_, 
                    service_.PartitionCount, 
                    key.Int64Key, 
                    partitionIndex);
            }

            case FABRIC_PARTITION_KEY_TYPE_STRING:
            {
                if (partitionScheme_ != FABRIC_PARTITION_SCHEME_NAMED)
                {
                    return false;
                }

                size_t index;
                if (partitionNamesIndexCache_->TryGetPartitionIndex(key.StringKey, partitionNames_, index))
                {
                    partitionIndex = static_cast<int>(index);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            default:
            {
                Trace.WriteWarning(
                    TraceComponent, 
                    "Unknown FABRIC_PARTITION_KEY_TYPE {0}", 
                    static_cast<int>(key.KeyType));
                return false;
            }
        } // switch key type
    }

    //
    // The function below calculates the key range for the range partition. The range is inclusive at both ends. 
    // The function determines the total keys and then distributes them equally among all partitions. 
    //
    // The remaining keys after the highest equal distribution are given to the partitions starting from the first 
    // partition. 
    //
    // The calculations are done in a way to not overflow 64 bit numbers.
    //
    //
    /*
    Examples:
    
        #######################################################
        LowKey         = -1
        HighKey        = 1
        NumPartitions  = 2
              [-1, 0]
              [1, 1]
    
        #######################################################
        LowKey         = -1
        HighKey        = 0
        NumPartitions  = 2
              [-1, -1]
              [0, 0]
    
        #######################################################
        LowKey         = 0
        HighKey        = 1
        NumPartitions  = 2
              [0, 0]
              [1, 1]
    
        #######################################################
        LowKey         = 0
        HighKey        = 33
        NumPartitions  = 4
              [0, 8]
              [9, 17]
              [18, 25]
              [26, 33]
    
        #######################################################
        LowKey         = -9223372036854775808
        HighKey        = 9223372036854775807
        NumPartitions  = 2
    	    [-9223372036854775808, -1]
    	    [0, 9223372036854775807]
    
        #######################################################
        LowKey         = -9223372036854775808
        HighKey        = 9223372036854775807
        NumPartitions  = 5
              [-9223372036854775808, -5534023222112865485]
              [-5534023222112865484, -1844674407370955162]
              [-1844674407370955161, 1844674407370955161]
              [1844674407370955162, 5534023222112865484]
              [5534023222112865485, 9223372036854775807]
    
    */
    bool PartitionedServiceDescriptor::TryGetPartitionRange( 
        __int64 serviceLowKey, 
        __int64 serviceHighKey, 
        int numPartitions, 
        int partitionIndex, 
        __out __int64 & partitionLowKey, 
        __out __int64 & partitionHighKey)
    {
       if ((serviceLowKey > serviceHighKey) || (numPartitions <= 0) || (partitionIndex >= numPartitions))
       {
           return false;
       }

       if (numPartitions == 1)
       {
           partitionLowKey = serviceLowKey;
           partitionHighKey = serviceHighKey;
           return true;
       }

        unsigned __int64 totalRange;
        unsigned __int64 rangePerPartition;
    
        if ((serviceLowKey < 0) && (serviceHighKey <= 0))
        {
            unsigned __int64 uLowKey = -1 * serviceLowKey;
            unsigned __int64 uHighKey = -1 * serviceHighKey;
    
            totalRange = uLowKey - uHighKey;
        }
        else
        if ((serviceLowKey >= 0) && (serviceHighKey > 0))
        {
            unsigned __int64 uLowKey = serviceLowKey;
            unsigned __int64 uHighKey = serviceHighKey;
    
            totalRange = uHighKey - uLowKey;
        }
        else
        {
            unsigned __int64 uLowKey = -1 * serviceLowKey;
            unsigned __int64 uHighKey = serviceHighKey;
    
            totalRange = uHighKey + uLowKey;
        }
    
        rangePerPartition = totalRange / numPartitions;
        unsigned __int64 remainder = totalRange % numPartitions;
    
        // the remainder is incremented to account for inclusive range 
        // this is done after the division to prevent overflow in the totalRange
        remainder++;

        // adjust the distribution if after adding one to remainder we can divide 
        // the keys equally among all partitions
        if (remainder == numPartitions)
        {
            rangePerPartition++;
            remainder = 0;
        }

        // this will be true if we have more number of partitions than the key range
        // this is not supported
        if (rangePerPartition < 1)
        {
            return false;
        }
    
        if (partitionIndex < remainder)
        {
            // these partitions handle one more key
            partitionLowKey = (__int64)(serviceLowKey + (partitionIndex * rangePerPartition) + partitionIndex);
            partitionHighKey = (__int64)(partitionLowKey + rangePerPartition);
        }
        else
        {
            partitionLowKey = (__int64)(serviceLowKey + (partitionIndex * rangePerPartition) + remainder);
            partitionHighKey = (__int64)(partitionLowKey + rangePerPartition - 1);
        }

        return true;
    }

    bool PartitionedServiceDescriptor::TryGetPartitionIndex( __int64 serviceLowKey, __int64 serviceHighKey, int numPartitions, __int64 key, __out int & partitionIndex)
    {
        if ((key < serviceLowKey) || (key > serviceHighKey) || (numPartitions <= 0))
        {
            return false;
        }

        int searchStartIndex = 0;
        int searchEndIndex = numPartitions;
        __int64 low;
        __int64 high;
        bool done = false;
        bool success = false;
        while(!done)
        {
            if (searchStartIndex == searchEndIndex)
            {
                done = true;

                if (TryGetPartitionRange(serviceLowKey, serviceHighKey, numPartitions, searchStartIndex, low, high))
                {
                    if ((key >= low) && (key <= high))
                    {
                        success = true;
                        partitionIndex = searchStartIndex;
                    }
                }
            }
            else
            {
                int probe = searchStartIndex + (searchEndIndex - searchStartIndex) / 2;

                if (!TryGetPartitionRange(serviceLowKey, serviceHighKey, numPartitions, probe, low, high))
                {
                    done = true;
                }
                else
                {
                    if (key > high)
                    {
                        searchStartIndex = probe + 1;
                    }
                    else if (key < low)
                    {
                        searchEndIndex = probe - 1;
                    }
                    else
                    {
                        partitionIndex = probe;
                        success = true;
                        done = true;
                    }
                }
            }
        } // while(!done)

        return success;
    }

    // *** PartitionIndexCache
    
    template class PartitionedServiceDescriptor::PartitionIndexCache<Reliability::ConsistencyUnitId>;
    template class PartitionedServiceDescriptor::PartitionIndexCache<std::wstring>;

    template <typename TKey>
    PartitionedServiceDescriptor::PartitionIndexCache<TKey>::PartitionIndexCache() 
            : indexCache_() 
            , indexCacheLock_()
        { 
        }

    template <typename TKey>
    bool PartitionedServiceDescriptor::PartitionIndexCache<TKey>::ContainsKey(
        TKey const & key, 
        vector<TKey> const & keys)
    {
        size_t unusedIndex;
        return this->TryGetPartitionIndex(key, keys, unusedIndex);
    }

    template <typename TKey>
    bool PartitionedServiceDescriptor::PartitionIndexCache<TKey>::TryGetPartitionIndex(
        TKey const & key, 
        vector<TKey> const & keys,
        __out size_t & index)
    {
        {
            AcquireReadLock lock(indexCacheLock_);
            
            if (!indexCache_.empty())
            {
                auto it = indexCache_.find(key);
                if (it != indexCache_.end())
                {
                    index = it->second;
                    return true;
                }

                return false;
            }
        }

        {
            AcquireWriteLock lock(indexCacheLock_);

            if (indexCache_.empty())
            {
                for (auto it=keys.begin(); it!=keys.end(); ++it)
                {
                    indexCache_.insert(make_pair<TKey, size_t>(
                        (TKey)*it,
                        distance(keys.begin(), it)));
                }
            }

            auto it = indexCache_.find(key);
            if (it != indexCache_.end())
            {
                index = it->second;
                return true;
            }
        }

        return false;
    }
}
