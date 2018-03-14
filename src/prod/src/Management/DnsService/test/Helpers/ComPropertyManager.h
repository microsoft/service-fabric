// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ComFabricPropertyValueResult.h"

namespace DNS { namespace Test
{
    using ::_delete;

    class ComPropertyManager :
        public KShared<ComPropertyManager>,
        public IFabricPropertyManagementClient
    {
        K_FORCE_SHARED(ComPropertyManager);

        K_BEGIN_COM_INTERFACE_LIST(ComPropertyManager)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricPropertyManagementClient)
            K_COM_INTERFACE_ITEM(IID_IFabricPropertyManagementClient, IFabricPropertyManagementClient)
            K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComPropertyManager::SPtr& spManager,
            __in KAllocator& allocator
        );

    public:
        // IFabricPropertyManagementClient Impl.
        virtual HRESULT BeginCreateName(
            __in FABRIC_URI name,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndCreateName(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginDeleteName(
            __in FABRIC_URI name,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndDeleteName(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginNameExists(
            __in FABRIC_URI name,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndNameExists(
            __in IFabricAsyncOperationContext * context,
            __out BOOLEAN * value) override;

        virtual HRESULT BeginEnumerateSubNames(
            __in FABRIC_URI name,
            __in IFabricNameEnumerationResult * previousResult,
            __in BOOLEAN recursive,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndEnumerateSubNames(
            __in IFabricAsyncOperationContext * context,
            __out IFabricNameEnumerationResult ** result) override;

        virtual HRESULT BeginPutPropertyBinary(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in ULONG dataLength,
            __in_bcount(dataLength) const BYTE * data,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndPutPropertyBinary(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginPutPropertyInt64(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in LONGLONG data,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndPutPropertyInt64(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginPutPropertyDouble(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in double data,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndPutPropertyDouble(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginPutPropertyWString(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in LPCWSTR data,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndPutPropertyWString(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginPutPropertyGuid(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in const GUID * data,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndPutPropertyGuid(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginDeleteProperty(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndDeleteProperty(
            __in IFabricAsyncOperationContext * context) override;

        virtual HRESULT BeginGetPropertyMetadata(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndGetPropertyMetadata(
            __in IFabricAsyncOperationContext * context,
            __out IFabricPropertyMetadataResult ** result) override;

        virtual HRESULT BeginGetProperty(
            __in FABRIC_URI name,
            __in LPCWSTR propertyName,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndGetProperty(
            __in IFabricAsyncOperationContext * context,
            __out IFabricPropertyValueResult ** result) override;

        virtual HRESULT BeginSubmitPropertyBatch(
            __in FABRIC_URI name,
            __in ULONG operationCount,
            __in_ecount(operationCount) const FABRIC_PROPERTY_BATCH_OPERATION * operations,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndSubmitPropertyBatch(
            __in IFabricAsyncOperationContext * context,
            __out ULONG * failedOperationIndexInRequest,
            __out IFabricPropertyBatchResult ** result) override;

        virtual HRESULT BeginEnumerateProperties(
            __in FABRIC_URI name,
            __in BOOLEAN includeValues,
            __in IFabricPropertyEnumerationResult * previousResult,
            __in DWORD timeoutMilliseconds,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        virtual HRESULT EndEnumerateProperties(
            __in IFabricAsyncOperationContext * context,
            __out IFabricPropertyEnumerationResult ** result) override;

    public:
        void SetResult(__in ComPointer<IFabricPropertyValueResult>& spResult) { _spResult = spResult; }

    private:
        ComPointer<IFabricPropertyValueResult> _spResult;
    };
}}
