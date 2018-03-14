// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Hosting2;

DEFINE_SINGLETON_COMPONENT_CONFIG(HostingConfig)

void HostingConfig::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_HOSTING_SETTINGS & settings) const
{
    settings.ContainerPackageRootFolder = heap.AddString(this->ContainerPackageRootFolder);
    settings.ContainerFabricBinRootFolder = heap.AddString(this->ContainerFabricBinRootFolder);
    settings.ContainerFabricLogRootFolder = heap.AddString(this->ContainerFabricLogRootFolder);
    settings.ContainerAppDeploymentRootFolder = heap.AddString(this->ContainerAppDeploymentRootFolder);
    settings.DockerRequestTimeout = this->DockerRequestTimeout.TotalSeconds();
    settings.ContainerImageDownloadTimeout = this->ContainerImageDownloadTimeout.TotalSeconds();
    settings.ContainerDeactivationTimeout = this->ContainerDeactivationTimeout.TotalSeconds();
    settings.ContainerDeactivationRetryDelayInSec = this->ContainerDeactivationRetryDelayInSec;
    settings.ContainerTerminationMaxRetryCount = this->ContainerTerminationMaxRetryCount;
    settings.ContainerEventManagerMaxContinuousFailure = this->ContainerEventManagerMaxContinuousFailure;
    settings.ContainerEventManagerFailureBackoffInSec = this->ContainerEventManagerFailureBackoffInSec;
    settings.EnableDockerHealthCheckIntegration = this->EnableDockerHealthCheckIntegration;
    settings.ContainerServiceNamedPipeOrUnixSocketAddress = heap.AddString(this->ContainerServiceNamedPipeOrUnixSocketAddress);
}



