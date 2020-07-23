// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

const StringLiteral TraceType("IPAddressProviderTest");

class DummyRoot : public Common::ComponentRoot
{
    public:
    ~DummyRoot() {}
};

#if defined(PLATFORM_UNIX)
class MockAzureVnetPluginProcessManager : public INetworkPluginProcessManager
{
    DENY_COPY(MockAzureVnetPluginProcessManager)

    public:
        MockAzureVnetPluginProcessManager() {};

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override
        {
            UNREFERENCED_PARAMETER(timeout);

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent);
        }

        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & operation) override
        {
            return CompletedAsyncOperation::End(operation);
        }

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override
        {
            UNREFERENCED_PARAMETER(timeout);

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent);
        }

        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & operation) override
        {
            return CompletedAsyncOperation::End(operation);
        }

        virtual void OnAbort() override
        {
            return;
        }
};
#endif

class MockIpamClient : public IIPAM
{
    DENY_COPY(MockIpamClient)

    public:
        MockIpamClient(
            int maxIPAddressPoolSize, 
            int reservedIpAddressCount,
            bool failRelease)
            : maxIpAddressPoolSize_(maxIPAddressPoolSize),
              reservedIpAddressCount_(reservedIpAddressCount),
              failRelease_(failRelease)
        {
        };

        virtual Common::AsyncOperationSPtr BeginInitialize(
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) override
        {
            UNREFERENCED_PARAMETER(timeout);

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                ErrorCodeValue::Success,
                callback,
                parent);
        }

        virtual Common::ErrorCode EndInitialize(Common::AsyncOperationSPtr const & operation) override
        {
            return CompletedAsyncOperation::End(operation);
        }

        virtual Common::ErrorCode StartRefreshProcessing(
            Common::TimeSpan const refreshInterval,
            Common::TimeSpan const refreshTimeout,
            std::function<void(Common::DateTime)> ghostChangeCallback) override
        {
            UNREFERENCED_PARAMETER(refreshInterval);
            UNREFERENCED_PARAMETER(refreshTimeout);
            UNREFERENCED_PARAMETER(ghostChangeCallback);

            return ErrorCodeValue::Success;
        }

        virtual void CancelRefreshProcessing()
        {
        }

        virtual Common::ErrorCode Reserve(std::wstring const &reservationId, uint &ip) override
        {
            UNREFERENCED_PARAMETER(reservationId);

            if (reservedIpAddressCount_ < maxIpAddressPoolSize_)
            {
                ip = baseIPAddress_ + reservedIpAddressCount_;
                reservedIpAddressCount_ += 1;
                return ErrorCodeValue::Success;
            }
            else
            {
                return ErrorCodeValue::IPAddressProviderAddressRangeExhausted;
            }
        }

        virtual Common::ErrorCode Release(std::wstring const &reservationId) override
        {
            UNREFERENCED_PARAMETER(reservationId);

            if (failRelease_)
            {
                return ErrorCodeValue::OperationFailed;
            }
            else
            {
                return ErrorCodeValue::Success;
            }
        }

        virtual std::list<std::wstring> GetGhostReservations() override
        {
            std::list<std::wstring> ghostReservations;
            return ghostReservations;
        }

        virtual Common::ErrorCode GetSubnetAndGatewayIpAddress(wstring &subnetCIDR, uint &gatewayIpAddress) override
        {
            gatewayIpAddress = 0;
            subnetCIDR = L"";
            return ErrorCodeValue::Success;
        }

    private:
        int maxIpAddressPoolSize_ = 0;
        int reservedIpAddressCount_ = 0;
        int baseIPAddress_ = 167772165; // 10.0.0.5
        bool failRelease_ = false;
};

class IPAddressProviderTest
{
protected:
    IPAddressProviderTest() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~IPAddressProviderTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);
};

BOOST_FIXTURE_TEST_SUITE(IPAddressProviderTestSuite, IPAddressProviderTest)

BOOST_AUTO_TEST_CASE(InitializeIPAddressProviderSuccessTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, false);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = true;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(ipAddressProvider->Initialized);
}

BOOST_AUTO_TEST_CASE(AcquireAndReleaseIPAddressNonAzureTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, false);

    std::wstring clusterId = L"";
    bool ipAddressProviderEnabled = true;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_FALSE(ipAddressProvider->Initialized);

    std::vector<std::wstring> assignedIps;
    std::vector<std::wstring> codePackageNames;
    codePackageNames.push_back(L"cp1");
    auto acquireError = ipAddressProvider->AcquireIPAddresses(L"node_0", L"service_0", codePackageNames, assignedIps);
    VERIFY_IS_TRUE(acquireError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::InvalidState).ErrorCodeValueToString());

    VERIFY_IS_TRUE(assignedIps.size() == 0);

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    VERIFY_IS_TRUE(reservedCodePackages.size() == 0);
    VERIFY_IS_TRUE(reservationIdCodePackageMap.size() == 0);

    auto relError = ipAddressProvider->ReleaseIpAddresses(L"node_0", L"service_0");
    VERIFY_IS_TRUE(relError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::InvalidState).ErrorCodeValueToString());
}

BOOST_AUTO_TEST_CASE(AcquireAndReleaseIPAddressProviderDisabledTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, false);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = false;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_FALSE(ipAddressProvider->Initialized);

    std::vector<std::wstring> assignedIps;
    std::vector<std::wstring> codePackageNames;
    codePackageNames.push_back(L"cp1");
    auto acquireError = ipAddressProvider->AcquireIPAddresses(L"node_0", L"service_0", codePackageNames, assignedIps);
    VERIFY_IS_TRUE(acquireError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::InvalidState).ErrorCodeValueToString());

    VERIFY_IS_TRUE(assignedIps.size() == 0);

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    VERIFY_IS_TRUE(reservedCodePackages.size() == 0);
    VERIFY_IS_TRUE(reservationIdCodePackageMap.size() == 0);

    auto relError = ipAddressProvider->ReleaseIpAddresses(L"node_0", L"service_0");
    VERIFY_IS_TRUE(relError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::InvalidState).ErrorCodeValueToString());
}

BOOST_AUTO_TEST_CASE(AcquireAndReleaseIPAddressSuccessTest)
{
    ErrorCode success(ErrorCodeValue::Success);

    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, false);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = true;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(ipAddressProvider->Initialized);

    std::vector<std::wstring> assignedIps;
    std::vector<std::wstring> codePackageNames;
    codePackageNames.push_back(L"cp1");
    codePackageNames.push_back(L"cp2");
    auto acquireError = ipAddressProvider->AcquireIPAddresses(L"node_0", L"service_0", codePackageNames, assignedIps);
    VERIFY_IS_TRUE(acquireError.ErrorCodeValueToString() == success.ErrorCodeValueToString());

    VERIFY_IS_TRUE(assignedIps.size() == 2);

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    VERIFY_IS_TRUE(reservedCodePackages.size() == 1);
    VERIFY_IS_TRUE(reservationIdCodePackageMap.size() == 2);

    int i = 0;
    for (auto & r : reservedCodePackages)
    {
        for (auto & s : r.second)
        {
            for (auto & c : s.second)
            {
                VERIFY_ARE_EQUAL(r.first, L"node_0");
                VERIFY_ARE_EQUAL(s.first, L"service_0");
                VERIFY_ARE_EQUAL(c, codePackageNames[i++]);
            }
        }
    }

    int j = 0;
    for (auto & r : reservationIdCodePackageMap)
    {
        auto reservationId = wformatString("{0}{1}", L"service_0", codePackageNames[j]);
        auto fullyQualifiedCodePkg = wformatString("{0},{1},{2}", L"node_0", L"service_0", codePackageNames[j++]);
        VERIFY_ARE_EQUAL(r.first, reservationId);
        VERIFY_ARE_EQUAL(r.second, fullyQualifiedCodePkg);
    }

    auto relError = ipAddressProvider->ReleaseIpAddresses(L"node_0", L"service_0");
    VERIFY_IS_TRUE(relError.ErrorCodeValueToString() == success.ErrorCodeValueToString());

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages1;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap1;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages1, reservationIdCodePackageMap1);

    VERIFY_IS_TRUE(reservedCodePackages1.size() == 0);
    VERIFY_IS_TRUE(reservationIdCodePackageMap1.size() == 0);
}

BOOST_AUTO_TEST_CASE(AcquireIPAddressesFailureTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, false);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = true;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(ipAddressProvider->Initialized);

    std::vector<std::wstring> assignedIps;
    std::vector<std::wstring> codePackageNames;
    codePackageNames.push_back(L"cp1");
    codePackageNames.push_back(L"cp2");
    codePackageNames.push_back(L"cp3");
    auto acquireError = ipAddressProvider->AcquireIPAddresses(L"node_0", L"service_0", codePackageNames, assignedIps);
    VERIFY_IS_TRUE(acquireError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::IPAddressProviderAddressRangeExhausted).ErrorCodeValueToString());

    VERIFY_IS_TRUE(assignedIps.size() == 0);

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    VERIFY_IS_TRUE(reservedCodePackages.size() == 0);
    VERIFY_IS_TRUE(reservationIdCodePackageMap.size() == 0);
}

BOOST_AUTO_TEST_CASE(ReleaseIPAddressesFailureTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, true);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = true;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_TRUE(ipAddressProvider->Initialized);

    std::vector<std::wstring> assignedIps;
    std::vector<std::wstring> codePackageNames;
    codePackageNames.push_back(L"cp1");
    codePackageNames.push_back(L"cp2");
    auto acquireError = ipAddressProvider->AcquireIPAddresses(L"node_0", L"service_0", codePackageNames, assignedIps);
    VERIFY_IS_TRUE(acquireError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::Success).ErrorCodeValueToString());

    VERIFY_IS_TRUE(assignedIps.size() == 2);

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages, reservationIdCodePackageMap);

    VERIFY_IS_TRUE(reservedCodePackages.size() == 1);
    VERIFY_IS_TRUE(reservationIdCodePackageMap.size() == 2);

    auto relError = ipAddressProvider->ReleaseIpAddresses(L"node_0", L"service_0");
    VERIFY_IS_TRUE(relError.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::OperationFailed).ErrorCodeValueToString());

    std::map<std::wstring, std::map<std::wstring, vector<std::wstring>>> reservedCodePackages1;
    std::map<std::wstring, std::wstring> reservationIdCodePackageMap1;
    ipAddressProvider->GetReservedCodePackages(reservedCodePackages1, reservationIdCodePackageMap1);

    VERIFY_IS_TRUE(reservedCodePackages1.size() == 1);
    VERIFY_IS_TRUE(reservationIdCodePackageMap1.size() == 2);

    int i = 0;
    for (auto & r : reservedCodePackages1)
    {
        for (auto & s : r.second)
        {
            for (auto & c : s.second)
            {
                VERIFY_ARE_EQUAL(r.first, L"node_0");
                VERIFY_ARE_EQUAL(s.first, L"service_0");
                VERIFY_ARE_EQUAL(c, codePackageNames[i++]);
            }
        }
    }

    int j = 0;
    for (auto & r : reservationIdCodePackageMap1)
    {
        auto reservationId = wformatString("{0}{1}", L"service_0", codePackageNames[j]);
        auto fullyQualifiedCodePkg = wformatString("{0},{1},{2}", L"node_0", L"service_0", codePackageNames[j++]);
        VERIFY_ARE_EQUAL(r.first, reservationId);
        VERIFY_ARE_EQUAL(r.second, fullyQualifiedCodePkg);
    }
}

BOOST_AUTO_TEST_CASE(StartRefreshProcessingInitializationFailureTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    IIPAMSPtr ipam = make_shared<MockIpamClient>(2, 0, true);

    std::wstring clusterId = L"test";
    bool ipAddressProviderEnabled = false;
    auto ipAddressProvider = new IPAddressProvider(ipam, clusterId, ipAddressProviderEnabled, root);

    ipAddressProvider->BeginOpen(
        TimeSpan::FromSeconds(1),
        [ipAddressProvider](AsyncOperationSPtr const & operation)
    {
        auto error = ipAddressProvider->EndOpen(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
    },
    AsyncOperationSPtr());

    VERIFY_IS_FALSE(ipAddressProvider->Initialized);

    auto error = ipAddressProvider->StartRefreshProcessing(
                 TimeSpan::FromMinutes(15.0),
                 TimeSpan::FromSeconds(5),
                 [ipAddressProvider](std::map<std::wstring, std::wstring> ghostReservations)
                 {});

    VERIFY_IS_TRUE(error.ErrorCodeValueToString() == ErrorCode(ErrorCodeValue::InvalidState).ErrorCodeValueToString());
}

BOOST_AUTO_TEST_SUITE_END()

bool IPAddressProviderTest::Setup()
{
    return true;
}

bool IPAddressProviderTest::Cleanup()
{
    return true;
}
