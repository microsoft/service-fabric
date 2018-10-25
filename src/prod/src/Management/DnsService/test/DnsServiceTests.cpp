// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

const ULONG LocalhostIP = 16777343;
LPCWSTR PartitionPrefix = L"--";
LPCWSTR PartitionSuffix = L"---";

namespace DNS { namespace Test
    {
        class DnsServiceTests
        {
        public:
            DnsServiceTests() : _port(0) { BOOST_REQUIRE(Setup()); }
            ~DnsServiceTests() { BOOST_REQUIRE(Cleanup()); }

            TEST_CLASS_SETUP(Setup);
            TEST_CLASS_CLEANUP(Cleanup);

        protected:
            KAllocator& GetAllocator() { return _pRuntime->NonPagedAllocator(); }

            DNS_STATUS QueryHelper(
                __in LPCWSTR wszQuestion,
                __in LPCWSTR wszFabricName,
                __in LPCWSTR wszAnswer,
                __in LPCWSTR wszARecord = L"",
                __in LPCWSTR wszTxtRecord = L"",
                __in LPCWSTR wszSrvRecord = L""
            );

        protected:
            USHORT _port;
            Runtime* _pRuntime;
            IDnsParser::SPtr _spParser;
            IDnsService::SPtr _spService;
            DnsServiceSynchronizer _dnsServiceSync;
            ComServiceManager::SPtr _spServiceManager;
            ComPropertyManager::SPtr _spPropertyManager;
            FabricData::SPtr _spData;
            INetIoManager::SPtr _spNetIoManager;
        };

        DNS_STATUS DnsServiceTests::QueryHelper(
            __in LPCWSTR wszQuestion,
            __in LPCWSTR wszFabricName,
            __in LPCWSTR wszAnswer,
            __in LPCWSTR wszARecord,
            __in LPCWSTR wszTxtRecord,
            __in LPCWSTR wszSrvRecord
        )
        {
            DNS_STATUS status = 0;

            KArray<KString::SPtr> arrResults(GetAllocator());
            KString::SPtr spAnswer = KString::Create(wszAnswer, GetAllocator());
            arrResults.Append(spAnswer);

            KStringView strA(wszARecord);
            KStringView strTxt(wszTxtRecord);
            KStringView strSrv(wszSrvRecord);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(wszFabricName, *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, wszFabricName);

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, wszQuestion, DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    if (0 != dnsData.DataStr->CompareNoCase(strA))
                    {
                        VERIFY_FAIL_FMT(L"Failed to verify DNS_TYPE_A, expected <%s>, got <%s>",
                            static_cast<LPCWSTR>(strA), static_cast<LPCWSTR>(*dnsData.DataStr));
                    }
                    else
                    {
                        fwprintf(stdout, L"DNS_TYPE_A <%s>\r\n", static_cast<LPCWSTR>(*dnsData.DataStr));
                    }
                }
                break;

                case DNS_TYPE_SRV:
                {
                    if (dnsData.DataStr != nullptr)
                    {
                        WCHAR wszSrvLower[1024];

                        STRPRINT(wszSrvLower, ARRAYSIZE(wszSrvLower), L"%s", static_cast<LPCWSTR>(strSrv));
                        CharLowerBuff(wszSrvLower, strSrv.Length());

                        int error = _wcsicmp(wszSrvLower, static_cast<LPCWSTR>(*dnsData.DataStr));
                        if (0 != error)
                        {
                            VERIFY_FAIL_FMT(L"Failed to verify DNS_TYPE_SRV, expected <%s>, got <%s>",
                                wszSrvLower, static_cast<LPCWSTR>(*dnsData.DataStr));
                        }
                        else
                        {
                            fwprintf(stdout, L"DNS_TYPE_SRV <%s>\r\n", static_cast<LPCWSTR>(*dnsData.DataStr));
                        }
                    }
                }
                break;

                case DNS_TYPE_TEXT:
                {
                    if (dnsData.DataStr != nullptr)
                    {
                        if (0 != strTxt.CompareNoCase(static_cast<LPCWSTR>(*dnsData.DataStr)))
                        {
                            VERIFY_FAIL_FMT(L"Failed to verify DNS_TYPE_TEXT, expected <%s>, got <%s>",
                                static_cast<LPCWSTR>(strTxt), static_cast<LPCWSTR>(*dnsData.DataStr));
                        }
                        else
                        {
                            fwprintf(stdout, L"DNS_TYPE_TEXT <%s>\r\n", static_cast<LPCWSTR>(strTxt));
                        }
                    }
                }
                break;
                }
            }

            return status;
        }

        bool DnsServiceTests::Setup()
        {
            _pRuntime = Runtime::Create();
            if (_pRuntime == nullptr)
            {
                return false;
            }

            DnsServiceParams params;
            params.IsRecursiveQueryEnabled = false;
            params.PartitionPrefix = KString::Create(PartitionPrefix, GetAllocator());
            params.PartitionSuffix = KString::Create(PartitionSuffix, GetAllocator());
            DnsHelper::CreateDnsServiceHelper(GetAllocator(), params,
                /*out*/_spService, /*out*/_spServiceManager, /*out*/_spPropertyManager,
                /*out*/_spData, /*out*/_spParser, /*out*/_spNetIoManager);

            if (!_spService->Open(/*inout*/_port, static_cast<DnsServiceCallback>(_dnsServiceSync)))
            {
                return false;
            }

            return true;
        }

        bool DnsServiceTests::Cleanup()
        {
            _spService->CloseAsync();
            _dnsServiceSync.Wait();

            _spService = nullptr;
            _spServiceManager = nullptr;
            _spPropertyManager = nullptr;
            _spData = nullptr;
            _spParser = nullptr;
            _spNetIoManager = nullptr;

            _pRuntime->Delete();

            return true;
        }

        BOOST_FIXTURE_TEST_SUITE(DnsServiceTestSuite, DnsServiceTests);

        BOOST_AUTO_TEST_CASE(TestAscii)
        {
            QueryHelper(L"microsoft.com", L"fabric:/foo", L"10.1.1.1:3450" /*answer*/,
                L"10.1.1.1" /*a*/, L"10.1.1.1:3450" /*txt*/, L"microsoft.com:3450"/*srv*/);
        }

        BOOST_AUTO_TEST_CASE(TestUTF8)
        {
            WCHAR wszQuestion[] = L"\u043f\u0412.\u0431\u0432";
            QueryHelper(wszQuestion, L"fabric:/foo", L"10.1.1.1:3450" /*answer*/,
                L"10.1.1.1" /*a*/, L"10.1.1.1:3450" /*txt*/, L"\u043f\u0412.\u0431\u0432:3450"/*srv*/);
        }

        BOOST_AUTO_TEST_CASE(TestLocalhost)
        {
            QueryHelper(L"www.foo.com", L"fabric:/foo", L"localhost:3450" /*answer*/,
                L"127.0.0.1" /*a*/, L"localhost:3450" /*txt*/, L"www.foo.com:3450"/*srv*/);
        }

        BOOST_AUTO_TEST_CASE(TestCacheHit)
        {
            // First query goes to the property manager, misses the cache
            //
            QueryHelper(L"google.com", L"fabric:/foo", L"10.1.1.1:1234" /*answer*/,
                L"10.1.1.1" /*a*/, L"10.1.1.1:1234" /*txt*/, L"google.com:1234"/*srv*/);

            // Second query does NOT hit the manager, hits the cache
            //
            QueryHelper(L"google.com", L"fabric:/bogus", L"bogus" /*answer*/,
                L"10.1.1.1" /*a*/, L"10.1.1.1:1234" /*txt*/, L"google.com:1234"/*srv*/);
        }

        BOOST_AUTO_TEST_CASE(TestBadRequest)
        {
            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            ULONG status = DnsHelper::QueryEx(*spBuffer, L"/?", DNS_TYPE_A, results);

            if (status != DNS_ERROR_RCODE_SERVER_FAILURE)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_ERROR_RCODE_SERVER_FAILURE);
            }
        }

        BOOST_AUTO_TEST_CASE(TestVerifyNoDuplicateAnswers)
        {
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());
            KString::SPtr spAnswer2 = KString::Create(L"10.0.0.1:1025", GetAllocator());
            KString::SPtr spAnswer3 = KString::Create(L"10.0.0.1:1025", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);
            arrResults.Append(spAnswer2);
            arrResults.Append(spAnswer3);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"duplicates.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestAnswerPlainUri)
        {
            KString::SPtr spAnswer1 = KString::Create(L"net.tcp://10.91.92.163:31025/CalculatorService/4d902da2-9e63-4077-8fb6-264752f845261313815433564114", GetAllocator());

            ComServicePartitionResult::SPtr spResult;
            ComServicePartitionResult::Create(/*out*/spResult, GetAllocator(), *spAnswer1.RawPtr());

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"long.uri.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;

            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestBadAnswer)
        {
            KString::SPtr spAnswer1 = KString::Create(L"dummyAnswer", GetAllocator());

            ComServicePartitionResult::SPtr spResult;
            ComServicePartitionResult::Create(/*out*/spResult, GetAllocator(), *spAnswer1.RawPtr());

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"long.uri.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 0)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 0)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestLargeAnswer)
        {
            // No more than 8 different records
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());
            KString::SPtr spAnswer2 = KString::Create(L"10.0.0.2:1025", GetAllocator());
            KString::SPtr spAnswer3 = KString::Create(L"10.0.0.3:1025", GetAllocator());
            KString::SPtr spAnswer4 = KString::Create(L"10.0.0.4:1025", GetAllocator());
            KString::SPtr spAnswer5 = KString::Create(L"10.0.0.5:1025", GetAllocator());
            KString::SPtr spAnswer6 = KString::Create(L"10.0.0.6:1025", GetAllocator());
            KString::SPtr spAnswer7 = KString::Create(L"10.0.0.7:1025", GetAllocator());
            KString::SPtr spAnswer8 = KString::Create(L"10.0.0.8:1025", GetAllocator());
            KString::SPtr spAnswer9 = KString::Create(L"10.0.0.9:1025", GetAllocator());
            KString::SPtr spAnswer10 = KString::Create(L"10.0.0.10:1035", GetAllocator());
            KString::SPtr spAnswer11 = KString::Create(L"10.0.0.11:1045", GetAllocator());
            KString::SPtr spAnswer12 = KString::Create(L"10.0.0.12:1055", GetAllocator());
            // duplicates
            KString::SPtr spAnswer13 = KString::Create(L"10.0.0.4:1025", GetAllocator());
            KString::SPtr spAnswer14 = KString::Create(L"10.0.0.5:1025", GetAllocator());
            KString::SPtr spAnswer15 = KString::Create(L"10.0.0.11:1045", GetAllocator());
            KString::SPtr spAnswer16 = KString::Create(L"10.0.0.12:1055", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);
            arrResults.Append(spAnswer2);
            arrResults.Append(spAnswer3);
            arrResults.Append(spAnswer4);
            arrResults.Append(spAnswer5);
            arrResults.Append(spAnswer6);
            arrResults.Append(spAnswer7);
            arrResults.Append(spAnswer8);
            arrResults.Append(spAnswer9);
            arrResults.Append(spAnswer10);
            arrResults.Append(spAnswer11);
            arrResults.Append(spAnswer12);
            arrResults.Append(spAnswer13);
            arrResults.Append(spAnswer14);
            arrResults.Append(spAnswer15);
            arrResults.Append(spAnswer16);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"largeanswer.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            // Expect 8 a records, they all need to be unique
            // srv records only 4 since srv are computed based on question + port
            // txt records 8, same as A record
            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 8)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 4)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 8)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestBadRequestWithRecursiveEnabled)
        {
            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            ULONG status = DnsHelper::QueryEx(*spBuffer, L"/?", DNS_TYPE_A, results);

            if (status != DNS_ERROR_RCODE_SERVER_FAILURE)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_ERROR_RCODE_SERVER_FAILURE)
            }
        }

        BOOST_AUTO_TEST_CASE(TestPublicSrvRequestWithRecursiveEnabled)
        {
            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            ULONG status = DnsHelper::QueryEx(*spBuffer, L"_ldap._tcp.redmond.corp.microsoft.com", DNS_TYPE_ANY, results);

            if (status != DNS_ERROR_RCODE_SERVER_FAILURE)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_ERROR_RCODE_SERVER_FAILURE);
            }

            USHORT id, flags, questionCount, answerCount, nsCount, adCount;
            _spParser->GetHeader(id, flags, questionCount, answerCount, nsCount, adCount, *spBuffer, spBuffer->QuerySize());

            bool fIsRecursionAvailable = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::RECURSION_AVAILABLE);
            bool fIsRecursionDesired = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::RECURSION_DESIRED);
            bool fTruncation = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::TRUNCATION);
            ULONG responseCode = DNS::DnsFlags::GetResponseCode(flags);
            // This query failed because of truncation, make sure the truncation flag is not set
            if (fTruncation)
            {
                VERIFY_FAIL_FMT(
                    L"Expected to fail due to Truncation, actual flags RC %d RA %d RD %d TC %d",
                    responseCode, fIsRecursionAvailable, fIsRecursionDesired, fTruncation);
            }
            // Also verify that RD and RA flags are set
            if (!fIsRecursionAvailable || !fIsRecursionDesired)
            {
                VERIFY_FAIL_FMT(
                    L"Expected to fail due to Truncation, actual flags RC %d RA %d RD %d TC %d",
                    responseCode, fIsRecursionAvailable, fIsRecursionDesired, fTruncation);
            }
        }

        BOOST_AUTO_TEST_CASE(TestNonExistentDomainWithRecursiveEnabled)
        {
            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            ULONG status = DnsHelper::QueryEx(*spBuffer, L"foo.bar.my", DNS_TYPE_ANY, results);

            if (status != DNS_ERROR_RCODE_NAME_ERROR)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_ERROR_RCODE_NAME_ERROR);
            }

            USHORT id, flags, questionCount, answerCount, nsCount, adCount;
            _spParser->GetHeader(id, flags, questionCount, answerCount, nsCount, adCount, *spBuffer, spBuffer->QuerySize());

            bool fIsRecursionAvailable = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::RECURSION_AVAILABLE);
            bool fIsRecursionDesired = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::RECURSION_DESIRED);
            bool fTruncation = DNS::DnsFlags::IsFlagSet(flags, DnsFlags::TRUNCATION);
            ULONG responseCode = DNS::DnsFlags::GetResponseCode(flags);

            if (fTruncation)
            {
                VERIFY_FAIL_FMT(
                    L"Expected to fail due to Truncation, actual flags RC %d RA %d RD %d TC %d",
                    responseCode, fIsRecursionAvailable, fIsRecursionDesired, fTruncation);
            }

            // Also verify that RD and RA flags are set
            if (!fIsRecursionAvailable || !fIsRecursionDesired)
            {
                VERIFY_FAIL_FMT(
                    L"Expected to fail due to Truncation, actual flags RC %d RA %d RD %d TC %d",
                    responseCode, fIsRecursionAvailable, fIsRecursionDesired, fTruncation);
            }
        }

        BOOST_AUTO_TEST_CASE(TestPublicAAAARequestWithRecursiveEnabled)
        {
            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            ULONG status = DnsHelper::QueryEx(*spBuffer, L"www.microsoft.com", DNS_TYPE_AAAA, results);

            if (status != NO_ERROR)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, NO_ERROR);
            }

            ULONG recCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_AAAA:
                {
                    recCount++;
                }
                break;
                }
            }

            if (recCount == 0)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestEmptyResponse)
        {
            // No more than 8 different records
            KString::SPtr spEmptyAnswer;
            KString::Create(/*out*/spEmptyAnswer, GetAllocator(), 10);
            spEmptyAnswer->SetNullTerminator();

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            ComServicePartitionResult::SPtr spResultImpl;
            ComServicePartitionResult::Create(/*out*/spResultImpl, GetAllocator(), *spEmptyAnswer);
            spResult = spResultImpl.RawPtr();

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"empty.answer.my", DNS_TYPE_ANY, results);

            if (status != DNS_ERROR_RCODE_SERVER_FAILURE)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_ERROR_RCODE_SERVER_FAILURE);
            }

            // Verify it doesn't crash
        }

        BOOST_AUTO_TEST_CASE(TestValidDnsNameWithUnsupportedQuestionType)
        {
            KString::SPtr spAnswer = KString::Create(L"10.0.0.1", GetAllocator());

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            ComServicePartitionResult::SPtr spResultImpl;
            ComServicePartitionResult::Create(/*out*/spResultImpl, GetAllocator(), *spAnswer);
            spResult = spResultImpl.RawPtr();

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"ipv6.my", DNS_TYPE_AAAA, results);

            if (status != DNS_INFO_NO_RECORDS)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, DNS_INFO_NO_RECORDS);
            }
        }

        BOOST_AUTO_TEST_CASE(TestFqdnResponse)
        {
            KString::SPtr spFqdnAnswer = KString::Create(L"www.microsoft.com", GetAllocator());

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            ComServicePartitionResult::SPtr spResultImpl;
            ComServicePartitionResult::Create(/*out*/spResultImpl, GetAllocator(), *spFqdnAnswer);
            spResult = spResultImpl.RawPtr();

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"fqdn.answer.my", DNS_TYPE_A, results);

            if (status != NO_ERROR)
            {
                VERIFY_FAIL_FMT(
                    L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                    status, NO_ERROR);
            }
        }

        BOOST_AUTO_TEST_CASE(TestFqdnResponseShuffle)
        {
            KString::SPtr spFqdnAnswer1 = KString::Create(L"www.microsoft.com", GetAllocator());
            KString::SPtr spFqdnAnswer2 = KString::Create(L"www.google.com", GetAllocator());
            KString::SPtr spFqdnAnswer3 = KString::Create(L"www.cnn.com", GetAllocator());
            KString::SPtr spFqdnAnswer4 = KString::Create(L"www.amazon.com", GetAllocator());
            KString::SPtr spFqdnAnswer5 = KString::Create(L"www.spacex.com", GetAllocator());
            KString::SPtr spFqdnAnswer6 = KString::Create(L"www.uber.com", GetAllocator());
            KString::SPtr spFqdnAnswer7 = KString::Create(L"www.facebook.com", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spFqdnAnswer1);
            arrResults.Append(spFqdnAnswer2);
            arrResults.Append(spFqdnAnswer3);
            arrResults.Append(spFqdnAnswer4);
            arrResults.Append(spFqdnAnswer5);
            arrResults.Append(spFqdnAnswer6);
            arrResults.Append(spFqdnAnswer7);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());

            for (int i = 0; i < 10; i++)
            {
                KArray<DNS_DATA> results(GetAllocator());
                DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"shuffle.answer.my", DNS_TYPE_A, results);

                if (status != NO_ERROR)
                {
                    VERIFY_FAIL_FMT(L"Failed to get valid answer from DNS server, status %d", status);
                }

                fprintf(stdout, "Shuffle results %d call:\r\n", i);
                for (ULONG j = 0; j < results.Count(); j++)
                {
                    DNS_DATA& data = results[j];
                    switch (data.Type)
                    {
                    case DNS_TYPE_A:
                    {
                        fwprintf(stdout, L"DNS_TYPE_A <%s>\r\n", static_cast<LPCWSTR>(*data.DataStr));
                    }
                    break;
                    }
                }
            }
        }

        BOOST_AUTO_TEST_CASE(TestFqdnLabel)
        {
            // Look for MSW (should resolve agains first suffix redmond.corp.microsoft.com)
            {
                KString::SPtr spFqdnLabelAnswer = KString::Create(L"msw", GetAllocator());

                ComPointer<IFabricResolvedServicePartitionResult> spResult;
                ComServicePartitionResult::SPtr spResultImpl;
                ComServicePartitionResult::Create(/*out*/spResultImpl, GetAllocator(), *spFqdnLabelAnswer);
                spResult = spResultImpl.RawPtr();

                ComFabricServiceDescriptionResult::SPtr spSdResult;
                ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

                _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

                ComPointer<IFabricPropertyValueResult> spPropResult;
                _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

                _spPropertyManager->SetResult(spPropResult);

                KBuffer::SPtr spBuffer;
                KBuffer::Create(4096, spBuffer, GetAllocator());
                KArray<DNS_DATA> results(GetAllocator());
                DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"fqdn.label.answer.my", DNS_TYPE_A, results);

                if (status != NO_ERROR)
                {
                    VERIFY_FAIL_FMT(
                        L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                        status, NO_ERROR);
                }
            }

            // Look for EUROPE (should resolve agains second suffix corp.microsoft.com)
            {
                KString::SPtr spFqdnLabelAnswer = KString::Create(L"europe", GetAllocator());

                ComPointer<IFabricResolvedServicePartitionResult> spResult;
                ComServicePartitionResult::SPtr spResultImpl;
                ComServicePartitionResult::Create(/*out*/spResultImpl, GetAllocator(), *spFqdnLabelAnswer);
                spResult = spResultImpl.RawPtr();

                ComFabricServiceDescriptionResult::SPtr spSdResult;
                ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_SINGLETON);

                _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

                ComPointer<IFabricPropertyValueResult> spPropResult;
                _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

                _spPropertyManager->SetResult(spPropResult);

                KBuffer::SPtr spBuffer;
                KBuffer::Create(4096, spBuffer, GetAllocator());
                KArray<DNS_DATA> results(GetAllocator());
                DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"fqdn.label.answer.my", DNS_TYPE_A, results);

                if (status != NO_ERROR)
                {
                    VERIFY_FAIL_FMT(
                        L"Received unexpected answer from the DnsService, actual <%d>, expected <%d>",
                        status, NO_ERROR);
                }
            }
        }

        BOOST_AUTO_TEST_CASE(TestManyRequests)
        {
            KAllocator& allocator = GetAllocator();
            IDnsService::SPtr spServiceInner;
            DnsServiceSynchronizer dnsServiceSyncInner;
            ComServiceManager::SPtr spServiceManagerInner;
            ComPropertyManager::SPtr spPropertyManagerInner;
            FabricData::SPtr spDataInner;
            IDnsParser::SPtr spParser;
            INetIoManager::SPtr spNetIoManager;

            DnsServiceParams params;
            params.IsRecursiveQueryEnabled = true;
            params.NumberOfConcurrentQueries = 10;
            DnsHelper::CreateDnsServiceHelper(allocator, params,
                /*out*/spServiceInner, /*out*/spServiceManagerInner, /*out*/spPropertyManagerInner,
                /*out*/spDataInner, /*out*/spParser, /*out*/spNetIoManager);

            USHORT port = 0;
            if (!spServiceInner->Open(/*inout*/port, static_cast<DnsServiceCallback>(dnsServiceSyncInner)))
            {
                VERIFY_FAIL(L"");
            }

            NullTracer::SPtr spTracer;
            NullTracer::Create(/*out*/spTracer, allocator);

            ISocketAddress::SPtr spAddress;
            DNS::CreateSocketAddress(/*out*/spAddress, allocator, LocalhostIP, htons(port));

            LPCWSTR questions[] = {
                L"www.microsoft.com" , L"www.google.com" , L"www.cnn.com" ,
                L"www.amazon.com", L"www.spacex.com", L"www.uber.com", L"www.facebook.com"
            };

            KBuffer::SPtr wbuffers[ARRAYSIZE(questions)];
            KBuffer::SPtr rbuffers[ARRAYSIZE(questions)];
            IUdpListener::SPtr listeners[ARRAYSIZE(questions)];
            DnsServiceSynchronizer listenerSyncs[ARRAYSIZE(questions)];
            USHORT clientPorts[ARRAYSIZE(questions)];
            ZeroMemory(clientPorts, sizeof(clientPorts[0]) * ARRAYSIZE(clientPorts));

            ISocketAddress::SPtr fromAddresses[ARRAYSIZE(questions)];
            DnsServiceSynchronizer readSyncs[ARRAYSIZE(questions)];
            DnsServiceSynchronizer writeSyncs[ARRAYSIZE(questions)];

            for (ULONG i = 0; i < ARRAYSIZE(questions); i++)
            {
                KBuffer::Create(4096, wbuffers[i], GetAllocator());
                KBuffer::Create(4096, rbuffers[i], GetAllocator());

                DWORD dwSize = wbuffers[i]->QuerySize();
                DnsHelper::SerializeQuestion(*spParser, questions[i], *wbuffers[i], DNS_TYPE_A, /*out*/dwSize, DnsTextEncodingTypeUTF8);

                spNetIoManager->CreateUdpListener(/*out*/listeners[i], false /*AllowMultipleListeners*/);

                if (!listeners[i]->StartListener(nullptr, /*inout*/clientPorts[i], listenerSyncs[i]))
                {
                    VERIFY_FAIL_FMT(L"Failed to start UDP listener");
                }

                listeners[i]->WriteAsync(*wbuffers[i], dwSize, *spAddress, writeSyncs[i]);

                DNS::CreateSocketAddress(/*out*/fromAddresses[i], allocator);
                listeners[i]->ReadAsync(*rbuffers[i], *fromAddresses[i], readSyncs[i], 5000);
            }

            for (ULONG i = 0; i < ARRAYSIZE(questions); i++)
            {
                if (readSyncs[i].Wait(5000))
                {
                    spTracer->Trace(DnsTraceLevel_Info, "Received answer for question {0}", WSTR(questions[i]));
                }
                else
                {
                    spTracer->Trace(DnsTraceLevel_Info, "Failed to receive answer for question {0}", WSTR(questions[i]));
                }
            }

            for (ULONG i = 0; i < ARRAYSIZE(questions); i++)
            {
                listeners[i]->CloseAsync();
                listenerSyncs[i].Wait(5000);
            }

            spServiceInner->CloseAsync();
            dnsServiceSyncInner.Wait();
        }

        BOOST_AUTO_TEST_CASE(TestNamedPartitionWithServiceDnsName)
        {
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_NAMED);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"partition.named.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestNamedPartitionWithPartitionDnsName)
        {
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_NAMED);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());

            WCHAR wszDnsName[1024];
            STRPRINT(wszDnsName, ARRAYSIZE(wszDnsName), L"partition%s1%s.named.my", PartitionPrefix, PartitionSuffix);

            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, wszDnsName, DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestInt64PartitionWithServiceDnsName)
        {
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());
            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, L"partition.int64.named.my", DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_CASE(TestInt64PartitionWithPartitionDnsName)
        {
            KString::SPtr spAnswer1 = KString::Create(L"10.0.0.1:1025", GetAllocator());

            KArray<KString::SPtr> arrResults(GetAllocator());
            arrResults.Append(spAnswer1);

            ComPointer<IFabricResolvedServicePartitionResult> spResult;
            _spData->SerializeServiceEndpoints(/*out*/spResult, arrResults);

            ComFabricServiceDescriptionResult::SPtr spSdResult;
            ComFabricServiceDescriptionResult::Create(/*out*/spSdResult, GetAllocator(), FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE);

            _spServiceManager->AddResult(L"fabric:/foo", *spResult.RawPtr(), *spSdResult.RawPtr());

            ComPointer<IFabricPropertyValueResult> spPropResult;
            _spData->SerializePropertyValue(/*out*/spPropResult, L"fabric:/foo");

            _spPropertyManager->SetResult(spPropResult);

            KBuffer::SPtr spBuffer;
            KBuffer::Create(4096, spBuffer, GetAllocator());
            KArray<DNS_DATA> results(GetAllocator());

            WCHAR wszDnsName[1024];
            STRPRINT(wszDnsName, ARRAYSIZE(wszDnsName), L"partition%s1%s.int64.named.my", PartitionPrefix, PartitionSuffix);

            DNS_STATUS status = DnsHelper::Query(*_spNetIoManager, *spBuffer, *_spParser, _port, wszDnsName, DNS_TYPE_ANY, results);

            if (status != 0)
            {
                VERIFY_FAIL_FMT(L"Failed to get valid answer to DNS server, status 0x%x", status);
            }

            ULONG aRecCount = 0;
            ULONG srvRecCount = 0;
            ULONG txtRecCount = 0;
            for (ULONG i = 0; i < results.Count(); i++)
            {
                DNS_DATA& dnsData = results[i];
                switch (dnsData.Type)
                {
                case DNS_TYPE_A:
                {
                    aRecCount++;
                }
                break;
                case DNS_TYPE_SRV:
                {
                    srvRecCount++;
                }
                break;
                case DNS_TYPE_TEXT:
                {
                    txtRecCount++;
                }
                break;
                }
            }

            if (aRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (srvRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
            if (txtRecCount != 1)
            {
                VERIFY_FAIL(L"");
            }
        }

        BOOST_AUTO_TEST_SUITE_END()
    }
}
