// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#ifdef WIN32
#define wcsdup _wcsdup
#endif

namespace Data
{
    namespace Interop
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;

        BOOST_AUTO_TEST_CASE(StateProviderInfo_FromPublicApi_SUCCESS)
        {
            KtlSystem* underlyingSystem_;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_); 
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status); 
            underlyingSystem_->SetStrictAllocationChecks(TRUE); 
            underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE); 
            KFinally([&]() { KtlSystem::Shutdown(); });
            KAllocator& allocator = underlyingSystem_->PagedAllocator();

            StateProvider_Info stateProvider_info;
            stateProvider_info.Size = StateProvider_Info_V1_Size;
            stateProvider_info.Kind = StateProvider_Kind_Store;
            stateProvider_info.LangMetadata = wcsdup(L"System.String");

            StateProviderInfo::SPtr stateProviderInfoSPtr;
            status = StateProviderInfo::FromPublicApi(
                allocator,
                L"CSHARP",
                stateProvider_info,
                stateProviderInfoSPtr);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateProvider_Info stateProvider_info_result;
            status = stateProviderInfoSPtr->ToPublicApi(stateProvider_info_result);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(stateProvider_info_result.Size == StateProvider_Info_V1_Size);
            VERIFY_IS_TRUE(stateProvider_info_result.Kind == StateProviderKind::Store);
            int result = wcscmp((WCHAR*)stateProvider_info_result.LangMetadata, L"System.String");
            VERIFY_IS_TRUE(result == 0);
        }    

        BOOST_AUTO_TEST_CASE(StateProviderInfo_Encode_SUCCESS)
        {
            KtlSystem* underlyingSystem_;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_);
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status);
            underlyingSystem_->SetStrictAllocationChecks(TRUE);
            underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE);
            KFinally([&]() { KtlSystem::Shutdown(); });
            KAllocator& allocator = underlyingSystem_->PagedAllocator();

            StateProvider_Info stateProvider_info;
            stateProvider_info.Size = StateProvider_Info_V1_Size;
            stateProvider_info.Kind = StateProvider_Kind_Store;
            stateProvider_info.LangMetadata = wcsdup(L"System.String");

            StateProviderInfo::SPtr stateProviderInfoSPtr;
            status = StateProviderInfo::FromPublicApi(
                allocator,
                L"CSHARP",
                stateProvider_info,
                stateProviderInfoSPtr);

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KString::SPtr encodedString;
            status = stateProviderInfoSPtr->Encode(encodedString);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            WCHAR* ptr = static_cast<WCHAR*>(*encodedString);
            int result = wcscmp(ptr, L"0\1\1CSHARP\nSystem.String");
            VERIFY_IS_TRUE(result == 0);
        }

        BOOST_AUTO_TEST_CASE(StateProviderInfo_Decode_SUCCESS)
        {
            KtlSystem* underlyingSystem_;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem_);
            ASSERT_IFNOT(NT_SUCCESS(status), "Status not success: {0}", status);
            underlyingSystem_->SetStrictAllocationChecks(TRUE);
            underlyingSystem_->SetDefaultSystemThreadPoolUsage(FALSE);
            KFinally([&]() { KtlSystem::Shutdown(); });
            KAllocator& allocator = underlyingSystem_->PagedAllocator();

            KString::SPtr encodedString;
            KString::Create(encodedString, allocator, L"0\1\1CSHARP\nSystem.String");

            StateProviderInfo::SPtr stateProviderInfoSPtr;
            status = StateProviderInfo::Decode(
                allocator,
                *encodedString,
                stateProviderInfoSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            StateProvider_Info stateProvider_info_result;
            status = stateProviderInfoSPtr->ToPublicApi(stateProvider_info_result);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(stateProvider_info_result.Size == StateProvider_Info_V1_Size);
            VERIFY_IS_TRUE(stateProvider_info_result.Kind == StateProviderKind::Store);
            int result = wcscmp((WCHAR*)stateProvider_info_result.LangMetadata, L"System.String");
            VERIFY_IS_TRUE(result == 0);
        }
    }
}
