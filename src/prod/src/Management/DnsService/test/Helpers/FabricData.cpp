// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricData.h"
#include "EndpointsDescription.h"
#include "DnsPropertyValue.h"
#include "ComFabricPropertyValueResult.h"

/*static*/
void FabricData::Create(
    __out FabricData::SPtr& spData,
    __in KAllocator& allocator
)
{
    spData = _new(TAG, allocator) FabricData();
    KInvariant(spData != nullptr);
}

FabricData::FabricData()
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
        return false;
    }

    DnsPropertyValue value;
    Common::ErrorCode error = Common::FabricSerializer::Deserialize(value, size, pData);
    if (!error.IsSuccess())
    {
        return false;
    }

    if (value.ServiceName.empty())
    {
        return false;
    }

    spValue = KString::Create(value.ServiceName.c_str(), GetThisAllocator());

    return true;
}

bool FabricData::SerializeServiceEndpoints(
    __out ComPointer<IFabricResolvedServicePartitionResult>& spResult,
    __in KArray<KString::SPtr>& arrEndpoints
)
{
    EndpointsDescription description;
    for (ULONG i = 0; i < arrEndpoints.Count(); i++)
    {
        std::wstring strKey = std::to_wstring(i);
        std::wstring strValue(*arrEndpoints[i]);
        description.Endpoints[strKey] = strValue;
    }

    std::wstring strjson;
    Common::ErrorCode error = Common::JsonHelper::Serialize(description, strjson);
    if (error.IsSuccess())
    {
        KString::SPtr spJson = KString::Create(strjson.c_str(), GetThisAllocator());
        ComServicePartitionResult::SPtr spResultImpl;
        ComServicePartitionResult::Create(/*out*/spResultImpl, GetThisAllocator(), *spJson);
        spResult = spResultImpl.RawPtr();

        return true;
    }

    return false;
}

bool FabricData::SerializePropertyValue(
    __out ComPointer<IFabricPropertyValueResult>& spResult,
    __in LPCWSTR wszValue
)
{
    spResult = nullptr;

    std::wstring serviceName(wszValue);
    DnsPropertyValue value;
    value.ServiceName = serviceName;

    KBuffer::SPtr spData;
    Common::FabricSerializer::Serialize(&value, /*out*/spData);

    ComFabricPropertyValueResult::SPtr sp;
    ComFabricPropertyValueResult::Create(/*out*/sp, GetThisAllocator(), *spData);

    spResult = sp.RawPtr();

    return true;
}
