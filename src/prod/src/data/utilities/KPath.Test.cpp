// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class KPathTests
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        KPathTests()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~KPathTests()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(KPathTestSuite, KPathTests);

    BOOST_AUTO_TEST_CASE(Combine_WithoutSeperatorAtTheEnd_Success)
    {
#if !defined(PLATFORM_UNIX)
        KStringView pathA(L"C:\\test");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:\\test\\mcoskun");
#else
        KStringView pathA(L"C:/test");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:/test/mcoskun");
#endif

        KString::SPtr result = KPath::Combine(pathA, pathB, GetAllocator());
        VERIFY_ARE_EQUAL(*result, expectedResult);
    }

    BOOST_AUTO_TEST_CASE(CombineInPlace_WithoutSeperatorAtTheEnd_Success)
    {
#if !defined(PLATFORM_UNIX)
        KStringView pathA(L"C:\\test");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:\\test\\mcoskun");
#else
        KStringView pathA(L"C:/test");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:/test/mcoskun");
#endif
        KString::SPtr result;
        KString::Create(result, GetAllocator(), pathA);

        KPath::CombineInPlace(*result, pathB);
        VERIFY_ARE_EQUAL(*result, expectedResult);
    }

    BOOST_AUTO_TEST_CASE(Combine_WithSeperatorAtTheEnd_Success)
    {
#if !defined(PLATFORM_UNIX)
        KStringView pathA(L"C:\\test\\");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:\\test\\mcoskun");
#else
        KStringView pathA(L"C:/test/");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:/test/mcoskun");
#endif

        KString::SPtr result = KPath::Combine(pathA, pathB, GetAllocator());
        VERIFY_ARE_EQUAL(*result, expectedResult);
    }

    BOOST_AUTO_TEST_CASE(CombineInPlace_WithSeperatorAtTheEnd_Success)
    {
#if !defined(PLATFORM_UNIX)
        KStringView pathA(L"C:\\test\\");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:\\test\\mcoskun");
#else
        KStringView pathA(L"C:/test/");
        KStringView pathB(L"mcoskun");
        KStringView expectedResult(L"C:/test/mcoskun");
#endif

        KString::SPtr result;
        KString::Create(result, GetAllocator(), pathA);

        KPath::CombineInPlace(*result, pathB);
        VERIFY_ARE_EQUAL(*result, expectedResult);
    }

    BOOST_AUTO_TEST_CASE(CreatePath_Success)
    {
#if !defined(PLATFORM_UNIX)
        KStringView pathA(L"C:\\test\\mcoskun");
        KStringView expectedResult(L"\\??\\C:\\test\\mcoskun");
#else
        KStringView pathA(L"C:/test/mcoskun");
        KStringView expectedResult(L"C:/test/mcoskun");
#endif

        KString::SPtr result = KPath::CreatePath(pathA, GetAllocator());
        VERIFY_ARE_EQUAL(*result, expectedResult);
    }

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(StartsWithGlobalDosDevicesNamespace_Yes_Yes)
    {
        KStringView pathA(L"\\??\\C:\\test\\");

        bool result = KPath::StartsWithGlobalDosDevicesNamespace(pathA);

        VERIFY_IS_TRUE(result);
    }

    BOOST_AUTO_TEST_CASE(StartsWithGlobalDosDevicesNamespace_OnlyGlobal_Yes)
    {
        KStringView pathA(L"\\??\\");

        bool result = KPath::StartsWithGlobalDosDevicesNamespace(pathA);

        VERIFY_IS_TRUE(result);
    }

    BOOST_AUTO_TEST_CASE(StartsWithGlobalDosDevicesNamespace_No_No)
    {
        KStringView pathA(L"C:\\test\\");

        bool result = KPath::StartsWithGlobalDosDevicesNamespace(pathA);

        VERIFY_IS_FALSE(result);
    }

    BOOST_AUTO_TEST_CASE(StartsWithGlobalDosDevicesNamespace_Empty_No)
    {
        KStringView pathA(L"");

        bool result = KPath::StartsWithGlobalDosDevicesNamespace(pathA);

        VERIFY_IS_FALSE(result);
    }

    BOOST_AUTO_TEST_CASE(StartsWithGlobalDosDevicesNamespace_ThreeChar_No)
    {
        KStringView pathA(L"\\??");

        bool result = KPath::StartsWithGlobalDosDevicesNamespace(pathA);

        VERIFY_IS_FALSE(result);
    }

    BOOST_AUTO_TEST_CASE(RemoveGlobalDosDevicesNamespaceIfExist_Exist_Remove)
    {
        KStringView testPathView(L"\\??\\C:\\test\\");
        KStringView expectPathView(L"C:\\test\\");

        KString::SPtr testPath = nullptr;
        NTSTATUS status = KString::Create(testPath, GetAllocator(), testPathView);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KString::CSPtr result = nullptr;
        status = KPath::RemoveGlobalDosDevicesNamespaceIfExist(*testPath, GetAllocator(), result);

        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(result->Compare(expectPathView) == 0);
    }

    BOOST_AUTO_TEST_CASE(RemoveGlobalDosDevicesNamespaceIfExist_Exist_NotPrefix_Same)
    {
        KStringView testPathView(L"C:\\test\\??\\test");

        KString::SPtr testPath = nullptr;
        NTSTATUS status = KString::Create(testPath, GetAllocator(), testPathView);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KString::CSPtr result = nullptr;
        status = KPath::RemoveGlobalDosDevicesNamespaceIfExist(*testPath, GetAllocator(), result);

        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(result->Compare(testPathView) == 0);
    }

    BOOST_AUTO_TEST_CASE(RemoveGlobalDosDevicesNamespaceIfExist_NotExist_Same)
    {
        KStringView testPathView(L"C:\\test\\");

        KString::SPtr testPath = nullptr;
        NTSTATUS status = KString::Create(testPath, GetAllocator(), testPathView);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KString::CSPtr result = nullptr;
        status = KPath::RemoveGlobalDosDevicesNamespaceIfExist(*testPath, GetAllocator(), result);

        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_TRUE(result->Compare(testPathView) == 0);
    }
#endif

    BOOST_AUTO_TEST_SUITE_END();
}
