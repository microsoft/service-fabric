// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"

#define STATE_PROVIDER_ENCODED_INFO_MARKER L'0'
namespace Data
{
    namespace Interop
    {
        class StateProviderInfo :
            public KObject<StateProviderInfo>,
            public KShared<StateProviderInfo>
        {

        public:
            typedef KSharedPtr<StateProviderInfo> SPtr;

            __declspec(property(get = get_Version)) WCHAR Version;
            WCHAR get_Version() const;

            __declspec(property(get = get_Kind)) StateProviderKind Kind;
            StateProviderKind get_Kind() const;

            __declspec(property(get = get_Lang)) KString::SPtr Lang;
            KString::SPtr get_Lang() const;

            __declspec(property(get = get_LangMetadata)) KString::SPtr LangMetadata;
            KString::SPtr get_LangMetadata() const;

            static NTSTATUS FromPublicApi(
                __in KAllocator& allocator,
                __in KStringView const & lang,
                __in StateProvider_Info& stateProvider_Info,
                __out StateProviderInfo::SPtr& stateProviderInfo);

            static NTSTATUS FromStateProvider(
                __in TxnReplicator::IStateProvider2* stateProvider,
                __in KStringView const & lang,
                __out StateProviderInfo& stateProviderInfo);

            NTSTATUS ToPublicApi(
                __out StateProvider_Info& stateProvider_Info);

            NTSTATUS Encode(
                __out KString::SPtr& encodedString);

            static NTSTATUS Decode(
                __in KAllocator& allocator,
                __in KString const & encodedString, 
                __out StateProviderInfo::SPtr& stateProviderInfo);

        private:

            WCHAR version_;
            StateProviderKind kind_;
            KString::SPtr lang_;
            KString::SPtr langMetadata_;
        };
    }
}
