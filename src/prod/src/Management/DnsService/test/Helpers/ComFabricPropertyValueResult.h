// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS { namespace Test
{
    using ::_delete;

    class ComFabricPropertyValueResult :
        public KShared<ComFabricPropertyValueResult>,
        public IFabricPropertyValueResult
    {
        K_FORCE_SHARED(ComFabricPropertyValueResult);

        K_BEGIN_COM_INTERFACE_LIST(ComFabricPropertyValueResult)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricPropertyValueResult)
            K_COM_INTERFACE_ITEM(IID_IFabricPropertyValueResult, IFabricPropertyValueResult)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out ComFabricPropertyValueResult::SPtr& spResult,
            __in KAllocator& allocator,
            __in KBuffer& buffer
            );

    private:
        ComFabricPropertyValueResult(
            __in KBuffer& buffer
        );

    public:
        virtual const FABRIC_NAMED_PROPERTY* get_Property(void) override;

        virtual HRESULT GetValueAsBinary(
            /* [out] */ ULONG *byteCount,
            /* [retval][out] */ const BYTE**) override;

        virtual HRESULT GetValueAsInt64(
            /* [retval][out] */ LONGLONG*) override;

        virtual HRESULT GetValueAsDouble(
            /* [retval][out] */ double*) override;

        virtual HRESULT GetValueAsWString(
            /* [retval][out] */ LPCWSTR*) override;

        virtual HRESULT GetValueAsGuid(
            /* [retval][out] */ GUID*) override;

    private:
        KBuffer::SPtr _spData;
    };
}}
