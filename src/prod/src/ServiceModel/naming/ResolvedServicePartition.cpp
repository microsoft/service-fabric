// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;
    using namespace ServiceModel;

    INITIALIZE_SIZE_ESTIMATION(ResolvedServicePartition)
        
    ResolvedServicePartition::ResolvedServicePartition()
      : isServiceGroup_(false)
      , locations_(ConsistencyUnitId::Zero, L"", ServiceReplicaSet())
      , partitionData_(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON)
      , storeVersion_(ServiceModel::Constants::UninitializedVersion)
      , generation_()
      , psd_()
    {
    }
 
    ResolvedServicePartition::ResolvedServicePartition(
        bool isServiceGroup,
        ServiceTableEntry const & locations, 
        PartitionInfo const & partitionData,
        GenerationNumber const & generation,
        __int64 storeVersion,
        PartitionedServiceDescriptorSPtr const & psd)
      : isServiceGroup_(isServiceGroup)
      , locations_(locations)
      , partitionData_(partitionData)
      , storeVersion_(storeVersion)
      , generation_(generation)
      , psd_(psd)
    {
    }

    ResolvedServicePartition::ResolvedServicePartition(
        ResolvedServicePartition const & other,
        ServiceTableEntry && locations)
      : isServiceGroup_(other.isServiceGroup_)
      , locations_(move(locations))
      , partitionData_(other.partitionData_)
      , storeVersion_(other.storeVersion_)
      , generation_(other.generation_)
      , psd_(other.psd_)
    {
    }

    ResolvedServicePartition::ResolvedServicePartition(ResolvedServicePartition const & other)
      : isServiceGroup_(other.isServiceGroup_)
      , locations_(other.locations_)
      , partitionData_(other.partitionData_)
      , storeVersion_(other.storeVersion_)
      , generation_(other.generation_)
      , psd_(other.psd_)
    {
    }

    ResolvedServicePartition::ResolvedServicePartition(ResolvedServicePartition && other)
      : isServiceGroup_(std::move(other.isServiceGroup_))
      , locations_(std::move(other.locations_))
      , partitionData_(std::move(other.partitionData_))
      , storeVersion_(std::move(other.storeVersion_))
      , generation_(std::move(other.generation_))
      , psd_(std::move(other.psd_))
    {
    }

    ResolvedServicePartition & ResolvedServicePartition::operator = (ResolvedServicePartition const & other)
    {
        if (this != &other)
        {
            isServiceGroup_ = other.isServiceGroup_;
            locations_ = other.locations_;
            partitionData_ = other.partitionData_;
            storeVersion_ = other.storeVersion_;
            generation_ = other.generation_;
            psd_ = other.psd_;
        }

        return *this;
    }

    ResolvedServicePartition & ResolvedServicePartition::operator = (ResolvedServicePartition && other)
    {
        if (this != &other)
        {
            isServiceGroup_ = std::move(other.isServiceGroup_);
            locations_ = std::move(other.locations_);
            partitionData_ = std::move(other.partitionData_);
            storeVersion_ = std::move(other.storeVersion_);
            generation_ = std::move(other.generation_);
            psd_ = std::move(other.psd_);
        }

        return *this;
    }
    
    bool ResolvedServicePartition::operator == (ResolvedServicePartition const & other) const
    {
        return (IsServiceGroup == other.IsServiceGroup) &&
            (PartitionData == other.PartitionData) &&
            (Locations.ConsistencyUnitId == other.Locations.ConsistencyUnitId) &&
            (Locations.ServiceName == other.Locations.ServiceName) &&
            (Generation == other.Generation) &&
            (FMVersion == other.FMVersion) &&
            (StoreVersion == other.StoreVersion) &&
            ((psd_ == nullptr && other.psd_ == nullptr) || (psd_ != nullptr && other.psd_ != nullptr && *psd_ == *other.psd_));
    }

    bool ResolvedServicePartition::operator != (ResolvedServicePartition const & other) const
    {
        return !(*this == other);
    }

    ErrorCode ResolvedServicePartition::CompareVersion(
        ResolvedServicePartitionSPtr const & other,
        __out LONG & compareResult) const
    {
        if (!other)
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        return CompareVersion(*other, compareResult);
    }

    ErrorCode ResolvedServicePartition::CompareVersion(
        ResolvedServicePartition const & other,
        __out LONG & compareResult) const
    {
        if (isServiceGroup_ != other.isServiceGroup_ ||
            partitionData_ != other.partitionData_)
        {
            return ErrorCodeValue::ServiceMetadataMismatch;
        }

        NamingUri serviceName;
        NamingUri otherServiceName;
        if (!TryGetName(serviceName) || !TryGetName(otherServiceName))
        {
            return ErrorCodeValue::InvalidNameUri;
        }

        if (locations_.ConsistencyUnitId != other.locations_.ConsistencyUnitId)
        {
            return ErrorCodeValue::ServiceMetadataMismatch;
        }

        compareResult = CompareVersionNoValidation(other);

        return ErrorCodeValue::Success;
    }

    ErrorCode ResolvedServicePartition::CompareVersion(
        ResolvedServicePartitionMetadata const &metadata,
        __out LONG & compareResult) const
    {
        if (locations_.ConsistencyUnitId != ConsistencyUnitId(metadata.PartitionId))
        {
            return ErrorCodeValue::ServiceMetadataMismatch;
        }

        compareResult = CompareVersionNoValidation(
            metadata.Generation,
            metadata.FMVersion);

        return ErrorCodeValue::Success;
    }

    LONG ResolvedServicePartition::CompareVersionNoValidation(
        ResolvedServicePartition const & other) const
    {
        return CompareVersionNoValidation(
            other.Generation,
            other.FMVersion);
    }

    LONG ResolvedServicePartition::CompareVersionNoValidation(
        GenerationNumber const &otherGeneration,
        __int64 otherFMVersion) const
    {
        if (this->Generation < otherGeneration)
        {
            return -1;
        }
        else if (this->Generation == otherGeneration)
        {
            if (this->FMVersion < otherFMVersion)
            {
                return -1;
            }
            else if (FMVersion == otherFMVersion)
            {
                return 0;
            }
            else
            {
                return +1;
            }
        }
        else
        {
            return +1;
        }
    }

    bool ResolvedServicePartition::TryGetName(__out NamingUri & name) const
    {
        return NamingUri::TryParse(locations_.ServiceName, name);
    }

    size_t ResolvedServicePartition::GetEndpointCount() const
    {
        size_t count = 0;
        if (IsStateful)
        {
            if (locations_.ServiceReplicaSet.IsPrimaryLocationValid)
            {
                ++count;
            }

            count += locations_.ServiceReplicaSet.SecondaryLocations.size();
        }
        else
        {
            count += locations_.ServiceReplicaSet.ReplicaLocations.size();
        }

        return count;
    }

    void ResolvedServicePartition::ToPublicApi(
        __inout Common::ScopedHeap & heap, 
        __out FABRIC_RESOLVED_SERVICE_PARTITION & resolvedServicePartition) const
    {
        wstring name = L"";
        NamingUri nativeServiceName;
        if (this->TryGetName(nativeServiceName))
        {
            // For all user services, TryGetName should be true. The NamingService and ClusterManager do not use URI service names.
            name = nativeServiceName.ToString();
        }

        resolvedServicePartition.ServiceName = heap.AddString(name.c_str());

        ServiceEndpointsUtility::SetEndpoints(
            this->Locations.ServiceReplicaSet,
            heap,
            resolvedServicePartition.EndpointCount,
            &resolvedServicePartition.Endpoints);

        ServiceEndpointsUtility::SetPartitionInfo(
            this->Locations,
            this->PartitionData,
            heap,
            resolvedServicePartition.Info);
    }

    void ResolvedServicePartition::ToWrapper(ResolvedServicePartitionWrapper & rspWrapper)
    {
        wstring name;
        NamingUri nativeServiceName;
        if (this->TryGetName(nativeServiceName))
        {
            // For all user services, TryGetName should be true. 
            // The NamingService and ClusterManager do not use URI service names.
            name = nativeServiceName.ToString();
        }

        size_t endpointsCount = this->GetEndpointCount();
        vector<ServiceEndpointInformationWrapper> endpoints;
        if (endpointsCount > 0)
        {
            if (this->IsStateful)
            {
                ServiceEndpointInformationWrapper endpoint;
                if (this->Locations.ServiceReplicaSet.IsPrimaryLocationValid)
                {
                    endpoint.Kind = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY;
                    endpoint.Address = this->Locations.ServiceReplicaSet.PrimaryLocation;
                    endpoints.push_back(endpoint);
                }

                endpoint.Kind = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY;
                for (size_t i = 0; i < this->Locations.ServiceReplicaSet.SecondaryLocations.size(); ++i)
                {
                    endpoint.Kind = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY;
                    endpoint.Address = this->Locations.ServiceReplicaSet.SecondaryLocations[i];
                    endpoints.push_back(endpoint);
                }
            }
            else
            {
                ServiceEndpointInformationWrapper endpoint;
                endpoint.Kind = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATELESS;
                for (size_t i = 0; i < this->Locations.ServiceReplicaSet.ReplicaLocations.size(); ++i)
                {
                    endpoint.Address = this->Locations.ServiceReplicaSet.ReplicaLocations[i];
                    endpoints.push_back(endpoint);
                }
            }
        }

        ServicePartitionInformation servicePartitionInfo;
        ResolvedServicePartitionMetadata rspMetadata(
            Locations.ConsistencyUnitId.Guid,
            this->FMVersion,
            storeVersion_,
            generation_);
        wstring version;
        JsonHelper::Serialize(rspMetadata, version);
        servicePartitionInfo.FromPartitionInfo(Locations.ConsistencyUnitId.Guid, PartitionData);

        ResolvedServicePartitionWrapper rsp(
            move(name),
            move(servicePartitionInfo),
            move(endpoints),
            move(version));

        rspWrapper = move(rsp);
    }
    
    bool ResolvedServicePartition::IsMoreRecent(ServiceLocationVersion const & prev)
    {
         return
            (Generation > prev.Generation) ||
            ((Generation == prev.Generation) && (FMVersion > prev.FMVersion)) ||
            (StoreVersion > prev.StoreVersion);
    }

    void ResolvedServicePartition::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << "RSP(serviceName='" << locations_.ServiceName << "'";
        w << ", partition='" << partitionData_ << "'";
        w << ", generation='" << generation_ << "', locations='" << locations_ << "'";
        w << ", ServiceGroup=" << isServiceGroup_;
        w << ")";
    }

    void ResolvedServicePartition::WriteToEtw(uint16 contextSequenceId) const
    {
        ServiceModelEventSource::Trace->RSPTrace(
            contextSequenceId,
            locations_.ServiceName,
            partitionData_.ToString(),
            generation_,
            locations_,
            isServiceGroup_);
    }

    wstring ResolvedServicePartition::ToString() const
    {
        std::wstring product;
        Common::StringWriter writer(product);
        WriteTo(writer, Common::FormatOptions(0, false, ""));
        return product;
    }
}
