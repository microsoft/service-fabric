//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("EndpointsHelper");

class EndpointsHelper::EndpointsJsonWrapper : public IFabricJsonSerializable
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

WStringLiteral const EndpointsHelper::EndpointsJsonWrapper::EndpointsParameter(L"Endpoints");

ErrorCode EndpointsHelper::ConvertToJsonString(
    wstring const & serviceInfo,
    vector<EndpointDescription> const & endpointDescriptions,
    __out wstring & endpointsJsonString)
{
    if (endpointDescriptions.size() == 0)
    {
        return ErrorCodeValue::Success;
    }

    EndpointsJsonWrapper endpointsJsonWrapper;

    for (auto const & endpoint : endpointDescriptions)
    {
        if (endpoint.IpAddressOrFqdn.empty())
        {
            WriteError(
                TraceType, 
                "{0}. IpAddressOrFqdn is empty for endpoint resource={1}",
                serviceInfo,
                endpoint.Name);
            
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
            "ConvertToJsonString(): {0}. EndpointName={1}, ListenerAddress={2}.",
            serviceInfo,
            endpoint.Name,
            listenerAddress);

        endpointsJsonWrapper.Insert(endpoint.Name, listenerAddress);
    }

    auto error = JsonHelper::Serialize(endpointsJsonWrapper, endpointsJsonString);

    WriteInfo(
        TraceType,
        "ConvertToJsonString(): {0}. EndpointsJsonString=[{1}].",
        serviceInfo,
        endpointsJsonString);

    return error;
}

HRESULT ComCompletedOperationHelper::BeginCompletedComOperation(
    IFabricAsyncOperationCallback *callback,
    IFabricAsyncOperationContext **context)
{
    auto operation = make_com<ComCompletedAsyncOperationContext>();
    auto hr = operation->Initialize(S_OK, ComponentRootSPtr(), callback);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComCompletedOperationHelper::EndCompletedComOperation(
    IFabricAsyncOperationContext *context)
{
    return ComCompletedAsyncOperationContext::End(context);
}

wstring HelperUtilities::GetSecondaryCodePackageEventName(ServicePackageInstanceIdentifier const & servicePackageId, bool forPrimary, bool completionEventName)
{
    auto uniqueId = servicePackageId.PublicActivationId;
    if (uniqueId.empty())
    {
        uniqueId = wformatString("{0}_{1}",servicePackageId.ApplicationId.ToString(), servicePackageId.ServicePackageName);
    }
    wstring eventName;
    if (forPrimary)
    {
        eventName = wformatString("{0}_{1}_Activate", L"SF", uniqueId);
    }
    else
    {
        eventName = wformatString("{0}_{1}_Deactivate", L"SF", uniqueId);
    }
    if (completionEventName)
    {
        return wformatString("{0}_Completed", eventName);
    }
    return eventName;
}