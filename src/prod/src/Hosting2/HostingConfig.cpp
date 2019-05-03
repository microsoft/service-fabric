// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DEFINE_SINGLETON_COMPONENT_CONFIG(HostingConfig)

ErrorCode HostingConfig::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_HOSTING_SETTINGS & settings) const
{
    map<wstring, wstring> settingsMap;

    // If you are adding anything new, please put it in alphabetical order!
    settingsMap.insert(make_pair(L"ContainerAppDeploymentRootFolder", this->ContainerAppDeploymentRootFolder));
    settingsMap.insert(make_pair(L"ContainerCleanupScanIntervalInMinutes", wformatString("{0}", this->ContainerCleanupScanIntervalInMinutes)));
    settingsMap.insert(make_pair(L"ContainerDeactivationTimeout", wformatString("{0}", this->ContainerDeactivationTimeout.TotalSeconds())));
    settingsMap.insert(make_pair(L"ContainerDeactivationRetryDelayInSec", wformatString("{0}", this->ContainerDeactivationRetryDelayInSec)));
    settingsMap.insert(make_pair(L"ContainerEventManagerMaxContinuousFailure", wformatString("{0}", this->ContainerEventManagerMaxContinuousFailure)));
    settingsMap.insert(make_pair(L"ContainerEventManagerFailureBackoffInSec", wformatString("{0}", this->ContainerEventManagerFailureBackoffInSec)));
    settingsMap.insert(make_pair(L"ContainerFabricBinRootFolder", this->ContainerFabricBinRootFolder));
    settingsMap.insert(make_pair(L"ContainerFabricLogRootFolder", this->ContainerFabricLogRootFolder));
    settingsMap.insert(make_pair(L"ContainerImageDownloadTimeout", wformatString("{0}", this->ContainerImageDownloadTimeout.TotalSeconds())));
    settingsMap.insert(make_pair(L"ContainerPackageRootFolder", this->ContainerPackageRootFolder));
    settingsMap.insert(make_pair(L"ContainerRepositoryCredentialTokenEndPoint", wformatString("{0}", this->ContainerRepositoryCredentialTokenEndPoint)));
    settingsMap.insert(make_pair(L"ContainerServiceNamedPipeOrUnixSocketAddress", this->ContainerServiceNamedPipeOrUnixSocketAddress));
    settingsMap.insert(make_pair(L"ContainerTerminationMaxRetryCount", wformatString("{0}", this->ContainerTerminationMaxRetryCount)));
    settingsMap.insert(make_pair(L"DeadContainerCleanupUntilInMinutes", wformatString("{0}", this->DeadContainerCleanupUntilInMinutes)));
    settingsMap.insert(make_pair(L"DockerRequestTimeout", wformatString("{0}", this->DockerRequestTimeout.TotalSeconds())));
    settingsMap.insert(make_pair(L"DefaultMSIEndpointForTokenAuthentication", wformatString("{0}", this->DefaultMSIEndpointForTokenAuthentication)));
    settingsMap.insert(make_pair(L"DefaultContainerRepositoryAccountName", wformatString("{0}", this->DefaultContainerRepositoryAccountName)));
    settingsMap.insert(make_pair(L"DefaultContainerRepositoryPassword", wformatString("{0}", this->DefaultContainerRepositoryPassword)));
    settingsMap.insert(make_pair(L"IsDefaultContainerRepositoryPasswordEncrypted", wformatString("{0}", this->IsDefaultContainerRepositoryPasswordEncrypted)));
    settingsMap.insert(make_pair(L"DefaultNatNetwork", wformatString("{0}", this->DefaultNatNetwork)));
    settingsMap.insert(make_pair(L"DefaultContainerNetwork", wformatString("{0}", this->DefaultContainerNetwork)));
    settingsMap.insert(make_pair(L"EnableDockerHealthCheckIntegration", this->EnableDockerHealthCheckIntegration ? L"true" : L"false"));
    settingsMap.insert(make_pair(L"DefaultContainerRepositoryPasswordType", this->DefaultContainerRepositoryPasswordType));
    settingsMap.insert(make_pair(L"DisableDockerRequestRetry", this->DisableDockerRequestRetry ? L"true" : L"false"));
    settingsMap.insert(make_pair(L"LocalNatIpProviderEnabled", this->LocalNatIpProviderEnabled ? L"true" : L"false"));
    settingsMap.insert(make_pair(L"LocalNatIpProviderNetworkName", this->LocalNatIpProviderNetworkName));

    auto nativeSettingMap = heap.AddItem<FABRIC_STRING_PAIR_LIST>();
    auto error = PublicApiHelper::ToPublicApiStringPairList(heap, settingsMap, *nativeSettingMap);
    if (!error.IsSuccess())
    {
        return error;
    }

    settings.SettingsMap = nativeSettingMap.GetRawPointer();

    return ErrorCode::Success();
}
