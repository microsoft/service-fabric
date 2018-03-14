// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("ComposeDeploymentUpgradeDescription");

ComposeDeploymentUpgradeDescription::ComposeDeploymentUpgradeDescription(
    std::wstring &&deploymentName,
    std::wstring &&applicationName,
    std::vector<Common::ByteBuffer> &&composeFiles,
    std::vector<Common::ByteBuffer> &&sfSettingsFiles,
    std::wstring &&repositoryUserName,
    std::wstring &&repositoryPassword,
    bool isPasswordEncrypted,
    ServiceModel::UpgradeType::Enum upgradeType,
    ServiceModel::RollingUpgradeMode::Enum upgradeMode,
    bool forceRestart,
    ServiceModel::RollingUpgradeMonitoringPolicy &&monitoringPolicy,
    bool isHealthPolicyValid,
    ServiceModel::ApplicationHealthPolicy &&healthPolicy,
    Common::TimeSpan const &replicaSetCheckTimeout)
    : deploymentName_(move(deploymentName))
    , applicationName_(move(applicationName))
    , composeFiles_(move(composeFiles))
    , sfSettingsFiles_(move(sfSettingsFiles))
    , repositoryUserName_(move(repositoryUserName))
    , repositoryPassword_(move(repositoryPassword))
    , isPasswordEncrypted_(isPasswordEncrypted)
    , upgradeType_(upgradeType)
    , rollingUpgradeMode_(upgradeMode)
    , forceRestart_(forceRestart)
    , monitoringPolicy_(move(monitoringPolicy))
    , healthPolicy_(move(healthPolicy))
    , isHealthPolicyValid_(isHealthPolicyValid)
    , replicaSetCheckTimeout_(replicaSetCheckTimeout)
{}

ErrorCode ComposeDeploymentUpgradeDescription::FromPublicApi(FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION const &publicDesc)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDesc.DeploymentName, false, deploymentName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(publicDesc.ContainerRegistryUserName, true, repositoryUserName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(publicDesc.ContainerRegistryPassword, true, repositoryPassword_);
    if (!error.IsSuccess()) { return error; }

    isPasswordEncrypted_ = !!publicDesc.IsPasswordEncrypted;

    composeFiles_.reserve(publicDesc.ComposeFilePaths.Count);
    for (unsigned int i = 0; i < publicDesc.ComposeFilePaths.Count; ++i)
    {
        wstring filePath;
        error = StringUtility::LpcwstrToWstring2(publicDesc.ComposeFilePaths.Items[i], false, filePath);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(
                TraceComponent, 
                "Failed to parse compose file path, Error: {0}",
                error);
            return error;
        }

        ByteBuffer fileContents;
        error = GetFileContents(filePath, fileContents);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(
                TraceComponent, 
                "Failed to read from Compose file '{0}', Error: {1}",
                filePath,
                error);
            return error;
        }

        composeFiles_.push_back(move(fileContents));
    }

    sfSettingsFiles_.reserve(publicDesc.ServiceFabricSettingsFilePaths.Count);
    for (unsigned int i = 0; i < publicDesc.ServiceFabricSettingsFilePaths.Count; ++i)
    {
        wstring filePath;
        error = StringUtility::LpcwstrToWstring2(publicDesc.ServiceFabricSettingsFilePaths.Items[i], false, filePath);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(
                TraceComponent, 
                "Failed to parse settings file path, Error: {0}",
                error);
            return error;
        }

        ByteBuffer fileContents;
        error = GetFileContents(filePath, fileContents);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(
                TraceComponent, 
                "Failed to read from SF Settings file '{0}', Error: {1}",
                filePath,
                error);

            return error;
        }

        sfSettingsFiles_.push_back(move(fileContents));
    }

    // TODO : Make this common code between all upgrade input parsing.
    upgradeType_ = UpgradeType::Rolling;
    auto policyDescription = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION *>(publicDesc.UpgradePolicyDescription);

    switch (policyDescription->RollingUpgradeMode)
    {
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredAuto;
            break;

        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredManual;
            break;

        case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
            rollingUpgradeMode_ = RollingUpgradeMode::Monitored;

            if (policyDescription->Reserved != NULL)
            {
                auto policyDescriptionEx = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription->Reserved);

                if (policyDescriptionEx != NULL)
                {
                    if (policyDescriptionEx->MonitoringPolicy != NULL)
                    {
                        error = monitoringPolicy_.FromPublicApi(*(policyDescriptionEx->MonitoringPolicy));
                        if (!error.IsSuccess()) { return error; }
                    }

                    if (policyDescriptionEx->HealthPolicy != NULL)
                    {
                        auto healthPolicy = reinterpret_cast<FABRIC_APPLICATION_HEALTH_POLICY*>(policyDescriptionEx->HealthPolicy);
                        error = healthPolicy_.FromPublicApi(*healthPolicy);
                        if (!error.IsSuccess()) { return error; }
                        isHealthPolicyValid_ = true;
                    }
                }
            }
            break;

        default:
            return ErrorCodeValue::InvalidArgument;
    }

    forceRestart_ = (policyDescription->ForceRestart == TRUE) ? true : false;
    replicaSetCheckTimeout_ = TimeSpan::FromSeconds(policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds);

    return ErrorCodeValue::Success;
}

ErrorCode ComposeDeploymentUpgradeDescription::Validate()
{
    if ((rollingUpgradeMode_ != RollingUpgradeMode::Monitored) &&
        (rollingUpgradeMode_ != RollingUpgradeMode::UnmonitoredAuto) &&
        (rollingUpgradeMode_ != RollingUpgradeMode::UnmonitoredManual))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ComposeDeploymentUpgradeDescription::GetFileContents(wstring const &filePath, __out ByteBuffer &fileContents)
{
    File composeFile;
    auto error = composeFile.TryOpen(
        filePath,
        FileMode::Open,
        FileAccess::Read,
        FileShare::Read,
        FileAttributes::SequentialScan);

    if (!error.IsSuccess()) { return error; }

    size_t fileSize = composeFile.size();

    DWORD bytesRead;
    int size = static_cast<int>(fileSize);
    ByteBuffer temp(size);
    error = composeFile.TryRead2(temp.data(), size, bytesRead);

    if (!error.IsSuccess()) { return error; }

    fileContents = move(temp);
    return error;
}
