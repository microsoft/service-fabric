// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComNamedProperty
        : public IFabricPropertyValueResult
        , private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComNamedProperty);

        COM_INTERFACE_LIST1(ComNamedProperty, IID_IFabricPropertyValueResult, IFabricPropertyValueResult);

    public:
        ComNamedProperty(Naming::NamePropertyResult && property);

        ComNamedProperty(Naming::NamePropertyMetadataResult && propertyMetadata);

        const FABRIC_NAMED_PROPERTY * STDMETHODCALLTYPE get_Property();

        HRESULT STDMETHODCALLTYPE GetValueAsBinary(
            __out ULONG * byteCount,
            __out const BYTE ** bufferedValue);

        HRESULT STDMETHODCALLTYPE GetValueAsInt64(
            __out LONGLONG * value);

        HRESULT STDMETHODCALLTYPE GetValueAsDouble(
            __out double * value);

        HRESULT STDMETHODCALLTYPE GetValueAsWString(
            __out LPCWSTR * bufferedValue);

        HRESULT STDMETHODCALLTYPE GetValueAsGuid(
            __out GUID * value);

    private:
        Common::ScopedHeap heap_;
        std::vector<byte> bytes_;
        FABRIC_NAMED_PROPERTY * property_;
    };

    typedef Common::ComPointer<ComNamedProperty> ComNamedPropertyCPtr;
}
