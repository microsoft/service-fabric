// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;

    StringLiteral const FabricClientTraceType("FabricClient");

    ServiceGroupDescriptor::ServiceGroupDescriptor()
        : serviceGroupDescription_()
        , serviceDescriptor_()
    {
    }

    ErrorCode ServiceGroupDescriptor::Create(__in PartitionedServiceDescriptor const & serviceDescription, __out ServiceGroupDescriptor & result)
    {
        // unpack the CServiceGroupDescription store in the intialization data of the serviceDescription
        std::vector<byte> initData(serviceDescription.Service.InitializationData);
        ErrorCode error = FabricSerializer::Deserialize(result.serviceGroupDescription_, initData);

        if (error.IsSuccess())
        {
            // replace the services InitializationData with the actual initialization data of the service group
            PartitionedServiceDescriptor svcGrpServiceDescriptor(serviceDescription, std::move(result.serviceGroupDescription_.ServiceGroupInitializationData));
            result.serviceDescriptor_ = std::move(svcGrpServiceDescriptor);
        }

        return error;
    }
        
    void ServiceGroupDescriptor::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_DESCRIPTION & serviceGroupDescription) const
    {
        // service description for service group
        auto serviceDescription = heap.AddItem<FABRIC_SERVICE_DESCRIPTION>();
        serviceDescriptor_.ToPublicApi(heap, *serviceDescription);

        serviceGroupDescription.Description = serviceDescription.GetRawPointer();

        // service member descriptions
        serviceGroupDescription.MemberCount = static_cast<ULONG>(serviceGroupDescription_.ServiceGroupMemberData.size());
        auto members = heap.AddArray<FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION>(serviceGroupDescription_.ServiceGroupMemberData.size()).GetRawArray();
        serviceGroupDescription.MemberDescriptions = members;

        for (unsigned int i = 0; i < serviceGroupDescription.MemberCount; ++i)
        {
            members[i].ServiceName = heap.AddString(serviceGroupDescription_.ServiceGroupMemberData[i].ServiceName);
            members[i].ServiceType = heap.AddString(serviceGroupDescription_.ServiceGroupMemberData[i].ServiceType);

            if (serviceGroupDescription_.ServiceGroupMemberData[i].ServiceGroupMemberInitializationData.size() > 0)
            {
                members[i].InitializationDataSize = static_cast<ULONG>(serviceGroupDescription_.ServiceGroupMemberData[i].ServiceGroupMemberInitializationData.size());

                auto initData = heap.AddArray<byte>(members[i].InitializationDataSize).GetRawArray();
                members[i].InitializationData = initData;

                copy_n_checked(
                    begin(serviceGroupDescription_.ServiceGroupMemberData[i].ServiceGroupMemberInitializationData),
                    members[i].InitializationDataSize,
                    initData,
                    members[i].InitializationDataSize);
            }
            else
            {
                members[i].InitializationData = nullptr;
                members[i].InitializationDataSize = 0;
            }

            if (serviceGroupDescription_.ServiceGroupMemberData[i].Metrics.size() > 0)
            {
                members[i].MetricCount = static_cast<ULONG>(serviceGroupDescription_.ServiceGroupMemberData[i].Metrics.size());

                members[i].Metrics = heap.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(members[i].MetricCount).GetRawArray();

                for (unsigned int j = 0; j < members[i].MetricCount; ++j)
                {
                    members[i].Metrics[j].Name = heap.AddString(serviceGroupDescription_.ServiceGroupMemberData[i].Metrics[j].Name);
                    members[i].Metrics[j].Weight = serviceGroupDescription_.ServiceGroupMemberData[i].Metrics[j].Weight;
                    members[i].Metrics[j].PrimaryDefaultLoad = serviceGroupDescription_.ServiceGroupMemberData[i].Metrics[j].PrimaryDefaultLoad;
                    members[i].Metrics[j].SecondaryDefaultLoad = serviceGroupDescription_.ServiceGroupMemberData[i].Metrics[j].SecondaryDefaultLoad;
                }
            }
            else
            {
                members[i].Metrics = nullptr;
                members[i].MetricCount = 0;
            }
        }

        serviceGroupDescription.MemberDescriptions = members;
        serviceGroupDescription.MemberCount = static_cast<ULONG>(serviceGroupDescription_.ServiceGroupMemberData.size());
    }

    ErrorCode ServiceGroupDescriptor::GetServiceDescriptorForMember(__in NamingUri const & name, __out PartitionedServiceDescriptor & memberServiceDescription)
    {
        for (auto it = begin(serviceGroupDescription_.ServiceGroupMemberData); it != end(serviceGroupDescription_.ServiceGroupMemberData); ++it)
        {
            NamingUri memberName;
            if (!NamingUri::TryParse(it->ServiceName, memberName))
            {
                Trace.WriteWarning(FabricClientTraceType, "Could not parse ServiceName '{0}'", it->ServiceName);
                return ErrorCode(ErrorCodeValue::InvalidNameUri);
            }

            if (name == memberName)
            {
                // construct the member service description by replacing 
                //      ServiceName
                //      ServiceType
                //      InitializationData
                //      Metrics
                // in the ServiceGroup service description

                ServiceModel::ServiceTypeIdentifier type;
                ErrorCode error = ServiceModel::ServiceTypeIdentifier::FromString(it->ServiceType, type);
                if (!error.IsSuccess()) 
                {
                    Trace.WriteWarning(FabricClientTraceType, "Could not parse ServiceType '{0}'", it->ServiceType);
                    return error;
                }

                std::wstring serviceName = it->ServiceName;
                std::vector<byte> initData = it->ServiceGroupMemberInitializationData;

                std::vector<Reliability::ServiceLoadMetricDescription> loadMetrics;

                for (auto metricIt = begin(it->Metrics); metricIt != end(it->Metrics); ++metricIt)
                {
                    Reliability::ServiceLoadMetricDescription metric(
                        metricIt->Name, 
                        metricIt->Weight, 
                        metricIt->PrimaryDefaultLoad,
                        metricIt->SecondaryDefaultLoad);

                    loadMetrics.push_back(std::move(metric));
                }

                memberServiceDescription = PartitionedServiceDescriptor(
                    serviceDescriptor_, 
                    std::move(serviceName), 
                    std::move(type), 
                    std::move(initData), 
                    std::move(loadMetrics), 
                    0);

                return ErrorCode::Success();
            }
        }

        Trace.WriteWarning(FabricClientTraceType, "Could not find service group member description for '{0}'", name);
        return ErrorCode(ErrorCodeValue::NameNotFound);
    }
}
