// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("GuestServiceInstance");

class ComGuestServiceInstance::EndpointsJsonWrapper : public IFabricJsonSerializable
{
public:
    EndpointsJsonWrapper() : endpoints_()
    {
    }

    void Insert(wstring const & endpointName, wstring const & listenerAddress)
    {
        endpoints_.insert(make_pair(endpointName, listenerAddress));
    }

    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY_SIMPLE_MAP(EndpointsJsonWrapper::EndpointsParameter, endpoints_)
    END_JSON_SERIALIZABLE_PROPERTIES()
    
private:
    map<wstring, wstring> endpoints_;
    static WStringLiteral const EndpointsParameter;
};

WStringLiteral const ComGuestServiceInstance::EndpointsJsonWrapper::EndpointsParameter(L"Endpoints");

ComGuestServiceInstance::ComGuestServiceInstance(
    wstring const & serviceName,
    wstring const & serviceTypeName,
    Guid const & partitionId,
    ::FABRIC_INSTANCE_ID instanceId,
    vector<EndpointDescription> const & endpointdescriptions)
    : IFabricStatelessServiceInstance()
    , ComUnknownBase()
    , serviceName_(serviceName)
    , serviceTypeName_(serviceTypeName)
    , partitionId_(partitionId)
    , instanceId_(instanceId)
    , endpointsAsJsonStr_()
    , endpointDescriptions_(endpointdescriptions)
    , traceId_(wformatString("{0}:{1}:{2}",serviceTypeName, partitionId, instanceId))
{
    WriteNoise(TraceType, traceId_, "ctor: {0}", serviceName_);
}

ComGuestServiceInstance::~ComGuestServiceInstance()
{
    WriteNoise(TraceType, traceId_, "dtor: {0}", serviceName_);
}

HRESULT ComGuestServiceInstance::BeginOpen( 
    /* [in] */ IFabricStatelessServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(partition);

    WriteNoise(TraceType, traceId_, "Opening {0}", serviceName_);

    auto operation = make_com<ComCompletedAsyncOperationContext>();
    auto hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComGuestServiceInstance::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    WriteNoise(TraceType, traceId_, "Opened {0}", serviceName_);

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    
    hr = this->SetupEndpointAddresses();
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    
    if (!endpointsAsJsonStr_.empty())
    {
        return ComStringResult::ReturnStringResult(endpointsAsJsonStr_, serviceAddress);
    }
    return ComStringResult::ReturnStringResult(traceId_, serviceAddress);
}
        
HRESULT ComGuestServiceInstance::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    WriteNoise(TraceType, traceId_, "Closing {0}", serviceName_);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);

    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}
        
HRESULT ComGuestServiceInstance::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    WriteNoise(TraceType, traceId_, "Closed {0}", serviceName_);

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    return ComUtility::OnPublicApiReturn(hr);
}
        
void ComGuestServiceInstance::Abort(void)
{
    WriteNoise(TraceType, traceId_, "Aborted {0}", serviceName_);
}

HRESULT ComGuestServiceInstance::SetupEndpointAddresses()
{
    if (endpointDescriptions_.size() == 0)
    {
        return S_OK;
    }
	
    EndpointsJsonWrapper endpointsJsonWrapper;
    
    for (auto const & endpoint : endpointDescriptions_)
    {
        if (endpoint.IpAddressOrFqdn.empty())
        {
            WriteError(TraceType, traceId_, "IpAddressOrFqdn is empty for endpoint resource={0}", endpoint.Name);
            ASSERT_IF(true, "IpAddressOrFqdn is empty for endpoint resource={0}", endpoint.Name);
        }

        wstring listenerAddress;
        
        if (endpoint.UriScheme.empty())
        {
            listenerAddress = wformatString("{0}:{1}", endpoint.IpAddressOrFqdn, endpoint.Port);
        }
        else
        {
            listenerAddress = wformatString(
                "{0}://{1}:{2}/{3}", endpoint.UriScheme, endpoint.IpAddressOrFqdn, endpoint.Port, endpoint.PathSuffix);
        }

        WriteInfo(
            TraceType,
            traceId_,
            "SetupEndpointAddresses(): EndpointName={0}, ListenerAddress={1}.",
            endpoint.Name,
            listenerAddress);

        endpointsJsonWrapper.Insert(endpoint.Name, listenerAddress);
    }

    auto error = JsonHelper::Serialize(endpointsJsonWrapper, endpointsAsJsonStr_);
    
    return error.ToHResult();
}
