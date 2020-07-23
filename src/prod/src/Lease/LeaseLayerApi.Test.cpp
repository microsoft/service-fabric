// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if defined(PLATFORM_UNIX)
#include "stdafx.h"
#endif
#include "LeaseLayerTestCommon.h"
#include "leaselayerpublic.h"

namespace LeaseLayerApiTest
{
    using namespace std;
    using namespace LeaseLayerTestCommon;

    const Common::StringLiteral TraceType("LeaseLayerApiTest");
 
    VOID WINAPI
    LeasingApplicationExpiredCallback(
        __in PVOID context
        )
    {
        Trace.WriteInfo(TraceType, "Application \"{0}\" has expired\n",
            context ? static_cast<LPCWSTR>(context) : L"(null)");
    }

    VOID WINAPI
    LeasingApplicationLeaseEstablishedCallback(
        __in HANDLE lease,
        __in LPCWSTR remoteLeasingApplicationIdentifier,
        __in PVOID context
        )
    {
        Trace.WriteInfo(TraceType, "Lease {0} between application \"{1}\" and remote application \"{2}\" has been established\n",
            lease,
            context ? static_cast<LPCWSTR>(context) : L"(null)",
            remoteLeasingApplicationIdentifier
            );
    }

    VOID WINAPI 
    RemoteLeasingApplicationExpiredCallback(
        __in LPCWSTR RemoteLeasingApplicationIdentifier,
        __in PVOID context
        )
    {
        Trace.WriteInfo(TraceType, "Remote application \"{0}\" has expired for application \"{1}\"\n",
            RemoteLeasingApplicationIdentifier,
            context ? static_cast<LPCWSTR>(context) : L"(null)"
            );
    }

    VOID WINAPI 
    LeasingApplicationArbitrateCallback(
        __in HANDLE localApplicationHandle,
        __in LONGLONG localInstance,
        __in LONG localTTL,
        __in PTRANSPORT_LISTEN_ENDPOINT remoteSocketAddress,
        __in LONGLONG remoteInstance,
        __in LONG remoteTTL,
        __in USHORT remoteVersion,
        __in LONGLONG monitorLeaseInstance,
        __in LONGLONG subjectLeaseInstance,
        __in LONG remoteArbitrationDurationUpperBound,
        __in LPCWSTR RemoteLeasingApplicationIdentifier,
        __in PVOID context
    )
    {
        UNREFERENCED_PARAMETER(localTTL);
        UNREFERENCED_PARAMETER(remoteSocketAddress);
        UNREFERENCED_PARAMETER(remoteTTL);
        UNREFERENCED_PARAMETER(remoteVersion);
        UNREFERENCED_PARAMETER(monitorLeaseInstance);
        UNREFERENCED_PARAMETER(subjectLeaseInstance);
        UNREFERENCED_PARAMETER(remoteArbitrationDurationUpperBound);
        UNREFERENCED_PARAMETER(RemoteLeasingApplicationIdentifier);
        UNREFERENCED_PARAMETER(context);
        Trace.WriteInfo(TraceType, "{0} needs to arbitrate for ({1}, {2})\n",
            localApplicationHandle,
            localInstance,
            remoteInstance
            );
    }

    VOID WINAPI 
    RemoteCertificateVerifyCallback(
        __in DWORD cbCertificate,
        __in PBYTE pbCertificate,
        __in PVOID certVerifyCallbackOperation,
        __in PVOID context
        )
    {
        UNREFERENCED_PARAMETER(cbCertificate);
        UNREFERENCED_PARAMETER(pbCertificate);
        UNREFERENCED_PARAMETER(certVerifyCallbackOperation);
        UNREFERENCED_PARAMETER(context);

        Trace.WriteInfo(TraceType, "Remote cert verify callback");
    }

    VOID WINAPI
    HealthReportCallback(
            __in int reportCode,
            __in LPCWSTR dynamicProperty,
            __in LPCWSTR extraDescription,
            __in PVOID context
        )
    {
        UNREFERENCED_PARAMETER(reportCode);
        UNREFERENCED_PARAMETER(dynamicProperty);
        UNREFERENCED_PARAMETER(extraDescription);
        UNREFERENCED_PARAMETER(context);

        Trace.WriteInfo(TraceType, "Health report callback");
    }

    class TestLeaseLayerApi
    {
    protected:
        DENY_COPY(TestLeaseLayerApi);
        
        TestLeaseLayerApi()
            :
            passCount_(0),
            failCount_(0),
            foundV4_(false),
            foundV6_(false)
        {
            ZeroMemory(&socketAddress1_, sizeof(TRANSPORT_LISTEN_ENDPOINT));
            ZeroMemory(&socketAddress2_, sizeof(TRANSPORT_LISTEN_ENDPOINT));
            ZeroMemory(&socketAddress6_1_, sizeof(TRANSPORT_LISTEN_ENDPOINT));
            ZeroMemory(&socketAddress6_2_, sizeof(TRANSPORT_LISTEN_ENDPOINT));
            BOOST_REQUIRE(SetupTest());
        }

        TEST_CLASS_SETUP(SetupTest);
        ~TestLeaseLayerApi()
        {
            PrintLeaseTaefTestSummary(passCount_.load(), failCount_.load());
            BOOST_REQUIRE(CleanupTest());
        }
        TEST_CLASS_CLEANUP(CleanupTest);

        // These aren't marked const because the Lease Layer API called takes in these structures non-const
        void RegisterLeasingApplicationTestApiInner(TRANSPORT_LISTEN_ENDPOINT &);
        void EstablishLeaseTestApiInner(TRANSPORT_LISTEN_ENDPOINT &, TRANSPORT_LISTEN_ENDPOINT &);
        void GetLeasingApplicationExpirationTimeTestApiInner(TRANSPORT_LISTEN_ENDPOINT &, TRANSPORT_LISTEN_ENDPOINT &);
        void TerminateUnregisterLeaseTestApiInner(TRANSPORT_LISTEN_ENDPOINT &,TRANSPORT_LISTEN_ENDPOINT &);
        void UpdateLeaseDurationTestApiInner(TRANSPORT_LISTEN_ENDPOINT &, TRANSPORT_LISTEN_ENDPOINT &);

        // Helpers to print last error
        HANDLE WINAPI RegisterLeasingApplicationLogErrorOnError(
            __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
            __in LPCWSTR LeasingApplicationIdentifier,
            __in LONG LeaseTimeToLiveMilliseconds,
            __in LONG LeaseSuspendDurationMilliseconds,
            __in LONG ArbitrationDurationMilliseconds,
            __in LONG NumOfLeaseRetry,
            __in LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
            __in REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
            __in_opt LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
            __in_opt LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
            __in_opt HEALTH_REPORT_CALLBACK HealthReportCallback,
            __in_opt PVOID CallbackContext
        );

        HANDLE WINAPI EstablishLeaseLogErrorOnError(
            __in HANDLE LeasingApplication,
            __in LPCWSTR RemoteApplicationIdentifier,
            __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
            );

        BOOL WINAPI GetLeasingApplicationExpirationTimeLogErrorOnError(
            __in HANDLE LeasingApplication,
            __out PLONG RemainingTimeToLiveMilliseconds,
            __out PLONGLONG KernelCurrentTime
            );

        BOOL WINAPI GetContainerLeasingApplicationExpirationTimeLogErrorOnError(
            __in HANDLE LeasingApplication,
            __in LONG RequestTTL,
            __out PLONG RemainingTimeToLiveMilliseconds,
            __out PLONGLONG KernelCurrentTime
            );

        BOOL WINAPI TestLeaseLayerApi::TerminateLeaseLogErrorOnError(
            __in HANDLE LeasingApplication,
            __in HANDLE Lease,
            __in LPCWSTR RemoteApplicationIdentifier
            );

        BOOL WINAPI TestLeaseLayerApi::UnregisterLeasingApplicationLogErrorOnError(__in HANDLE LeasingApplicationHandle);

        template <typename T>
        T ProcessResult(T result, T errorValue, wstring const & comment)
        {
            if (result == errorValue)
            {
                DWORD lastError = GetLastError();           
                Trace.WriteInfo(TraceType, "{0} failed with {1}", comment.c_str(), lastError);
                SetLastError(lastError);
            }

            return result;
        }
        
        TRANSPORT_LISTEN_ENDPOINT socketAddress1_;
        TRANSPORT_LISTEN_ENDPOINT socketAddress2_;
        TRANSPORT_LISTEN_ENDPOINT socketAddress6_1_;
        TRANSPORT_LISTEN_ENDPOINT socketAddress6_2_;

        Common::atomic_long passCount_; 
        Common::atomic_long failCount_;
        bool foundV4_;
        bool foundV6_;
 
    };

    
    HANDLE WINAPI TestLeaseLayerApi::RegisterLeasingApplicationLogErrorOnError(
        __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
        __in LPCWSTR LeasingApplicationIdentifier,
        __in LONG LeaseTimeToLiveMilliseconds,
        __in LONG LeaseSuspendDurationMilliseconds,
        __in LONG ArbitrationDurationMilliseconds,
        __in LONG NumOfLeaseRetry,
        __in LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
        __in REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
        __in_opt LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
        __in_opt LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
        __in_opt HEALTH_REPORT_CALLBACK HealthReportCallback,
        __in_opt PVOID CallbackContext
        )
    {
        BOOL SecurityEnable = FALSE;
        PBYTE CertHash = NULL;
        wstring CertHashStoreName(L"");
        LONG StartOfLeaseRetry = 2;
        HANDLE LeaseAgentHandle = NULL;
        LEASE_CONFIG_DURATIONS leaseDurations;

        leaseDurations.LeaseDuration = LeaseTimeToLiveMilliseconds;
        // EstablishLease test use 10000 as establish duration
        leaseDurations.LeaseDurationAcrossFD = 10000;

        HANDLE application = RegisterLeasingApplication(
            SocketAddress,
            LeasingApplicationIdentifier,
            &leaseDurations,
            LeaseSuspendDurationMilliseconds,
            ArbitrationDurationMilliseconds,
            NumOfLeaseRetry,
            StartOfLeaseRetry,
#if defined(PLATFORM_UNIX)
            NULL,
#else
            SecurityEnable,
            CertHash,
            CertHashStoreName.c_str(),
            28800000,
#endif
            1000, // LeasingAppExpiry defined in fed config, default to 1s (1000ms)
            LeasingApplicationExpired,
            RemoteLeasingApplicationExpired,
            LeasingApplicationArbitrate,
            LeasingApplicationLeaseEstablished,
#if !defined(PLATFORM_UNIX)
            RemoteCertificateVerifyCallback,
            HealthReportCallback,
#endif
            CallbackContext,
            NULL,
            &LeaseAgentHandle
            );

        return ProcessResult<HANDLE>(application, nullptr, L"RegisterLeasingApplication()");
    }
  
    HANDLE WINAPI TestLeaseLayerApi::EstablishLeaseLogErrorOnError(
        __in HANDLE LeasingApplication,
        __in LPCWSTR RemoteApplicationIdentifier,
        __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    )
    {
        int isEstablished(0);
        HANDLE lease = EstablishLease(
            LeasingApplication,
            RemoteApplicationIdentifier,
            RemoteSocketAddress,
            0,
            LEASE_DURATION,
            &isEstablished);

        return ProcessResult<HANDLE>(lease, nullptr, L"EstablishLease()");
    }

    BOOL WINAPI 
    TestLeaseLayerApi:: GetLeasingApplicationExpirationTimeLogErrorOnError(
        __in HANDLE LeasingApplication,
        __out PLONG RemainingTimeToLiveMilliseconds,
        __out PLONGLONG KernelCurrentTime
        )
    {
        LONG ConfigApplicationLeasingDuration = 1000;
        BOOL result = GetLeasingApplicationExpirationTime(
            LeasingApplication,
            ConfigApplicationLeasingDuration,
            RemainingTimeToLiveMilliseconds,
            KernelCurrentTime);
        
        return ProcessResult(result, FALSE, L"GetLeasingApplicationExpirationTime()");
    }

    BOOL WINAPI
        TestLeaseLayerApi::GetContainerLeasingApplicationExpirationTimeLogErrorOnError(
            __in HANDLE LeasingApplication,
            __in LONG RequestTTL,
            __out PLONG RemainingTimeToLiveMilliseconds,
            __out PLONGLONG KernelCurrentTime
        )
    {
        BOOL result = GetLeasingApplicationExpirationTime(
            LeasingApplication,
            RequestTTL,
            RemainingTimeToLiveMilliseconds,
            KernelCurrentTime);

        return ProcessResult(result, FALSE, L"GetLeasingApplicationExpirationTime()");
    }

    BOOST_FIXTURE_TEST_SUITE(TestLeaseLayerApiSuite,TestLeaseLayerApi)

    BOOST_AUTO_TEST_CASE(RegisterLeasingApplicationTestApi)
    {
        if (foundV4_)
        {
            Trace.WriteInfo(TraceType, "Calling RegisterLeasingApplicationTestApiInner with IPv4 addresses");
            RegisterLeasingApplicationTestApiInner(socketAddress1_);
        }

        if (foundV6_)
        {
            Trace.WriteInfo(TraceType, "Calling RegisterLeasingApplicationTestApiInner with IPv6 addresses");
            RegisterLeasingApplicationTestApiInner(socketAddress6_1_); 
        }
    }

#pragma prefast(push)
#pragma prefast(disable:6309, "Negative test case");


    BOOST_AUTO_TEST_CASE(EstablishLeaseTestApi)
    {
        if (foundV4_)
        {
            Trace.WriteInfo(TraceType, "Calling EstablishLeaseTestApiInner with IPv4 addresses");
            EstablishLeaseTestApiInner(socketAddress1_ ,socketAddress2_);
        }

        if (foundV6_)
        {
            Trace.WriteInfo(TraceType, "Calling EstablishLeaseTestApiInner with IPv6 addresses");
            EstablishLeaseTestApiInner(socketAddress6_1_,socketAddress6_2_);   
        }
    }


    BOOST_AUTO_TEST_CASE(GetLeasingApplicationExpirationTimeTestApi)
    {
        if (foundV4_)
        {
            Trace.WriteInfo(TraceType, "Calling GetLeasingApplicationExpirationTimeTestApiInner with IPv4 addresses");
            GetLeasingApplicationExpirationTimeTestApiInner(socketAddress1_ ,socketAddress2_);
        }

        if (foundV6_)
        {
            Trace.WriteInfo(TraceType, "Calling GetLeasingApplicationExpirationTimeTestApiInner with IPv6 addresses");
            GetLeasingApplicationExpirationTimeTestApiInner(socketAddress6_1_,socketAddress6_2_);  
        }    
    }


    BOOST_AUTO_TEST_CASE(TerminateUnregisterLeaseTestApi)
    {
        if (foundV4_)
        {
            Trace.WriteInfo(TraceType, "Calling TerminateUnregisterLeaseTestApiInner with IPv4 addresses");
            TerminateUnregisterLeaseTestApiInner(socketAddress1_  ,socketAddress2_);
        }

        if (foundV6_)
        {
            Trace.WriteInfo(TraceType, "Calling TerminateUnregisterLeaseTestApiInner with IPv6 addresses");
            TerminateUnregisterLeaseTestApiInner(socketAddress6_1_,socketAddress6_2_);
        }
    }
    

    BOOST_AUTO_TEST_CASE(UpdateLeaseDurationTestApi)
    {
        if (foundV4_)
        {
            Trace.WriteInfo(TraceType, "Calling UpdateLeaseDurationTestApiInner with IPv4 addresses");
            UpdateLeaseDurationTestApiInner(socketAddress1_ ,socketAddress2_);
        }

        if (foundV6_)
        {
            Trace.WriteInfo(TraceType, "Calling UpdateLeaseDurationTestApiInner with IPv6 addresses");
            UpdateLeaseDurationTestApiInner(socketAddress6_1_,socketAddress6_2_);   
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestLeaseLayerApi::SetupTest()
    {
        bool bRet = true;
        EtcmResult er(passCount_, failCount_);   
        VERIFY_IS_TRUE(GetAddressesHelper(socketAddress1_, socketAddress2_,socketAddress6_1_,socketAddress6_2_, foundV4_, foundV6_));

        er.MarkPass();
        return bRet;
    }

    bool TestLeaseLayerApi::CleanupTest()
    {
        bool bRet = true;

        Sleep(1500);

        return bRet;
    }

    void TestLeaseLayerApi::RegisterLeasingApplicationTestApiInner(TRANSPORT_LISTEN_ENDPOINT & sa1)
    {       
        HANDLE leasingApplication1 = nullptr;
        HANDLE leasingApplication2 = nullptr;
        EtcmResult er(passCount_, failCount_);
#if defined(PLATFORM_UNIX)
        wstring longString(1025,L'Q');
#else
        wstring longString(300,L'Q');
#endif
        wstring leasingApplicationIdentifier1 = L"RegAppApiApp";
        wstring leasingApplicationIdentifier2 = L"RegAppApiApp2";
        LONG lastError;
   
        //
        // Test 1st param == NULL 
        //      
        Trace.WriteInfo(TraceType, "RegisterLeasingApplication() with SocketAddress NULL ");
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            nullptr,
            leasingApplicationIdentifier1.c_str(), 
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
    
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1,L"Called RegisterLeasingApplication() with SocketAddress NULL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        
        // 
        // Test 2nd param == NULL
        //        
        Trace.WriteInfo(TraceType, "RegisterLeasingApplication() with LeasingApplicationIdentifier NULL ");
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            nullptr,
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with LeasingApplicationIdentifier NULL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test a "too long" string     
        Trace.WriteInfo(TraceType, "RegisterLeasingApplication() with LeasingApplicationIdentifier string length too long");
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            longString.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );  

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with LeasingApplicationIdentifier string length too long");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        //
        // Test another long string with no null terminator
        //
        vector<wchar_t> noNull(2000000,L'E');
        Trace.WriteInfo(TraceType, "RegisterLeasingApplication() with LeasingApplicationIdentifier with long string with no null terminator");
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            static_cast<LPCWSTR>(noNull.data()),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            ); 
        
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with LeasingApplicationIdentifier with no null terminator");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test 3rd param == NULL
        Trace.WriteInfo(TraceType, "RegisterLeasingApplication() with LeasingApplicationExpired NULL ");
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            5000,
            2000,
            10000,
            3,
            nullptr,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with LeasingApplicationExpired NULL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
            
        // Test 4th param == NULL
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            nullptr,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
        
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with RemoteLeasingApplicationExpired NULL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test negative TTL
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            -1,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with negative lease TTL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test 0 TTL
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            0,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with 0 lease TTL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test MAXLONG TTL
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            MAXLONG,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with MAXLONG lease TTL");
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        // Test 5th param == NULL
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            nullptr,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        VERIFY_IS_NOT_NULL(leasingApplication1, L"Called RegisterLeasingApplication() with LeasingApplicationArbitrate NULL");

        // Test 6th param == NULL
        leasingApplication2 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier2.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            nullptr,
            HealthReportCallback,
            nullptr
            );

        VERIFY_IS_NOT_NULL(leasingApplication2, L"Called RegisterLeasingApplication() with LeasingApplicationLeaseEstablished NULL");

        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication2));
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication1));
        
        er.MarkPass();
        
    }

    void TestLeaseLayerApi::EstablishLeaseTestApiInner(TRANSPORT_LISTEN_ENDPOINT & sa1, TRANSPORT_LISTEN_ENDPOINT & sa2)
    {   
        EtcmResult er(passCount_, failCount_);
        HANDLE leasingApplication1 = nullptr;
        HANDLE leasingApplication2 = nullptr;
        wstring leasingApplicationIdentifier1(L"EstApiApp1");
        wstring leasingApplicationIdentifier2(L"EstApiApp2");
        HANDLE lease = nullptr;
        LONG lastError;
            
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        VERIFY_IS_NOT_NULL(leasingApplication1);

        //
        // Register second app, this should work
        //
        leasingApplication2 = RegisterLeasingApplicationLogErrorOnError(
            &sa2,
            leasingApplicationIdentifier2.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );

        VERIFY_IS_NOT_NULL(leasingApplication2);

        // NULL for first param (LeasingApplication)        
        lease = EstablishLeaseLogErrorOnError(
            nullptr,
            leasingApplicationIdentifier2.c_str(),
            &sa2
            );      

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(lease);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        
        //
        // NULL for 3rd param
        //
        lease = EstablishLeaseLogErrorOnError(
            leasingApplication1,
            nullptr,
            &sa2
            );

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(lease);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        
#if defined(PLATFORM_UNIX)
        wstring longString(1025,L'Q');
#else
        wstring longString(300,L'Q');
#endif      
        lease = EstablishLeaseLogErrorOnError(
                leasingApplication1,
                longString.c_str(),
                &sa2
                );
        
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(lease);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        
        lease = EstablishLeaseLogErrorOnError(
            leasingApplication1,
            leasingApplicationIdentifier2.c_str(),
            nullptr
            );        

        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_NULL(lease);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        
        //
        // Valid inputs
        //
        lease = EstablishLeaseLogErrorOnError(
            leasingApplication1,
            leasingApplicationIdentifier2.c_str(),
            &sa2
            );
        VERIFY_IS_NOT_NULL(lease);
        Sleep(500);

        VERIFY_IS_TRUE(TerminateLeaseLogErrorOnError(leasingApplication1, lease, leasingApplicationIdentifier2.c_str()));
        Sleep(500);

        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication2));
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication1));
        Sleep(1000);

        er.MarkPass();

    }

    void TestLeaseLayerApi::GetLeasingApplicationExpirationTimeTestApiInner(TRANSPORT_LISTEN_ENDPOINT & sa1, TRANSPORT_LISTEN_ENDPOINT & sa2)
    {
        EtcmResult er(passCount_, failCount_);
        HANDLE leasingApplication1 = nullptr;
        HANDLE leasingApplication2 = nullptr; 
        wstring leasingApplicationIdentifier1(L"ExpTimeApiApp1");
        wstring leasingApplicationIdentifier2(L"ExpTimeApiApp2");
        HANDLE lease = nullptr;
        LONG remainingTTL = 0;
        LONGLONG kernelSystemTime = 0;
        LONG leaseDuration = 5000;
        LONG sleepWaitTime = 500;
        BOOL res;
        LONG lastError;

        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            leaseDuration,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
    
        VERIFY_IS_NOT_NULL(leasingApplication1);
            
        leasingApplication2 = RegisterLeasingApplicationLogErrorOnError(
            &sa2,
            leasingApplicationIdentifier2.c_str(),
            leaseDuration,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
    
        VERIFY_IS_NOT_NULL(leasingApplication2);
               
        lease = EstablishLeaseLogErrorOnError(
            leasingApplication1,
            leasingApplicationIdentifier2.c_str(),
            &sa2
            );

        VERIFY_IS_NOT_NULL(lease);
        Sleep(sleepWaitTime);

        // Lease just established, it should have TTL than the requestTTL
        LONG shortRequestTTL = leaseDuration - sleepWaitTime - 1000;
        VERIFY_IS_TRUE(GetContainerLeasingApplicationExpirationTimeLogErrorOnError(leasingApplication1, shortRequestTTL, &remainingTTL, &kernelSystemTime));
        Trace.WriteInfo(TraceType, "GetContainerLeasingApplicationExpirationTime test, leasing handle {0}, short request TTL: ({1}), returned TTL: ({2})",
            leasingApplication1, shortRequestTTL, remainingTTL);
        VERIFY_ARE_EQUAL(remainingTTL, shortRequestTTL);
        
        // actual TTL should be less than lease duration + suspend duration
        LONG longRequestTTL = leaseDuration + 3000;
        VERIFY_IS_TRUE(GetContainerLeasingApplicationExpirationTimeLogErrorOnError(leasingApplication1, longRequestTTL, &remainingTTL, &kernelSystemTime));
        Trace.WriteInfo(TraceType, "GetContainerLeasingApplicationExpirationTime test, leasing handle {0}, long request TTL: ({1}), returned TTL: ({2})",
            leasingApplication1, longRequestTTL, remainingTTL);
        VERIFY_IS_TRUE(remainingTTL < longRequestTTL);
        res = GetLeasingApplicationExpirationTimeLogErrorOnError(nullptr, &remainingTTL, &kernelSystemTime);
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_FALSE(res);
        
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        res = GetLeasingApplicationExpirationTimeLogErrorOnError(leasingApplication1, nullptr, nullptr);
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_FALSE(res);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);
        VERIFY_IS_TRUE(TerminateLeaseLogErrorOnError(leasingApplication1, lease, leasingApplicationIdentifier2.c_str()));
        Sleep(sleepWaitTime);

        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication2));
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication1));
        Sleep(1000);

        er.MarkPass();
    }

    void TestLeaseLayerApi::TerminateUnregisterLeaseTestApiInner(TRANSPORT_LISTEN_ENDPOINT & sa1, TRANSPORT_LISTEN_ENDPOINT & sa2)
    {
        EtcmResult er(passCount_, failCount_);
        HANDLE leasingApplication1 = nullptr;
        HANDLE leasingApplication2 = nullptr;
        wstring leasingApplicationIdentifier1(L"TermUnregApiApp1");
        wstring leasingApplicationIdentifier2(L"TermUnregApiApp2");
        HANDLE lease = nullptr;    
        LONG remainingTTL = 0;
        LONGLONG kernelSystemTime = 0;
        BOOL res;
        LONG lastError;
        
        //
        // Use valid inputs to register 2 apps
        //
        leasingApplication1 = RegisterLeasingApplicationLogErrorOnError(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
    
        VERIFY_IS_NOT_NULL(leasingApplication1);
    
        leasingApplication2 = RegisterLeasingApplicationLogErrorOnError(
            &sa2,
            leasingApplicationIdentifier2.c_str(),
            5000,
            2000,
            10000,
            3,
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
            HealthReportCallback,
            nullptr
            );
    
        VERIFY_IS_NOT_NULL(leasingApplication2);
        
        lease = EstablishLeaseLogErrorOnError(
            leasingApplication1,
            leasingApplicationIdentifier2.c_str(),
            &sa2
            );

        VERIFY_IS_NOT_NULL(lease);
        Sleep(500);

        // 
        // Valid inputs
        //
        VERIFY_IS_TRUE(GetLeasingApplicationExpirationTimeLogErrorOnError(leasingApplication1, &remainingTTL, &kernelSystemTime));
        VERIFY_IS_TRUE(remainingTTL >= 0);

        //
        // Terminate lease with invalid arg
        //                      
        res = TerminateLeaseLogErrorOnError(nullptr, nullptr, nullptr);
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_FALSE(res);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);           

        //
        // Valid inputs 
        //
        VERIFY_IS_TRUE(TerminateLeaseLogErrorOnError(leasingApplication1, lease, leasingApplicationIdentifier2.c_str()));
        Sleep(500);

        //
        // Unregister with invalid arg
        //
        res = UnregisterLeasingApplicationLogErrorOnError(nullptr);
        lastError = static_cast<LONG>(GetLastError());
        VERIFY_IS_FALSE(res);
        VERIFY_ARE_EQUAL(ERROR_INVALID_PARAMETER, lastError);

        //
        // Valid inputs
        //
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication2));
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication1));
        Sleep(1000);

        er.MarkPass();
    }


    void TestLeaseLayerApi::UpdateLeaseDurationTestApiInner(TRANSPORT_LISTEN_ENDPOINT & sa1, TRANSPORT_LISTEN_ENDPOINT & sa2)
    {
        EtcmResult er(passCount_, failCount_);
        HANDLE leasingApplication1 = nullptr;
        HANDLE leasingApplication2 = nullptr;
        HANDLE LeaseAgentHandle1 = nullptr;
        HANDLE LeaseAgentHandle2 = nullptr;
        wstring leasingApplicationIdentifier1(L"EstApiApp1");
        wstring leasingApplicationIdentifier2(L"EstApiApp2");

        HANDLE lease1 = nullptr;
        HANDLE lease2 = nullptr;
        BOOL SecurityEnable = FALSE;
        PBYTE CertHash = NULL;
        wstring CertHashStoreName(L"");
        LONG StartOfLeaseRetry = 2;
        LONG NumOfLeaseRetry = 3;
        PVOID CallbackContext = nullptr;

        LONG LeaseSuspendDuration = 2000;
        LONG ArbitrationDuration = 10000;
        LONG EstDuration1 = 1000;
        LONG EstDuration2 = 3000;

        LEASE_CONFIG_DURATIONS LeaseConfigDurations;
        LeaseConfigDurations.LeaseDuration = EstDuration1;
        LeaseConfigDurations.LeaseDurationAcrossFD = EstDuration2;
        LeaseConfigDurations.UnresponsiveDuration = 0;
        
        leasingApplication1 = RegisterLeasingApplication(
            &sa1,
            leasingApplicationIdentifier1.c_str(),
            &LeaseConfigDurations,
            LeaseSuspendDuration,
            ArbitrationDuration,
            NumOfLeaseRetry,
            StartOfLeaseRetry,
#if defined(PLATFORM_UNIX)
            NULL,
#else
            SecurityEnable,
            CertHash,
            CertHashStoreName.c_str(),
            28800000,
#endif
            1000, // LeasingAppExpiry defined in fed config, default to 1s (1000ms)
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
#if !defined(PLATFORM_UNIX)
            RemoteCertificateVerifyCallback,
            HealthReportCallback,
#endif
            CallbackContext,
            NULL,
            &LeaseAgentHandle1
            );

        VERIFY_IS_NOT_NULL(leasingApplication1);

        //
        // Register second app, this should work
        //
        leasingApplication2 = RegisterLeasingApplication(
            &sa2,
            leasingApplicationIdentifier2.c_str(),
            &LeaseConfigDurations,
            LeaseSuspendDuration,
            ArbitrationDuration,
            NumOfLeaseRetry,
            StartOfLeaseRetry,
#if defined(PLATFORM_UNIX)
            NULL,
#else
            SecurityEnable,
            CertHash,
            CertHashStoreName.c_str(),
            28800000,
#endif
            1000, // LeasingAppExpiry defined in fed config, default to 1s (1000ms)
            LeasingApplicationExpiredCallback,
            RemoteLeasingApplicationExpiredCallback,
            LeasingApplicationArbitrateCallback,
            LeasingApplicationLeaseEstablishedCallback,
#if !defined(PLATFORM_UNIX)
            RemoteCertificateVerifyCallback,
            HealthReportCallback,
#endif
            CallbackContext,
            NULL,
            &LeaseAgentHandle2
            );

        VERIFY_IS_NOT_NULL(leasingApplication2);

        // Establish lease from 1 to 2 using shorter duration (1000)
        int isEstablished(0);
        lease1 = EstablishLease(
            leasingApplication1,
            leasingApplicationIdentifier2.c_str(),
            &sa2,
            0,
            LEASE_DURATION,
            &isEstablished);

        VERIFY_IS_NOT_NULL(lease1);
        Sleep(EstDuration1);

        LONG DurationLeasingApp1 = QueryLeaseDuration(&sa1, &sa2);
        VERIFY_ARE_EQUAL(EstDuration1, DurationLeasingApp1);

        // Establish lease from 2 to 1 using longer duration (3000)
        lease2 = EstablishLease(
            leasingApplication2,
            leasingApplicationIdentifier1.c_str(),
            &sa1,
            0,
            LEASE_DURATION_ACROSS_FAULT_DOMAIN,
            &isEstablished);

        VERIFY_IS_NOT_NULL(lease2);
        Sleep(EstDuration2);

        // The current lease duration should be the longer one
        LONG DurationLeasingApp2 = QueryLeaseDuration(&sa1, &sa2);
        VERIFY_ARE_EQUAL(EstDuration2, DurationLeasingApp2);

        LONG UpdatedLeaseDuration = 5000;
        // Update regular duration from 1000 to 5000
        LeaseConfigDurations.LeaseDuration = UpdatedLeaseDuration;
        VERIFY_IS_TRUE(UpdateLeaseDuration(LeaseAgentHandle1, &LeaseConfigDurations));
        Sleep(UpdatedLeaseDuration);
        // After the update, the lease relationship should pick up the new duration
        VERIFY_ARE_EQUAL(UpdatedLeaseDuration, QueryLeaseDuration(&sa1, &sa2));
        VERIFY_ARE_EQUAL(UpdatedLeaseDuration, QueryLeaseDuration(&sa2, &sa1));


        // Update AcrossFD duration (which LeaseAgent2 used) from 3000 to 10000
        LONG UpdatedLeaseDurationAcrossFD = 10000;
        LeaseConfigDurations.LeaseDurationAcrossFD = UpdatedLeaseDurationAcrossFD;
        VERIFY_IS_TRUE(UpdateLeaseDuration(LeaseAgentHandle2, &LeaseConfigDurations));
        Sleep(UpdatedLeaseDurationAcrossFD);
        // After the update, the lease relationship should pick up the longer duration
        VERIFY_ARE_EQUAL(UpdatedLeaseDurationAcrossFD, QueryLeaseDuration(&sa1, &sa2));
        VERIFY_ARE_EQUAL(UpdatedLeaseDurationAcrossFD, QueryLeaseDuration(&sa2, &sa1));


        // Reduce the duration used in LeaseAgent1 5000 -> 1000
        LeaseConfigDurations.LeaseDuration = EstDuration1;
        VERIFY_IS_TRUE(UpdateLeaseDuration(LeaseAgentHandle1, &LeaseConfigDurations));
        Sleep(UpdatedLeaseDurationAcrossFD);
        // After the update, the lease relationship should still use the longest 
        // since the LeaseAgent2 is still using the 10000 as config
        VERIFY_ARE_EQUAL(UpdatedLeaseDurationAcrossFD, QueryLeaseDuration(&sa1, &sa2));
        VERIFY_ARE_EQUAL(UpdatedLeaseDurationAcrossFD, QueryLeaseDuration(&sa2, &sa1));


        // Reduce the duration used in LeaseAgent2 10000 -> 3000
        LeaseConfigDurations.LeaseDurationAcrossFD = EstDuration2;
        VERIFY_IS_TRUE(UpdateLeaseDuration(LeaseAgentHandle2, &LeaseConfigDurations));
        Sleep(UpdatedLeaseDurationAcrossFD * 2);
        // After the update, the lease relationship should be reduced
        VERIFY_ARE_EQUAL(EstDuration2, QueryLeaseDuration(&sa1, &sa2));
        VERIFY_ARE_EQUAL(EstDuration2, QueryLeaseDuration(&sa2, &sa1));


        // Clean up the lease establish and driver registeration
        VERIFY_IS_TRUE(TerminateLeaseLogErrorOnError(leasingApplication1, lease1, leasingApplicationIdentifier2.c_str()));
        Sleep(1000);
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication2));
        VERIFY_IS_TRUE(UnregisterLeasingApplicationLogErrorOnError(leasingApplication1));
        Sleep(1000);

        er.MarkPass();

    }

    BOOL WINAPI TestLeaseLayerApi::TerminateLeaseLogErrorOnError(
        __in HANDLE LeasingApplication,
        __in HANDLE Lease,
        __in LPCWSTR RemoteApplicationIdentifier
        )
    {    
        BOOL result = TerminateLease(LeasingApplication, Lease, RemoteApplicationIdentifier);

        return ProcessResult(result, FALSE, L"TerminateLease()");
    }

    BOOL WINAPI TestLeaseLayerApi::UnregisterLeasingApplicationLogErrorOnError(
        __in HANDLE LeasingApplicationHandle
        )
    {
        // 'false' indicates that the unregister/clean-up will not be delayed in kernel
        BOOL result = UnregisterLeasingApplication(LeasingApplicationHandle, false);     

        return ProcessResult(result, FALSE, L"UnregisterLeasingApplication()");
    }
#pragma prefast(pop)
} 
