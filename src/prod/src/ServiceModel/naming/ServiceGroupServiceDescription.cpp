// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{ 
    using namespace std;
    using namespace Common;

    ServiceGroupServiceDescription::ServiceGroupServiceDescription()
    {
    }

    HRESULT ServiceGroupServiceDescription::FromServiceGroupDescription(FABRIC_SERVICE_GROUP_DESCRIPTION const & description)
    {
        if (description.Description == NULL)
        {
            return E_POINTER;
        }

        serviceDescription_ = heap_.AddItem<FABRIC_SERVICE_DESCRIPTION>();

        // create the description for the service group service
        HRESULT hr = ProcessServiceDescription(description);
        if (FAILED(hr)) { return hr; }

        // process the members and extract loadmetrics and move cost
        hr = ProcessMembers(description);
        if (FAILED(hr)) { return hr; }

        // set the serialized service group description as initialization data on the service description
        hr = UpdateInitializationData();
        if (FAILED(hr)) { return hr; }

        // set the aggregated load metrics
        UpdateLoadMetrics();
        
        return S_OK;
    }

    HRESULT ServiceGroupServiceDescription::ProcessServiceDescription(FABRIC_SERVICE_GROUP_DESCRIPTION const & description)
    {
        ULONG initializationDataSize = 0;
        byte* initializationData = NULL;

        std::set<NamingUri> serviceInstanceNames;

        memcpy_s(
            serviceDescription_.GetRawPointer(),
            sizeof(FABRIC_SERVICE_DESCRIPTION),
            description.Description,
            sizeof(FABRIC_SERVICE_DESCRIPTION)
            );

        switch (description.Description->Kind)
        {
        case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
            {
                auto statelessServiceDescription = heap_.AddItem<FABRIC_STATELESS_SERVICE_DESCRIPTION>();

                memcpy_s(
                    statelessServiceDescription.GetRawPointer(),
                    sizeof(FABRIC_STATELESS_SERVICE_DESCRIPTION),
                    description.Description->Value,
                    sizeof(FABRIC_STATELESS_SERVICE_DESCRIPTION)
                    );

                initializationData = statelessServiceDescription->InitializationData;
                initializationDataSize = statelessServiceDescription->InitializationDataSize;

                if (NULL == statelessServiceDescription->ServiceTypeName || 
                    NULL == statelessServiceDescription->ServiceName)
                {
                    return E_POINTER;
                }

                if (!NamingUri::TryParse(statelessServiceDescription->ServiceName, "ServiceName", nameServiceGroupUri_).IsSuccess() ||
                    !nameServiceGroupUri_.Fragment.empty())
                {
                    return E_INVALIDARG;
                }

                serviceGroupDescription_.HasPersistedState = FALSE;

                serviceDescription_->Value = statelessServiceDescription.GetRawPointer();
            }
            break;

        case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
            {
                auto statefulServiceDescription = heap_.AddItem<FABRIC_STATEFUL_SERVICE_DESCRIPTION>();

                memcpy_s(
                    statefulServiceDescription.GetRawPointer(),
                    sizeof(FABRIC_STATEFUL_SERVICE_DESCRIPTION),
                    description.Description->Value,
                    sizeof(FABRIC_STATEFUL_SERVICE_DESCRIPTION)
                    );

                initializationData = statefulServiceDescription->InitializationData;
                initializationDataSize = statefulServiceDescription->InitializationDataSize;

                if (NULL == statefulServiceDescription->ServiceTypeName || 
                    NULL == statefulServiceDescription->ServiceName)
                {

                    return E_POINTER;
                }

                if (!NamingUri::TryParse(statefulServiceDescription->ServiceName, "ServiceName", nameServiceGroupUri_).IsSuccess() ||
                    !nameServiceGroupUri_.Fragment.empty())
                {

                    return E_INVALIDARG;
                }

                serviceGroupDescription_.HasPersistedState = statefulServiceDescription->HasPersistedState;

                serviceDescription_->Value = statefulServiceDescription.GetRawPointer();
            }
            break;

        default: 
            return E_INVALIDARG;
        }

        if ((0 == initializationDataSize && NULL != initializationData) ||
            (0 != initializationDataSize && NULL == initializationData))
        {

            return E_INVALIDARG;
        }

        if (0 != initializationDataSize)
        {
            serviceGroupDescription_.ServiceGroupInitializationData.insert(
                serviceGroupDescription_.ServiceGroupInitializationData.begin(), 
                initializationData,
                initializationData + initializationDataSize);
        }

        return S_OK;
    }

    HRESULT ServiceGroupServiceDescription::ProcessMembers(FABRIC_SERVICE_GROUP_DESCRIPTION const & description)
    {
        if ((0 == description.MemberCount && NULL != description.MemberDescriptions) ||
            (0 != description.MemberCount && NULL == description.MemberDescriptions) ||
            (0 == description.MemberCount))
        {
            return E_INVALIDARG;
        }

        serviceGroupDescription_.ServiceGroupMemberData.resize(description.MemberCount);

        for (size_t index = 0; index < serviceGroupDescription_.ServiceGroupMemberData.size(); index++)
        {
            HRESULT hr = ProcessMemberDescription(description.MemberDescriptions[index], index);
            if (FAILED(hr)) { return hr; }
        }

        return S_OK;
    }

    HRESULT ServiceGroupServiceDescription::ProcessMemberDescription(FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION const & memberDescription, size_t memberIndex)
    {
        NamingUri memberName;

        //
        // Check input.
        //
        if ((0 == memberDescription.InitializationDataSize && NULL != memberDescription.InitializationData) ||
            (0 != memberDescription.InitializationDataSize && NULL == memberDescription.InitializationData))
        {
            return E_INVALIDARG;
        }

        if ((0 == memberDescription.MetricCount && NULL != memberDescription.Metrics) ||
            (0 != memberDescription.MetricCount && NULL == memberDescription.Metrics))
        {
            return E_INVALIDARG;
        }

        if (NULL == memberDescription.ServiceType || NULL == memberDescription.ServiceName)
        {
            return E_POINTER;
        }

        if (!NamingUri::TryParse(memberDescription.ServiceName, "ServiceName", memberName).IsSuccess())
        {
            return E_INVALIDARG;
        }

        if (!StringUtility::AreEqualCaseInsensitive(memberName.Authority, nameServiceGroupUri_.Authority) ||
            !StringUtility::AreEqualCaseInsensitive(memberName.Path, nameServiceGroupUri_.Path) ||
            memberName.Fragment.empty())
        {
            return E_INVALIDARG;
        }

        if (serviceInstanceNames_.end() != serviceInstanceNames_.find(memberName))
        {
            return E_INVALIDARG;
        }

        serviceInstanceNames_.insert(memberName);

        //
        // Populate each service group member from caller's input.
        //
        serviceGroupDescription_.ServiceGroupMemberData[memberIndex].Identifier = Common::Guid::NewGuid().AsGUID();
        serviceGroupDescription_.ServiceGroupMemberData[memberIndex].ServiceDescriptionType = serviceDescription_->Kind;
        serviceGroupDescription_.ServiceGroupMemberData[memberIndex].ServiceType = memberDescription.ServiceType; 
        serviceGroupDescription_.ServiceGroupMemberData[memberIndex].ServiceName = memberName.Name; 
        
        if (0 != memberDescription.InitializationDataSize)
        {
            serviceGroupDescription_.ServiceGroupMemberData[memberIndex].ServiceGroupMemberInitializationData.insert(
                serviceGroupDescription_.ServiceGroupMemberData[memberIndex].ServiceGroupMemberInitializationData.begin(),
                memberDescription.InitializationData,
                memberDescription.InitializationData + 
                memberDescription.InitializationDataSize);
        }

        HRESULT hr = S_OK;
        //
        // Add LoadMetrics
        //
        for (ULONG metricIndex = 0; metricIndex < memberDescription.MetricCount; ++metricIndex)
        {
            // aggregate in group description
            hr = AddLoadMetric(memberDescription.Metrics[metricIndex]);
            if (FAILED(hr)) { return hr; }

            // add to member description
            ServiceModel::CServiceGroupMemberLoadMetricDescription metricDescription;
            
            hr = StringUtility::LpcwstrToWstring(memberDescription.Metrics[metricIndex].Name, true /*acceptNull*/, metricDescription.Name);
            if (FAILED(hr)) { return hr; }
                
            metricDescription.Weight = memberDescription.Metrics[metricIndex].Weight;
            metricDescription.PrimaryDefaultLoad = memberDescription.Metrics[metricIndex].PrimaryDefaultLoad;
            metricDescription.SecondaryDefaultLoad = memberDescription.Metrics[metricIndex].SecondaryDefaultLoad;

            serviceGroupDescription_.ServiceGroupMemberData[memberIndex].Metrics.push_back(metricDescription);
        }

        return S_OK;
    }

    HRESULT ServiceGroupServiceDescription::AddLoadMetric(__in FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION const & loadMetric)
    {
        wstring name;
        HRESULT hr = StringUtility::LpcwstrToWstring(loadMetric.Name, false /*acceptNull*/, name);
        if (FAILED(hr)) { return hr; }

        if (name.empty())
        {
            return E_INVALIDARG;
        }

        auto it = loadMetrics_.find(name);
        if (it == end(loadMetrics_))
        {
            loadMetrics_[name].PrimaryDefaultLoad = loadMetric.PrimaryDefaultLoad;
            loadMetrics_[name].SecondaryDefaultLoad = loadMetric.SecondaryDefaultLoad;
            loadMetrics_[name].Weight = loadMetric.Weight;
            loadMetrics_[name].Name = NULL; // populated in UpdateLoadMetrics (no need to copy the string twice)
        }
        else
        {
            if (it->second.PrimaryDefaultLoad > ~loadMetric.PrimaryDefaultLoad)
            {
                // aggregated default load would overflow
                return E_INVALIDARG;
            }

            if (it->second.SecondaryDefaultLoad > ~loadMetric.SecondaryDefaultLoad)
            {
                // aggregated default load would overflow
                return E_INVALIDARG;
            }

            it->second.PrimaryDefaultLoad += loadMetric.PrimaryDefaultLoad;
            it->second.SecondaryDefaultLoad += loadMetric.SecondaryDefaultLoad;
            it->second.Weight = max(it->second.Weight, loadMetric.Weight);
        }

        return S_OK;
    }

    HRESULT ServiceGroupServiceDescription::UpdateInitializationData()
    {
        ULONG newInitializationDataSize = 0;

        FabricSerializer::Serialize(&serviceGroupDescription_, serializedServiceGroupDescription_);
        HRESULT hr = ::SizeTToULong(serializedServiceGroupDescription_.size(), &newInitializationDataSize);

        if (FAILED(hr))
        {
            return hr;
        }

        switch (serviceDescription_->Kind)
        {
        case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
            {
                FABRIC_STATELESS_SERVICE_DESCRIPTION * statelessServiceDescription = (FABRIC_STATELESS_SERVICE_DESCRIPTION*)serviceDescription_->Value;
                statelessServiceDescription->InitializationDataSize = newInitializationDataSize;
                statelessServiceDescription->InitializationData = serializedServiceGroupDescription_.data();
            }
            break;

        case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
            {
                FABRIC_STATEFUL_SERVICE_DESCRIPTION * statefulServiceDescription = (FABRIC_STATEFUL_SERVICE_DESCRIPTION*)serviceDescription_->Value;
                statefulServiceDescription->InitializationDataSize = newInitializationDataSize;
                statefulServiceDescription->InitializationData = serializedServiceGroupDescription_.data();
            }
            break;
        }

        return S_OK;
    }

    void ServiceGroupServiceDescription::UpdateLoadMetrics()
    {
        if (loadMetrics_.size() == 0)
        {
            return;
        }

        auto metrics = heap_.AddArray<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION>(loadMetrics_.size());

        ULONG index = 0;
        for (auto it = begin(loadMetrics_); it != end(loadMetrics_); ++it, ++index)
        {
            metrics[index].PrimaryDefaultLoad = it->second.PrimaryDefaultLoad;
            metrics[index].SecondaryDefaultLoad = it->second.SecondaryDefaultLoad;
            metrics[index].Weight = it->second.Weight;

            metrics[index].Name = heap_.AddString(it->first);
        }

        switch (serviceDescription_->Kind)
        {
        case FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS:
            {
                FABRIC_STATELESS_SERVICE_DESCRIPTION * statelessServiceDescription = (FABRIC_STATELESS_SERVICE_DESCRIPTION*)serviceDescription_->Value;
                if (statelessServiceDescription->MetricCount == 0)
                {
                    statelessServiceDescription->MetricCount = static_cast<ULONG>(loadMetrics_.size());
                    statelessServiceDescription->Metrics = metrics.GetRawArray();
                }
            }
            break;

        case FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL:
            {
                FABRIC_STATEFUL_SERVICE_DESCRIPTION * statefulServiceDescription = (FABRIC_STATEFUL_SERVICE_DESCRIPTION*)serviceDescription_->Value;
                if (statefulServiceDescription->MetricCount == 0)
                {
                    statefulServiceDescription->MetricCount = static_cast<ULONG>(loadMetrics_.size());
                    statefulServiceDescription->Metrics = metrics.GetRawArray();
                }
            }
            break;
        }
    }

    FABRIC_SERVICE_DESCRIPTION * ServiceGroupServiceDescription::GetRawPointer()
    {
        return serviceDescription_.GetRawPointer();
    }
} 
