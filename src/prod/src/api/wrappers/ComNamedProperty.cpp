// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace Api;

ComNamedProperty::ComNamedProperty(NamePropertyResult && property)
    : heap_()
    , bytes_(property.TakeBytes())
    , property_()
{
    property_ = heap_.AddItem<FABRIC_NAMED_PROPERTY>().GetRawPointer();

    Common::ReferencePointer<FABRIC_NAMED_PROPERTY_METADATA> metadata = heap_.AddItem<FABRIC_NAMED_PROPERTY_METADATA>();
    property_->Metadata = metadata.GetRawPointer();

    property.Metadata.ToPublicApi(heap_, *metadata);

    property_->Value = bytes_.data();
}

ComNamedProperty::ComNamedProperty(NamePropertyMetadataResult && propertyMetadata)
    : heap_()
    , bytes_()
    , property_()
{
    property_ = heap_.AddItem<FABRIC_NAMED_PROPERTY>().GetRawPointer();

    Common::ReferencePointer<FABRIC_NAMED_PROPERTY_METADATA> metadata = heap_.AddItem<FABRIC_NAMED_PROPERTY_METADATA>();
    property_->Metadata = metadata.GetRawPointer();

    propertyMetadata.ToPublicApi(heap_, *metadata);

    property_->Value = NULL;
}

const FABRIC_NAMED_PROPERTY * STDMETHODCALLTYPE ComNamedProperty::get_Property()
{
    return property_;
}

HRESULT STDMETHODCALLTYPE ComNamedProperty::GetValueAsBinary(
    __out ULONG * byteCount,
    __out const BYTE ** bufferedValue)
{
    if ((byteCount == NULL) || (bufferedValue == NULL))
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (property_->Metadata->TypeId != FABRIC_PROPERTY_TYPE_BINARY)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    *byteCount = property_->Metadata->ValueSize;
    *bufferedValue = property_->Value;
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComNamedProperty::GetValueAsInt64(
    __out LONGLONG * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (property_->Metadata->TypeId != FABRIC_PROPERTY_TYPE_INT64)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    if (bytes_.empty())
    {
        return ComUtility::OnPublicApiReturn(FABRIC_E_VALUE_EMPTY);
    }

    if (bytes_.size() != sizeof(*value))
    {
        return ComUtility::OnPublicApiReturn(E_FAIL);
    }

    memcpy_s(value, sizeof(*value), property_->Value, sizeof(*value));
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComNamedProperty::GetValueAsDouble(
    __out double * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (property_->Metadata->TypeId != FABRIC_PROPERTY_TYPE_DOUBLE)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    if (bytes_.empty())
    {
        return ComUtility::OnPublicApiReturn(FABRIC_E_VALUE_EMPTY);
    }

    if (bytes_.size() != sizeof(*value))
    {
        return ComUtility::OnPublicApiReturn(E_FAIL);
    }

    memcpy_s(value, sizeof(*value), property_->Value, sizeof(*value));
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComNamedProperty::GetValueAsWString(
    __out LPCWSTR * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (property_->Metadata->TypeId != FABRIC_PROPERTY_TYPE_WSTRING)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    if (bytes_.empty())
    {
        return ComUtility::OnPublicApiReturn(FABRIC_E_VALUE_EMPTY);
    }

    if (bytes_.size() < sizeof(wchar_t))
    {
        return ComUtility::OnPublicApiReturn(E_FAIL);
    }

    // Check that the last character in the buffer is a terminating null.
    BYTE * lastCharBytePointer = property_->Value + (bytes_.size() - sizeof(wchar_t));
    wchar_t * lastCharPointer = reinterpret_cast<wchar_t *>(lastCharBytePointer);
    if (*lastCharPointer != 0)
    {
        return ComUtility::OnPublicApiReturn(E_FAIL);
    }

    *value = reinterpret_cast<LPCWSTR>(property_->Value);
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComNamedProperty::GetValueAsGuid(
    __out GUID * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (property_->Metadata->TypeId != FABRIC_PROPERTY_TYPE_GUID)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    if (bytes_.empty())
    {
        return ComUtility::OnPublicApiReturn(FABRIC_E_VALUE_EMPTY);
    }

    if (bytes_.size() != sizeof(*value))
    {
        return ComUtility::OnPublicApiReturn(E_FAIL);
    }

    memcpy_s(value, sizeof(*value), property_->Value, sizeof(*value));
    return ComUtility::OnPublicApiReturn(S_OK);
}

