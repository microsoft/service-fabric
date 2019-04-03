// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricData.h"
#include "EndpointsDescription.h"
#include "DnsPropertyValue.h"

/*static*/
void FabricData::Create(
    __out FabricData::SPtr& spData,
    __in KAllocator& allocator,
    __in IDnsTracer& tracer
)
{
    spData = _new(TAG, allocator) FabricData(tracer);
    KInvariant(spData != nullptr);
}

FabricData::FabricData(
    __in IDnsTracer& tracer
) : _tracer(tracer)
{
}

FabricData::~FabricData()
{
}

bool FabricData::DeserializeServiceEndpoints(
    __in IFabricResolvedServicePartitionResult& result,
    __out KArray<KString::SPtr>& arrEndpoints
) const
{
    const FABRIC_RESOLVED_SERVICE_PARTITION* pPartition = result.get_Partition();

    if (pPartition->EndpointCount == 0)
    {
        _tracer.Trace(DnsTraceLevel_Warning, "DeserializeServiceEndpoints endpoint count is zero");
    }

    for (ULONG i = 0; i < pPartition->EndpointCount; i++)
    {
        FABRIC_RESOLVED_SERVICE_ENDPOINT& endpoint = pPartition->Endpoints[i];

        // For stateful services, only return endpoint information of the primary replica.
        if ((endpoint.Role != FABRIC_SERVICE_ROLE_STATEFUL_SECONDARY) && (endpoint.Address != nullptr))
        {
            EndpointsDescription endpointDesc;
            Common::ErrorCode error = Common::JsonHelper::Deserialize(endpointDesc, endpoint.Address);
            if (error.IsSuccess())
            {
                for (std::map<std::wstring, std::wstring>::const_iterator it = endpointDesc.Endpoints.begin();
                    it != endpointDesc.Endpoints.end();
                    ++it)
                {
                    // We care only about value, not the key.
                    std::wstring value = it->second;
                    if (!value.empty())
                    {
                        KString::SPtr spEndpoint = KString::Create(value.c_str(), GetThisAllocator());
                        if (spEndpoint != nullptr)
                        {
                            if (STATUS_SUCCESS != arrEndpoints.Append(spEndpoint))
                            {
                                KInvariant(false);
                            }
                        }
                    }
                }
            }
            // If JSON deserialize fails, we assume this is a plain URI.
            else if (endpoint.Address != nullptr)
            {
                KStringView strAddress(endpoint.Address);
                if (!strAddress.IsEmpty())
                {
                    KString::SPtr spEndpoint = KString::Create(endpoint.Address, GetThisAllocator());
                    if (spEndpoint != nullptr)
                    {
                        if (STATUS_SUCCESS != arrEndpoints.Append(spEndpoint))
                        {
                            KInvariant(false);
                        }
                    }
                }
            }
        }
        else
        {
            _tracer.Trace(DnsTraceLevel_Warning, "DeserializeServiceEndpoints endpoint address null");
        }
    }

    return (arrEndpoints.Count() > 0);
}

bool FabricData::DeserializePropertyValue(
    __in IFabricPropertyValueResult& result,
    __out KString::SPtr& spValue
) const
{
    spValue = nullptr;

    ULONG size = 0;
    BYTE* pData = nullptr;
    HRESULT hr = result.GetValueAsBinary(/*out*/&size, /*out*/(const BYTE**)&pData);

    if (S_OK != hr)
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "DeserializePropertyValue get binary value failed, error {0}",
            hr);

        return false;
    }

    DnsPropertyValue value;
    Common::ErrorCode error = Common::FabricSerializer::Deserialize(value, size, pData);
    if (!error.IsSuccess())
    {
        std::wstring strError = error.ErrorCodeValueToString();
        _tracer.Trace(DnsTraceLevel_Warning,
            "DeserializePropertyValue failed to deserialize DnsPropertyValue, error {0}",
            WSTR(strError.c_str()));

        return false;
    }

    if (value.ServiceName.empty())
    {
        _tracer.Trace(DnsTraceLevel_Warning,
            "DeserializePropertyValue failed, service name is empty"
        );

        return false;
    }

    spValue = KString::Create(value.ServiceName.c_str(), GetThisAllocator());

    return true;
}

/*static*/
bool FabricData::SerializeServiceEndpoints(
    __in LPCWSTR wszEndpoint,
    __out KString& result
)
{
    EndpointsDescription endpoint;
    endpoint.Endpoints.insert(std::pair<std::wstring, std::wstring>(L"udp", wszEndpoint));

    std::wstring strJson;
    Common::ErrorCode error = Common::JsonHelper::Serialize(endpoint, strJson);
    if (!error.IsSuccess())
    {
        return false;
    }

    if (STATUS_SUCCESS != result.Concat(strJson.c_str()))
    {
        return false;
    }

    return true;
}
