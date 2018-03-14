// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    // 498f4c96-c685-49a9-b80bb5ac482be165
    static const GUID IID_IFabricTestClient =
    {0x498f4c96,0xc685,0x49a9,{0xb8,0x0b,0xb5,0xac,0x48,0x2b,0xe1,0x65}};

    class IFabricTestClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNthNamingPartitionId(
            /* [in] */ ULONG n,
            /* [retval][out] */ FABRIC_PARTITION_ID * partitionId) = 0;

        virtual HRESULT STDMETHODCALLTYPE BeginResolvePartition(
            /* [in] */ FABRIC_PARTITION_ID * partitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;

        virtual HRESULT STDMETHODCALLTYPE EndResolvePartition(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult ** result) = 0;

        virtual HRESULT STDMETHODCALLTYPE BeginResolveNameOwner(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;

        virtual HRESULT STDMETHODCALLTYPE EndResolveNameOwner(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult ** result) = 0;

        virtual HRESULT STDMETHODCALLTYPE NodeIdFromNameOwnerAddress(
            /* [in] */ LPCWSTR address,
            _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId) = 0;

        virtual HRESULT STDMETHODCALLTYPE NodeIdFromFMAddress(
            /* [in] */ LPCWSTR address,
            _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId) = 0;

        virtual HRESULT STDMETHODCALLTYPE GetCurrentGatewayAddress(
            /* [retval][out] */ IFabricStringResult ** gatewayAddress) = 0;
    };
}
