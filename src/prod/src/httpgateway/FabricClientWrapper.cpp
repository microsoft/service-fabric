// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace HttpGateway;
using namespace Client;

StringLiteral const TraceType("FabricClientWrapper");

ErrorCode FabricClientWrapper::CreateFabricClient(
    __in FabricNodeConfigSPtr &config,
    __out FabricClientWrapperSPtr &resultSPtr,
    __in Transport::RoleMask::Enum role)
{
    auto fabricClientWrapperSPtr = FabricClientWrapperSPtr(new FabricClientWrapper(role));

    auto error = fabricClientWrapperSPtr->Initialize(config);
    if (error.IsSuccess())
    {
        resultSPtr = move(fabricClientWrapperSPtr);
    }
    else
    {
        WriteError(TraceType, "FabricClient Initialization failed - {0}", error);
    }

    return error;
}

ErrorCode FabricClientWrapper::Initialize(__in FabricNodeConfigSPtr &config)
{
    IClientFactoryPtr factoryPtr;
    auto error = ClientFactory::CreateLocalClientFactory(config, role_, true, factoryPtr);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateQueryClient(queryClientPtr_);
    if (!error.IsSuccess()) { return error; }

    wstring fabricDataRoot;
    error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
    if (!error.IsSuccess()) { return error; }
    error = Management::ImageStore::NativeImageStore::CreateNativeImageStoreClient(true, fabricDataRoot, factoryPtr, this->nativeImageStoreClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateApplicationManagementClient(appMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateComposeManagementClient(composeAppMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateResourceManagementClient(resourceMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateServiceManagementClient(serviceMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateServiceGroupManagementClient(serviceGroupMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateHealthClient(healthClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateClusterManagementClient(clusterMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateFaultManagementClient(faultMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateSecretStoreClient(secretStoreClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateInternalTokenValidationServiceClient(tvsClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateInfrastructureServiceClient(infraSvcClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateRepairManagementClient(repairMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateTestManagementClient(testMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateSettingsClient(settingsClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreatePropertyManagementClient(propertyMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateNetworkManagementClient(networkMgmtClientPtr_);
    if (!error.IsSuccess()) { return error; }

    error = factoryPtr->CreateGatewayResourceManagerClient(gatewayResourceManagerClientPtr_);
    if (!error.IsSuccess()) { return error; }

    FabricClientSettings settings = settingsClientPtr_->GetSettings();

    auto healthReportSendInterval = HttpGatewayConfig::GetConfig().HttpGatewayHealthReportSendInterval;
    settings.SetHealthReportSendIntervalInSeconds(static_cast<ULONG>(healthReportSendInterval.TotalSeconds()));
    error = settingsClientPtr_->SetSettings(move(settings));
    return error;
}

ErrorCode FabricClientWrapper::SetSecurity(Transport::SecuritySettings && settings)
{
    return settingsClientPtr_->SetSecurity(move(settings));
}
