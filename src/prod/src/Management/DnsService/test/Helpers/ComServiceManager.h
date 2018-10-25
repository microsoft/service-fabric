// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ComServicePartitionResult.h"
#include "ComFabricServiceDescriptionResult.h"

namespace DNS { namespace Test
{
    using ::_delete;

    class ComServiceManager :
        public KShared<ComServiceManager>,
        public IFabricServiceManagementClient,
        public IInternalFabricServiceManagementClient2
    {
        K_FORCE_SHARED(ComServiceManager);

        K_BEGIN_COM_INTERFACE_LIST(ComServiceManager)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricServiceManagementClient)
            K_COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient, IFabricServiceManagementClient)
            K_COM_INTERFACE_ITEM(IID_IInternalFabricServiceManagementClient2, IInternalFabricServiceManagementClient2)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComServiceManager::SPtr& spManager,
            __in KAllocator& allocator
            );

    public:
        /* IFabricServiceManagementClient */
        virtual HRESULT BeginCreateService(
            /* [in] */ const FABRIC_SERVICE_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndCreateService(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT BeginCreateServiceFromTemplate(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ ULONG InitializationDataSize,
            /* [size_is][in] */ BYTE *InitializationData,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndCreateServiceFromTemplate(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT BeginDeleteService(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndDeleteService(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT BeginGetServiceDescription(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndGetServiceDescription(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result);

        virtual HRESULT RegisterServicePartitionResolutionChangeHandler(
            /* [in] */ FABRIC_URI name,
            /* [in] */ FABRIC_PARTITION_KEY_TYPE keyType,
            /* [in] */ const void *partitionKey,
            /* [in] */ IFabricServicePartitionResolutionChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);

        virtual HRESULT UnregisterServicePartitionResolutionChangeHandler(
            /* [in] */ LONGLONG callbackHandle);

        virtual HRESULT BeginResolveServicePartition(
            /* [in] */ FABRIC_URI name,
            /* [in] */ FABRIC_PARTITION_KEY_TYPE partitionKeyType,
            /* [in] */ const void *partitionKey,
            /* [in] */ IFabricResolvedServicePartitionResult *previousResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndResolveServicePartition(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult **result);

        /* IInternalFabricServiceManagementClient2 */
        virtual HRESULT BeginMovePrimary(
            /* [in] */ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION *movePrimaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndMovePrimary(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT BeginMoveSecondary(
            /* [in] */ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION *moveSecondaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndMoveSecondary(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT BeginGetCachedServiceDescription(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT EndGetCachedServiceDescription(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result);

    public:
        void AddResult(
            __in LPCWSTR wsz,
            __in IFabricResolvedServicePartitionResult& result,
            __in IFabricServiceDescriptionResult& sdResult
        );

    private:
        struct ServiceResult
        {
            ComPointer<IFabricResolvedServicePartitionResult> ServicePartitionResult;
            ComPointer<IFabricServiceDescriptionResult> ServiceDescriptionResult;
        };

        KHashTable<KWString, ServiceResult> _htResults;
    };
}}
