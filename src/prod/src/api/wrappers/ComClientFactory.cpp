// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

ComClientFactory::ComClientFactory(IClientFactoryPtr const & impl)
    : impl_(impl)
{
}

ComClientFactory::~ComClientFactory()
{
}

HRESULT ComClientFactory::CreateComFabricClient(
    __RPC__in REFIID iid,
    __RPC__deref_out_opt void **fabricClient)
{
    if (iid == IID_IFabricQueryClient)
    {
        IQueryClientPtr queryClientPtr;
        auto error = impl_->CreateQueryClient(queryClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComQueryClient> queryClient = make_com<ComQueryClient>(queryClientPtr);
        return queryClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricClientSettings || iid == IID_IFabricClientSettings2)
    {
        IClientSettingsPtr clientSettingsPtr;
        auto error = impl_->CreateSettingsClient(clientSettingsPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComClientSettings> clientSettings = make_com<ComClientSettings>(clientSettingsPtr);
        return clientSettings->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricApplicationManagementClient || iid == IID_IFabricApplicationManagementClient2)
    {
        IApplicationManagementClientPtr appMgmtClientPtr;
        auto error = impl_->CreateApplicationManagementClient(appMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComApplicationManagementClient> appMgmtClient = make_com<ComApplicationManagementClient>(appMgmtClientPtr);
        return appMgmtClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricClusterManagementClient || iid == IID_IFabricClusterManagementClient2)
    {
        IClusterManagementClientPtr clusterMgmtClientPtr;
        auto error = impl_->CreateClusterManagementClient(clusterMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComClusterManagementClient> clusterMgmtClient = make_com<ComClusterManagementClient>(clusterMgmtClientPtr);
        return clusterMgmtClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricHealthClient)
    {
        IHealthClientPtr healthClientPtr;
        auto error = impl_->CreateHealthClient(healthClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComHealthClient> healthClient = make_com<ComHealthClient>(healthClientPtr);
        return healthClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricPropertyManagementClient || iid == IID_IFabricPropertyManagementClient2)
    {
        IPropertyManagementClientPtr propertyMgmtClientPtr;
        auto error = impl_->CreatePropertyManagementClient(propertyMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComPropertyManagementClient> propertyMgmtClient = make_com<ComPropertyManagementClient>(propertyMgmtClientPtr);
        return propertyMgmtClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricServiceGroupManagementClient || iid == IID_IFabricServiceGroupManagementClient2)
    {
        IServiceGroupManagementClientPtr serviceGrpMgmtClientPtr;
        auto error = impl_->CreateServiceGroupManagementClient(serviceGrpMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComServiceGroupManagementClient> propertyMgmtClient = make_com<ComServiceGroupManagementClient>(serviceGrpMgmtClientPtr);
        return propertyMgmtClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricServiceManagementClient || iid == IID_IFabricServiceManagementClient2)
    {
        IServiceManagementClientPtr serviceMgmtClientPtr;
        auto error = impl_->CreateServiceManagementClient(serviceMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComServiceManagementClient> serviceMgmtClient = make_com<ComServiceManagementClient>(serviceMgmtClientPtr);
        return serviceMgmtClient->QueryInterface(iid, fabricClient);
    }
    else if (iid == IID_IFabricRepairManagementClient || iid == IID_IFabricRepairManagementClient2)
    {
        IRepairManagementClientPtr repairMgmtClientPtr;
        auto error = impl_->CreateRepairManagementClient(repairMgmtClientPtr);
        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<ComRepairManagementClient> repairMgmtClient = make_com<ComRepairManagementClient>(repairMgmtClientPtr);
        return repairMgmtClient->QueryInterface(iid, fabricClient);
    }

    return E_NOINTERFACE;
}
