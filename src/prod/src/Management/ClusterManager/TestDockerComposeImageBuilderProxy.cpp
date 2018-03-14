// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Transport;

using namespace Management::ImageModel;
using namespace Management::ClusterManager;

StringLiteral const DockerComposeIBComponent("DockerComposeImageBuilderProxy");

TestDockerComposeImageBuilderProxy::TestDockerComposeImageBuilderProxy(
    wstring const & imageBuilderExeDirectory,
    wstring const & workingDirectory,
    wstring const & nodeName,
    Federation::NodeInstance const & nodeInstance)
    : ImageBuilderProxy(
        imageBuilderExeDirectory,
        workingDirectory,
        nodeName,
        nodeInstance)
{
}

shared_ptr<TestDockerComposeImageBuilderProxy> TestDockerComposeImageBuilderProxy::Create(
    wstring const & imageBuilderExeDirectory,
    wstring const & workingDirectory,
    wstring const & nodeName,
    Federation::NodeInstance const & nodeInstance)
{
    auto proxy = shared_ptr<TestDockerComposeImageBuilderProxy>(new TestDockerComposeImageBuilderProxy(
        imageBuilderExeDirectory,
        workingDirectory,
        nodeName,
        nodeInstance));

    proxy->Initialize();

    return proxy;
}

ErrorCode TestDockerComposeImageBuilderProxy::ValidateComposeFile(
    ByteBuffer const &composeFile,
    NamingUri const &appName,
    ServiceModelTypeName const &applicationTypeName,
    ServiceModelVersion const &applicationTypeVersion,
    TimeSpan const &timeout)
{
    UNREFERENCED_PARAMETER(composeFile);
    UNREFERENCED_PARAMETER(appName);
    UNREFERENCED_PARAMETER(applicationTypeName);
    UNREFERENCED_PARAMETER(applicationTypeVersion);
    UNREFERENCED_PARAMETER(timeout);

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr TestDockerComposeImageBuilderProxy::BeginBuildComposeDeploymentType(
    ActivityId const & activityId,
    Common::ByteBufferSPtr const &composeFile,
    Common::ByteBufferSPtr const &overridesFile,
    std::wstring const &registryUserName,
    std::wstring const &registryPassword,
    bool isPasswordEncrypted,
    NamingUri const &appName,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &typeVersion,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(composeFile);
    UNREFERENCED_PARAMETER(overridesFile);
    UNREFERENCED_PARAMETER(registryUserName);
    UNREFERENCED_PARAMETER(registryPassword);
    UNREFERENCED_PARAMETER(isPasswordEncrypted);
    UNREFERENCED_PARAMETER(appName);

    auto buildPathIter = mockAppBuildPaths_.find(appName.ToString());
    ASSERT_IF(buildPathIter == mockAppBuildPaths_.end(), "Mock AppBuildPaths does not contain entry {0}", appName);

    // we can add behaviors here to return various error codes
    wstring mockAppBuildPath = buildPathIter->second;

    return this->BeginBuildApplicationType(
        activityId,
        mockAppBuildPath,
        wstring(),
        typeName,
        typeVersion,
        [this](wstring const & details) { UNREFERENCED_PARAMETER(details); },
        timeout,
        callback,
        parent);
}

ErrorCode TestDockerComposeImageBuilderProxy::EndBuildComposeDeploymentType(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring &,
    __out wstring & applicationManifestContent,
    __out ServiceModel::ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList,
    __out wstring &mergedComposeFile)
{
    mergedComposeFile = L"dummy file";
    wstring dummyApplicationManifestId;

    return this->EndBuildApplicationType(operation, serviceManifests, dummyApplicationManifestId, applicationManifestContent, healthPolicy, defaultParamList);
}

AsyncOperationSPtr TestDockerComposeImageBuilderProxy::BeginBuildComposeApplicationTypeForUpgrade(
    ActivityId const & activityId,
    ByteBufferSPtr const & composeFile,
    ByteBufferSPtr const & overridesFile,
    wstring const & registryUserName,
    wstring const & registryPassword,
    bool isPasswordEncrypted,
    Common::NamingUri const & appName,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & currentTypeVersion,
    ServiceModelVersion const & targetTypeVersion,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(currentTypeVersion);

    return this->BeginBuildComposeDeploymentType(
        activityId,
        composeFile,
        overridesFile,
        registryUserName,
        registryPassword,
        isPasswordEncrypted,
        appName,
        typeName,
        targetTypeVersion,
        timeout,
        callback,
        parent);
}

ErrorCode TestDockerComposeImageBuilderProxy::EndBuildComposeApplicationTypeForUpgrade(
    AsyncOperationSPtr const & operation,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests,
    __out wstring & applicationManifestId,
    __out wstring & applicationManifestContent,
    __out ApplicationHealthPolicy & healthPolicy,
    __out map<wstring, wstring> & defaultParamList,
    __out wstring & mergedComposeFile)
{
    return this->EndBuildComposeDeploymentType(
        operation,
        serviceManifests,
        applicationManifestId,
        applicationManifestContent,
        healthPolicy,
        defaultParamList,
        mergedComposeFile);
}
