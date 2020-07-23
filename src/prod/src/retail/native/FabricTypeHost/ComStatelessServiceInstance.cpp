// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTypeHost;
using namespace ServiceModel;

StringLiteral const TraceType("FabricTypeHost.StatelessServiceInstance");

ComStatelessServiceInstance::ComStatelessServiceInstance(
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    ::FABRIC_INSTANCE_ID instanceId) :
    IFabricStatelessServiceInstance(),
    ComUnknownBase(),
    serviceName_(serviceName),
    serviceTypeName_(serviceTypeName),
    partitionId_(partitionId),
    instanceId_(instanceId),
    id_(wformatString("{0}:{1}:{2}",serviceTypeName, partitionId, instanceId)),
    endpoints_()
{
    WriteNoise(TraceType, id_, "ctor: {0}", serviceName_);
}

ComStatelessServiceInstance::~ComStatelessServiceInstance()
{
    WriteNoise(TraceType, id_, "dtor: {0}", serviceName_);
}

HRESULT ComStatelessServiceInstance::BeginOpen( 
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(partition);

    WriteNoise(TraceType, id_, "Opening {0}", serviceName_);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComStatelessServiceInstance::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    WriteNoise(TraceType, id_, "Opened {0}", serviceName_);

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    
    hr = this->SetupEndpointAddresses();
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    
    if (!endpoints_.empty())
    {
        return ComStringResult::ReturnStringResult(endpoints_, serviceAddress);
    }
    return ComStringResult::ReturnStringResult(id_, serviceAddress);
}
        
HRESULT ComStatelessServiceInstance::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, id_, "Closing {0}", serviceName_);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComStatelessServiceInstance::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, id_, "Closed {0}", serviceName_);

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    return ComUtility::OnPublicApiReturn(hr);
}
        
void ComStatelessServiceInstance::Abort(void)
{
    WriteNoise(TraceType, id_, "Aborted {0}", serviceName_);
}

HRESULT ComStatelessServiceInstance::SetupEndpointAddresses()
{
    ComPointer<IFabricRuntime> fabricRuntimeCPtr;
    ComPointer<IFabricCodePackageActivationContext> activationContextCPtr;

    HRESULT hrContext = ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContextCPtr.VoidInitializationAddress());
    if (FAILED(hrContext))
    {
        WriteError(TraceType, id_, "Failed to get CodePackageActivationContext. Error:{0}", hrContext);
        return hrContext;
    }

    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = activationContextCPtr->get_ServiceEndpointResources();

    if (endpoints == NULL ||
        endpoints->Count == 0 ||
        endpoints->Items == NULL)
    {
        return S_OK;
    }
	
    EndpointsDescription endpointsDescription;
    for (ULONG i = 0; i < endpoints->Count; ++i)
    {
        const auto & endpoint = endpoints->Items[i];
        
        wstring uriScheme;
        wstring pathSuffix;

		if (endpoint.Reserved == nullptr)
		{
			WriteError(TraceType, id_, "Failed to get EndpointDescriptionEx1");
			return E_INVALIDARG;
		}

		auto endpointEx1Ptr = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1*)endpoint.Reserved;
		uriScheme = endpointEx1Ptr->UriScheme;
		pathSuffix = endpointEx1Ptr->PathSuffix;

		if (endpointEx1Ptr->Reserved == nullptr)
		{
			WriteError(TraceType, id_, "Failed to get EndpointDescriptionEx2");
			return E_INVALIDARG;
		}

		auto endpointEx2Ptr = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX2*)endpointEx1Ptr->Reserved;		
		wstring ipAddressOrFqdn = endpointEx2Ptr->IpAddressOrFqdn;
		
		if (ipAddressOrFqdn.empty())
		{
			WriteError(TraceType, id_, "IpAddressOrFqdn is empty for endpoint resource={0}", endpoint.Name);
			ASSERT_IF(true, "IpAddressOrFqdn is empty for endpoint resource={0}", endpoint.Name);
		}
		
		wstring listenerAddress;
        if (!uriScheme.empty())
        {
            listenerAddress = wformatString("{0}://{1}:{2}/{3}", uriScheme, ipAddressOrFqdn, endpoint.Port, pathSuffix);
        }
        else
        {
            listenerAddress = wformatString("{0}:{1}", ipAddressOrFqdn, endpoint.Port);
        }

		WriteInfo(
			TraceType, 
			id_, 
			"SetupEndpointAddresses(): EndpointName={0}, ListenerAddress={1}.", 
			endpoint.Name, 
			listenerAddress);

        endpointsDescription.Endpoints.insert(make_pair(endpoint.Name, listenerAddress));
    }

    auto error = JsonHelper::Serialize<EndpointsDescription>(endpointsDescription, endpoints_);
    return error.ToHResult();
}
