// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace DNS { namespace Test
{

    class DnsCacheTests
    {
    public:
        DnsCacheTests() { BOOST_REQUIRE(Setup()); }
        ~DnsCacheTests() { BOOST_REQUIRE(Cleanup()); }

        TEST_CLASS_SETUP(Setup);
        TEST_CLASS_CLEANUP(Cleanup);

    protected:
        KAllocator& GetAllocator() { return _pRuntime->NonPagedAllocator(); }

    protected:
        Runtime* _pRuntime;
        IDnsCache::SPtr _spCache;
    };

    bool DnsCacheTests::Setup()
    {
        _pRuntime = Runtime::Create();
        if (_pRuntime == nullptr)
        {
            return false;
        }

        DNS::CreateDnsCache(/*out*/_spCache, GetAllocator(), 100/*maxSize*/);
        return true;
    }

    bool DnsCacheTests::Cleanup()
    {
        _spCache = nullptr;

        _pRuntime->Delete();

        return true;
    }

    BOOST_FIXTURE_TEST_SUITE(DnsCacheTestSuite, DnsCacheTests);

    BOOST_AUTO_TEST_CASE(TestCacheMiss)
    {
        KString::SPtr spDnsName = KString::Create(L"miss.cache", GetAllocator());
        KString::SPtr spFabricName;
        if (DnsNameTypeUnknown != _spCache->Read(*spDnsName, /*out*/spFabricName))
        {
            VERIFY_FAIL(L"");
        }
    }

    BOOST_AUTO_TEST_CASE(TestCacheHit)
    {
        KString::SPtr spDnsName = KString::Create(L"hit.cache", GetAllocator());
        KString::SPtr spFabricName = KString::Create(L"fabric:/hit", GetAllocator());
        _spCache->Put(*spDnsName, *spFabricName);
        KString::SPtr spFabricName1;
        if (DnsNameTypeFabric != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        if (*spFabricName != *spFabricName1)
        {
            VERIFY_FAIL(L"");
        }

        if (!_spCache->IsServiceKnown(*spFabricName))
        {
            VERIFY_FAIL(L"");
        }
    }

    BOOST_AUTO_TEST_CASE(TestMarkNameAsPublic)
    {
        KString::SPtr spDnsName = KString::Create(L"mark.as.public", GetAllocator());
        KString::SPtr spFabricName = KString::Create(L"fabric:/mark/as/public", GetAllocator());

        _spCache->MarkNameAsPublic(*spDnsName);
        KString::SPtr spFabricName1;
        if (DnsNameTypePublic != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }
        _spCache->Put(*spDnsName, *spFabricName);

        if (DnsNameTypeFabric != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        _spCache->MarkNameAsPublic(*spDnsName);

        if (DnsNameTypeFabric != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        _spCache->Remove(*spFabricName);

        if (DnsNameTypeUnknown != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        _spCache->MarkNameAsPublic(*spDnsName);

        if (DnsNameTypePublic != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }
    }

    BOOST_AUTO_TEST_CASE(TestCacheRemove)
    {
        KString::SPtr spDnsName = KString::Create(L"cache.remove", GetAllocator());
        KString::SPtr spFabricName = KString::Create(L"fabric:/cache/remove", GetAllocator());

        _spCache->MarkNameAsPublic(*spDnsName);
        KString::SPtr spFabricName1;
        if (DnsNameTypePublic != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }
        _spCache->Remove(*spFabricName);
        if (DnsNameTypePublic != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }
        if (_spCache->IsServiceKnown(*spFabricName))
        {
            VERIFY_FAIL(L"");
        }

        _spCache->Put(*spDnsName, *spFabricName);

        if (DnsNameTypeFabric != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        if (!_spCache->IsServiceKnown(*spFabricName))
        {
            VERIFY_FAIL(L"");
        }

        _spCache->Remove(*spFabricName);

        if (DnsNameTypeUnknown != _spCache->Read(*spDnsName, /*out*/spFabricName1))
        {
            VERIFY_FAIL(L"");
        }

        if (_spCache->IsServiceKnown(*spFabricName))
        {
            VERIFY_FAIL(L"");
        }
    }

    BOOST_AUTO_TEST_CASE(TestCacheResize)
    {
        KString::SPtr spTmp;

        const int loop = 100;
        LONGLONG averageQueryTimeEmpty = 0;
        LONGLONG averageQueryTimeFull100 = 0;
        LONGLONG averageQueryTimeFull2000 = 0;
        LONGLONG averageQueryTimeFull30000 = 0;
        LONGLONG averageQueryTimeFull70000 = 0;

        // Single entry perf
        {
            KString::SPtr spFabricName = KString::Create(L"fabric:/cache.resize", GetAllocator());
            KString::SPtr spDnsName = KString::Create(L"cache.resize", GetAllocator());
            _spCache->Put(*spDnsName, *spFabricName);

            for (int i = 0; i < loop; i++)
            {
                LONGLONG start = KNt::GetPerformanceTime();
                _spCache->Read(*spDnsName, /*out*/spTmp);
                LONGLONG end = KNt::GetPerformanceTime();

                averageQueryTimeEmpty += (end - start);
            }

            averageQueryTimeEmpty /= loop;
        }

        // Fill the cache
        WCHAR wsz[128];
        for (int i = 0; i < 100000; i++)
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"cache.resize.%d", i);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());

            STRPRINT(wsz, ARRAYSIZE(wsz), L"fabric:/cache.resize.%d", i);

            KString::SPtr spFabric = KString::Create(wsz, GetAllocator());
            _spCache->Put(*spDns, *spFabric);
        }

        // Now measure the query time of a full cache

        // Time for a 100th item
        {
            KString::SPtr spDnsName100 = KString::Create(L"cache.resize.100", GetAllocator());
            for (int i = 0; i < loop; i++)
            {
                LONGLONG start = KNt::GetPerformanceTime();
                _spCache->Read(*spDnsName100, /*out*/spTmp);
                LONGLONG end = KNt::GetPerformanceTime();

                averageQueryTimeFull100 += (end - start);
            }

            averageQueryTimeFull100 /= loop;
        }

        // Time for a 2000th item
        {
            KString::SPtr spDnsName2000 = KString::Create(L"cache.resize.2000", GetAllocator());
            for (int i = 0; i < loop; i++)
            {
                LONGLONG start = KNt::GetPerformanceTime();
                _spCache->Read(*spDnsName2000, /*out*/spTmp);
                LONGLONG end = KNt::GetPerformanceTime();

                averageQueryTimeFull2000 += (end - start);
            }

            averageQueryTimeFull2000 /= loop;
        }

        // Time for a 30000th item
        {
            KString::SPtr spDnsName30000 = KString::Create(L"cache.resize.30000", GetAllocator());
            for (int i = 0; i < loop; i++)
            {
                LONGLONG start = KNt::GetPerformanceTime();
                _spCache->Read(*spDnsName30000, /*out*/spTmp);
                LONGLONG end = KNt::GetPerformanceTime();

                averageQueryTimeFull30000 += (end - start);
            }

            averageQueryTimeFull30000 /= loop;
        }

        // Time for a 70000th item
        {
            KString::SPtr spDnsName70000 = KString::Create(L"cache.resize.70000", GetAllocator());
            for (int i = 0; i < loop; i++)
            {
                LONGLONG start = KNt::GetPerformanceTime();
                _spCache->Read(*spDnsName70000, /*out*/spTmp);
                LONGLONG end = KNt::GetPerformanceTime();

                averageQueryTimeFull70000 += (end - start);
            }

            averageQueryTimeFull70000 /= loop;
        }

        fprintf(stdout, "Results: base %d, 100 %d, 2.000 %d, 30.000 %d, 70.000 %d\r\n",
            (int)averageQueryTimeEmpty, (int)averageQueryTimeFull100, (int)averageQueryTimeFull2000,
            (int)averageQueryTimeFull30000, (int)averageQueryTimeFull70000
        );
    }

    BOOST_AUTO_TEST_CASE(TestCacheMaxSize)
    {
        ULONG maxSize = 100;
        IDnsCache::SPtr spCache;
        DNS::CreateDnsCache(/*out*/spCache, GetAllocator(), maxSize);

        KString::SPtr spFabric;

        WCHAR wsz[128];
        for (ULONG i = 0; i < maxSize; i++)
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"publicName.%d", i);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());
            _spCache->MarkNameAsPublic(*spDns);
        }

        // Verify all names are actually public
        for (ULONG i = 0; i < maxSize; i++)
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"publicName.%d", i);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());
            if (DnsNameTypePublic != _spCache->Read(*spDns, /*out*/spFabric))
            {
                VERIFY_FAIL(L"");
            }
        }

        // Insert new item
        {
            KString::SPtr spDns = KString::Create(L"NewItem", GetAllocator());
            _spCache->MarkNameAsPublic(*spDns);
        }

        // The oldest item should be publicName.0, so it should not be in the queue
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"publicName.%d", 0);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());
            if (DnsNameTypeUnknown != _spCache->Read(*spDns, /*out*/spFabric))
            {
                VERIFY_FAIL(L"");
            }
        }

        // Query item .1, that should refresh it
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"publicName.%d", 1);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());
            if (DnsNameTypePublic != _spCache->Read(*spDns, /*out*/spFabric))
            {
                VERIFY_FAIL(L"");
            }
        }

        // Insert new item
        {
            KString::SPtr spDns = KString::Create(L"NewItem1", GetAllocator());
            _spCache->MarkNameAsPublic(*spDns);
        }

        // The oldest item should be publicName.2, so it should not be in the queue
        {
            STRPRINT(wsz, ARRAYSIZE(wsz), L"publicName.%d", 2);

            KString::SPtr spDns = KString::Create(wsz, GetAllocator());
            if (DnsNameTypeUnknown != _spCache->Read(*spDns, /*out*/spFabric))
            {
                VERIFY_FAIL(L"");
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}}
