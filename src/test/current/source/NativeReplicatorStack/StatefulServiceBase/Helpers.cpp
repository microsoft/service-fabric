// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TXRStatefulServiceBase;

StringLiteral const TraceComponent("Helpers");

GlobalWString Helpers::ReplicatorSettingsConfigPackageName = make_global<wstring>(L"Config");
GlobalWString Helpers::ReplicatorSettingsSectionName = make_global<wstring>(L"ReplicatorConfig");
GlobalWString Helpers::ServiceHttpEndpointResourceName = make_global<wstring>(L"ServiceHttpEndpoint");

GlobalWString ProtocolScheme = make_global<wstring>(L"http");
GlobalWString EndPointType = make_global<wstring>(L"Input");

wstring Helpers::GetFabricNodeIpAddressOrFqdn()
{
    ComPointer<IFabricNodeContextResult> nodeContext;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetNodeContext(nodeContext.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in EnableTracing");

    FABRIC_NODE_CONTEXT const * nodeContextResult = nodeContext->get_NodeContext();

    wstring result(nodeContextResult->IPAddressOrFQDN);

    return result;
}

wstring Helpers::GetWorkingDirectory()
{
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in EnableTracing");

    wstring result(activationContext->get_WorkDirectory());

    return result;
}

void Helpers::EnableTracing()
{
    ComPointer<IFabricNodeContextResult> nodeContext;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetNodeContext(nodeContext.VoidInitializationAddress()) == S_OK,
        "Failed to create fabric runtime");

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in EnableTracing");

    FABRIC_NODE_CONTEXT const * nodeContextResult = nodeContext->get_NodeContext();

    wstring traceFileName = wformatString("{0}_{1}", nodeContextResult->NodeName, Common::Guid::NewGuid().ToString());
    wstring traceFilePath = Path::Combine(activationContext->get_WorkDirectory(), traceFileName);

    TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Noise);
    TraceTextFileSink::SetOption(L"");
    TraceTextFileSink::SetPath(traceFilePath);

    Trace.WriteInfo(
        TraceComponent,
        "Node Name = {0}, IP = {1}",
        nodeContextResult->NodeName,
        nodeContextResult->IPAddressOrFQDN);
}

ComPointer<IFabricReplicatorSettingsResult> Helpers::LoadV1ReplicatorSettings()
{
    ComPointer<IFabricReplicatorSettingsResult> replicatorSettings;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadV1ReplicatorSettings");

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(Helpers::ReplicatorSettingsConfigPackageName);
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(Helpers::ReplicatorSettingsSectionName);

    auto hr = ::FabricLoadReplicatorSettings(
        activationContext.GetRawPointer(),
        configPackageName->get_String(),
        sectionName->get_String(),
        replicatorSettings.InitializationAddress());

    ASSERT_IFNOT(hr == S_OK, "Failed to load replicator settings due to error {0}", hr);

    return replicatorSettings;
}

ErrorCode GetEndpointResourceFromManifest(
    __in wstring const & httpResourceName,
    __in IFabricCodePackageActivationContext & codePackageActivationContext,
    __out ULONG & port)
{
    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = codePackageActivationContext.get_ServiceEndpointResources();

    if (endpoints == NULL ||
        endpoints->Count == 0 ||
        endpoints->Items == NULL)
    {
        return ErrorCode(ErrorCodeValue::InvalidConfiguration);
    }

    // find endpoint by name
    for (ULONG i = 0; i < endpoints->Count; ++i)
    {
        wstring endpointinfo;
        StringWriter writer(endpointinfo);

        const auto & endpoint = endpoints->Items[i];

        ASSERT_IF(endpoint.Name == NULL, "FABRIC_ENDPOINT_RESOURCE_DESCRIPTION with Name == NULL");

        if (StringUtility::AreEqualCaseInsensitive(httpResourceName, endpoint.Name))
        {
            // check protocol is TCP
            if (endpoint.Protocol == NULL ||
                !StringUtility::AreEqualCaseInsensitive(ProtocolScheme->c_str(), endpoint.Protocol))
            {
                return ErrorCode(ErrorCodeValue::InvalidConfiguration);
            }

            // check type is Internal
            if (endpoint.Type == NULL ||
                !StringUtility::AreEqualCaseInsensitive(EndPointType->c_str(), endpoint.Type))
            {
                return ErrorCode(ErrorCodeValue::InvalidConfiguration);
            }

            // extract the useful values
            port = endpoint.Port;

            return ErrorCode::Success();
        }
    }

    return ErrorCode(ErrorCodeValue::InvalidConfiguration);
}

ULONG Helpers::GetHttpPortFromConfigurationPackage()
{
    return GetHttpPortFromConfigurationPackage(ServiceHttpEndpointResourceName->c_str());
}

ULONG Helpers::GetHttpPortFromConfigurationPackage(__in wstring const & endpointResourceName)
{
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadReplicatorSettings");

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(Helpers::ReplicatorSettingsConfigPackageName);
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(Helpers::ReplicatorSettingsSectionName);

    ComPointer<IFabricConfigurationPackage> configPackageCPtr;

    HRESULT hr = activationContext->GetConfigurationPackage(
        Helpers::ReplicatorSettingsConfigPackageName->c_str(), 
        configPackageCPtr.InitializationAddress());

    ASSERT_IFNOT(
        hr == S_OK || configPackageCPtr.GetRawPointer() == nullptr,
        "Failed to get configuration package in GetHttpPortFromConfigurationPackage");

    ULONG port = 0;
    
    ErrorCode error = GetEndpointResourceFromManifest(endpointResourceName, *activationContext.GetRawPointer(), port);

    ASSERT_IFNOT(
        error.IsSuccess(), 
        "Failed to read http resource with error {0}", 
        error);

    return port;

}
