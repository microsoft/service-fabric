// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Interop;

NTSTATUS StateProviderInfo::FromPublicApi(
    __in KAllocator& allocator,
    __in KStringView const & lang,
    __in StateProvider_Info& stateProvider_Info,
    __out StateProviderInfo::SPtr& stateProviderInfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    if (stateProvider_Info.Size != StateProvider_Info_V1_Size)
        return STATUS_INVALID_PARAMETER;

    StateProviderInfo* created = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) StateProviderInfo();
    if (!created)
        return E_OUTOFMEMORY;

    stateProviderInfo = created;

    created->version_ = L'\1';

    created->kind_ = static_cast<StateProviderKind>(stateProvider_Info.Kind);

    if (!lang.IsEmpty())
    {
        status = KString::Create(created->lang_, allocator, lang);
        if (!NT_SUCCESS(status))
            return status;
    }

    if (stateProvider_Info.LangMetadata != nullptr)
    {
        status = KString::Create(created->langMetadata_, allocator, stateProvider_Info.LangMetadata);
        if (!NT_SUCCESS(status))
            return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateProviderInfo::FromStateProvider(
    __in TxnReplicator::IStateProvider2* stateProvider,
    __in KStringView const & lang,
    __out StateProviderInfo& stateProviderInfo)
{
    IStateProviderInfo* stateProviderInfoPtr = dynamic_cast<IStateProviderInfo*>(stateProvider);
    if (stateProviderInfoPtr == nullptr)
        return STATUS_INVALID_PARAMETER;

    stateProviderInfo.version_ = L'\1';

    stateProviderInfo.kind_ = stateProviderInfoPtr->GetKind();
    // Not Setting Lang when caller already asked data for a particular lang
    stateProviderInfo.langMetadata_ = stateProviderInfoPtr->GetLangTypeInfo(lang);

    return STATUS_SUCCESS;
}

NTSTATUS StateProviderInfo::ToPublicApi(
    __inout StateProvider_Info& stateProvider_Info)
{
    stateProvider_Info.Size = StateProvider_Info_V1_Size;
    stateProvider_Info.Kind = static_cast<StateProvider_Kind>(kind_);

    if (langMetadata_ != nullptr)
        stateProvider_Info.LangMetadata = static_cast<LPCWSTR>(*langMetadata_);
    else
        stateProvider_Info.LangMetadata = nullptr;

    return STATUS_SUCCESS;
}

NTSTATUS StateProviderInfo::Encode(
    __out KString::SPtr& encodedString)
{
    if (Kind == StateProviderKind::ReliableDictionary_Compat)
    { 
        encodedString = LangMetadata;
    }
    else
    {
        KString::SPtr result;
        NTSTATUS status = KString::Create(result, GetThisAllocator());
        if (!NT_SUCCESS(status))
            return status;

        if (!result->AppendChar(STATE_PROVIDER_ENCODED_INFO_MARKER))
            return STATUS_INSUFFICIENT_RESOURCES;

        if (!result->AppendChar(version_))
            return STATUS_INSUFFICIENT_RESOURCES;

        if (!result->AppendChar((WCHAR)kind_))
            return STATUS_INSUFFICIENT_RESOURCES;

        if (lang_ != nullptr && !result->Concat(const_cast<KString &>(*lang_)))
            return STATUS_INSUFFICIENT_RESOURCES;

        if (!result->AppendChar(L'\n'))
            return STATUS_INSUFFICIENT_RESOURCES;

        if (langMetadata_ != nullptr && !result->Concat(const_cast<KString &>(*langMetadata_)))
            return STATUS_INSUFFICIENT_RESOURCES;

        encodedString = result;
    }

    return STATUS_SUCCESS;
}

NTSTATUS StateProviderInfo::Decode(
    __in KAllocator& allocator,
    __in KString const & encodedString,
    __out StateProviderInfo::SPtr& stateProviderInfo)
{
    WCHAR* ptr;
    ULONG pos;
    ULONG termCharPos;
    if (encodedString.IsNull() || encodedString.IsEmpty())
        return STATUS_INVALID_PARAMETER;

    StateProviderInfo* created = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) StateProviderInfo();
    if (!created)
        return E_OUTOFMEMORY;

    stateProviderInfo = created;

    ptr = static_cast<WCHAR*>(encodedString);
    pos = 0;
    if (*ptr == STATE_PROVIDER_ENCODED_INFO_MARKER)
    {
        pos++;

        // new Encoding Format
        // 1 char Marker('0') to idenfity new encoding format. '0' has been used because existing typename strings are C# identifiers and C# identifiers do not start with number 
        // 1 wchar Version
        // 1 wchar kind
        // language name terminated by '\n'
        // language metadata 

        if (encodedString.Length() < 3) // 1 - marker, 1 - version, 1 - kind
            return STATUS_INVALID_PARAMETER;


        if (*(ptr + pos) != L'\1')
            return STATUS_INVALID_PARAMETER;

        created->version_ = L'\1'; pos++;

        created->kind_ = (StateProviderKind)(*(ptr + pos)); pos++;

        if (encodedString.Find('\n', termCharPos))
        {
            NTSTATUS status = KString::Create(created->lang_, allocator, termCharPos - pos + 1); // +1 for null term
            if (!NT_SUCCESS(status))
                return status;

            while (pos < termCharPos)
            {
                created->lang_->AppendChar(*(ptr + pos));
                pos++;
            }

            if (!created->lang_->SetNullTerminator())
                return STATUS_INSUFFICIENT_RESOURCES;

            pos++; // skip terminating char '\n'

            if (pos < encodedString.Length())
            {
                status = KString::Create(created->langMetadata_, allocator, ptr + pos);
                if (!NT_SUCCESS(status))
                    return status;
            }
        }
        else
            return STATUS_INVALID_PARAMETER;
    }
    else // Compat
    {
        created->version_ = L'\1';

        if (wcsncmp(ptr, L"Microsoft.ServiceFabric.Data.Collections.DistributedDictionary", 62) == 0)
            created->kind_ = StateProviderKind::ReliableDictionary_Compat;
        else if (wcsncmp(ptr, L"Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue.ReliableConcurrentQueue", 88) == 0)
            created->kind_ = StateProviderKind::ConcurrentQueue;
        else if (wcsncmp(ptr, L"Microsoft.ServiceFabric.Data.Collections.DistributedQueue", 57) == 0)
            return STATUS_NOT_IMPLEMENTED;
        else
            return STATUS_INVALID_PARAMETER;

        NTSTATUS status = KString::Create(created->lang_, allocator, L"CSHARP");
        if (!NT_SUCCESS(status))
            return status;

        status = KString::Create(created->langMetadata_, allocator, ptr);
        if (!NT_SUCCESS(status))
            return status;
    }

    return STATUS_SUCCESS;
}

WCHAR StateProviderInfo::get_Version() const { return version_; }

Data::StateProviderKind StateProviderInfo::get_Kind() const { return kind_; }

KString::SPtr StateProviderInfo::get_Lang() const { return lang_; }

KString::SPtr StateProviderInfo::get_LangMetadata() const { return langMetadata_; }
