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

    TraceProvider::GetSingleton()->SetDefaultLevel(TraceSinkType::ETW, LogLevel::Noise);

    Trace.WriteInfo(
        TraceComponent,
        "[Helpers::EnableTracing] NodeName = '{0}', IP = '{1}'",
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


struct ClusterConfig_DnsSettings
{
    uint32_t Size;
    BOOL EnablePartitionedQuery;
    wchar_t* PrefixBuffer;
    uint32_t PrefixBufferSize;
    wchar_t* SuffixBuffer;
    uint32_t SuffixBufferSize;
};

struct ClusterConfig_DnsSettings_V1
{
    uint32_t Size;
    BOOL EnablePartitionedQuery;
    wchar_t* PrefixBuffer;
    uint32_t PrefixBufferSize;
    wchar_t* SuffixBuffer;
    uint32_t SuffixBufferSize;
};

extern "C" __declspec(dllexport) HRESULT ClusterConfig_GetDnsSettings(__inout ClusterConfig_DnsSettings* dnsSettings)
{
   if (dnsSettings == nullptr)
       return E_POINTER;

    DNS::DnsServiceConfig& config = DNS::DnsServiceConfig::GetConfig();

    if (dnsSettings->Size >= sizeof(ClusterConfig_DnsSettings_V1))
    {
        dnsSettings->EnablePartitionedQuery = config.EnablePartitionedQuery;

        wstring PartitionPrefix = config.PartitionPrefix;
        uint32_t prefixSize = static_cast<uint32_t>(PartitionPrefix.length()) + 1;
        if (dnsSettings->PrefixBuffer != nullptr)
        {
            errno_t error = wcscpy_s(dnsSettings->PrefixBuffer, dnsSettings->PrefixBufferSize, PartitionPrefix.c_str());
            if (error != 0)
            {
                Trace.WriteError(
                    TraceComponent,
                    "[ClusterConfig_GetDnsSettings] Failed to copy PartitionPrefix '{0}' to provided buffer. Error: '{1}'",
                    PartitionPrefix,
                    error);
                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
        {
            dnsSettings->PrefixBufferSize = prefixSize;
        }

        wstring PartitionSuffix = config.PartitionSuffix;
        uint32_t suffixSize = static_cast<uint32_t>(PartitionSuffix.length()) + 1;
        if (dnsSettings->SuffixBuffer != nullptr)
        {
            errno_t error = wcscpy_s(dnsSettings->SuffixBuffer, dnsSettings->SuffixBufferSize, PartitionSuffix.c_str());
            if (error != 0)
            {
                Trace.WriteError(
                    TraceComponent,
                    "[ClusterConfig_GetDnsSettings] Failed to copy PartitionSuffix '{0}' to provided buffer. Error: '{1}'",
                    PartitionSuffix,
                    error);
                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
        {
            dnsSettings->SuffixBufferSize = suffixSize;
        }
    }

    return S_OK;
}

