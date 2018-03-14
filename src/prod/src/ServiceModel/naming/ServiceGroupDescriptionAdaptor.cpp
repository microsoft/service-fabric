// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ServiceModel;
using namespace Common;

HRESULT ServiceGroupDescriptionAdaptor::ToPartitionedServiceDescWrapper()
{
    CServiceGroupDescription cServiceGroupDescription;

    switch (serviceKind_)
    {
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            cServiceGroupDescription.HasPersistedState = FALSE;
            break;
        }
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            cServiceGroupDescription.HasPersistedState = TRUE;
            break;
        }
    default:
        return E_INVALIDARG;
    }

    if (initializationData_.size() != 0)
    {
        cServiceGroupDescription.ServiceGroupInitializationData.insert(
            cServiceGroupDescription.ServiceGroupInitializationData.begin(), 
            initializationData_.begin(),
            initializationData_.begin() + initializationData_.size());
    }

    // Process each member description
    cServiceGroupDescription.ServiceGroupMemberData.resize(serviceGroupMemberDescriptions_.size());
    for (size_t i = 0; i < serviceGroupMemberDescriptions_.size(); i++)
    {
        HRESULT hr = MemberDescriptionToPartitionedServiceDescWrapper(cServiceGroupDescription.ServiceGroupMemberData[i], i);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Clear original data
    initializationData_.clear();
    Common::FabricSerializer::Serialize(&cServiceGroupDescription, initializationData_);

    isServiceGroup_ = true;

    return S_OK;
}

HRESULT ServiceGroupDescriptionAdaptor::MemberDescriptionToPartitionedServiceDescWrapper(__in CServiceGroupMemberDescription &cServiceGroupMemberDescription, size_t index)
{
    cServiceGroupMemberDescription.Identifier = Common::Guid::NewGuid().AsGUID();

    switch (serviceKind_)
    {
    case FABRIC_SERVICE_KIND_STATELESS:
        {
            cServiceGroupMemberDescription.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATELESS;
            break;
        }
    case FABRIC_SERVICE_KIND_STATEFUL:
        {
            cServiceGroupMemberDescription.ServiceDescriptionType = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
            break;
        }
    default:
        return E_INVALIDARG;
    }

    cServiceGroupMemberDescription.ServiceType = serviceGroupMemberDescriptions_[index].getServiceTypeName();
    cServiceGroupMemberDescription.ServiceName = serviceGroupMemberDescriptions_[index].getServiceName();

    if (serviceGroupMemberDescriptions_[index].getInitializationData().size() != 0)
    {
        cServiceGroupMemberDescription.ServiceGroupMemberInitializationData.insert(
                cServiceGroupMemberDescription.ServiceGroupMemberInitializationData.begin(), 
                serviceGroupMemberDescriptions_[index].getInitializationData().begin(),
                serviceGroupMemberDescriptions_[index].getInitializationData().begin() + serviceGroupMemberDescriptions_[index].getInitializationData().size());
    }

    for (ULONG i = 0; i < serviceGroupMemberDescriptions_[index].getMetrics().size(); i++)
    {

        CServiceGroupMemberLoadMetricDescription cServiceGroupMemberLoadMetricDescription;

        cServiceGroupMemberLoadMetricDescription.Name = serviceGroupMemberDescriptions_[index].getMetrics()[i].get_Name();
        cServiceGroupMemberLoadMetricDescription.Weight = serviceGroupMemberDescriptions_[index].getMetrics()[i].get_Weight();
        cServiceGroupMemberLoadMetricDescription.PrimaryDefaultLoad = serviceGroupMemberDescriptions_[index].getMetrics()[i].get_PrimaryDefaultLoad();
        cServiceGroupMemberLoadMetricDescription.SecondaryDefaultLoad = serviceGroupMemberDescriptions_[index].getMetrics()[i].get_SecondaryDefaultLoad();
        
        cServiceGroupMemberDescription.Metrics.push_back(cServiceGroupMemberLoadMetricDescription);
    }

    return S_OK;
}

HRESULT ServiceGroupDescriptionAdaptor::FromPartitionedServiceDescWrapper(CServiceGroupDescription &cServiceGroupDescription)
{
    serviceGroupMemberDescriptions_.resize(cServiceGroupDescription.ServiceGroupMemberData.size());

    for (int i = 0; i < cServiceGroupDescription.ServiceGroupMemberData.size(); i++)
    {
        serviceGroupMemberDescriptions_[i].setServiceName(cServiceGroupDescription.ServiceGroupMemberData[i].ServiceName);
        serviceGroupMemberDescriptions_[i].setServiceTypeName(cServiceGroupDescription.ServiceGroupMemberData[i].ServiceType);

        vector<Reliability::ServiceLoadMetricDescription> serviceLoadMetricDescriptions;
            
        for (int j = 0; j < cServiceGroupDescription.ServiceGroupMemberData[i].Metrics.size(); j++)
        {
            Reliability::ServiceLoadMetricDescription serviceLoadMetricDescription(
                cServiceGroupDescription.ServiceGroupMemberData[i].Metrics[j].Name,
                cServiceGroupDescription.ServiceGroupMemberData[i].Metrics[j].Weight,
                cServiceGroupDescription.ServiceGroupMemberData[i].Metrics[j].PrimaryDefaultLoad,
                cServiceGroupDescription.ServiceGroupMemberData[i].Metrics[j].SecondaryDefaultLoad);
            serviceLoadMetricDescriptions.push_back(serviceLoadMetricDescription);
        }
        serviceGroupMemberDescriptions_[i].setMetrics(serviceLoadMetricDescriptions);
    }

    return S_OK;
}
