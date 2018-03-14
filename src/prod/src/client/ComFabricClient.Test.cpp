// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestClientFactory.h"
#include "Naming/ComCallbackWaiter.h"
#include "api/wrappers/ApiWrappers.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ClientTest
{
    using namespace Common;
    using namespace Client;
    using namespace std;
    using namespace ServiceModel;
    using namespace Api;
    using namespace Naming;

    const StringLiteral ComFabricClientTestSource = "ComFabricClientTest";

    class ComFabricClientTest
    {
    protected:
        ~ComFabricClientTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP( MethodCleanup );

    };


    //
    // This test verifies that the comfabricclient reference count is properly maintained
    // across begin and end calls.
    //
    BOOST_FIXTURE_TEST_SUITE(ComFabricClientTestSuite,ComFabricClientTest)

    BOOST_AUTO_TEST_CASE(RefCountTest)
    {
        FabricNodeConfigSPtr config = make_shared<FabricNodeConfig>();
        IClientFactoryPtr factoryPtr;
        DWORD timeout = 2000;
        auto error = TestClientFactory::CreateLocalClientFactory(config, factoryPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        ComPointer<ComFabricClient> comClient;
        auto hr = ComFabricClient::CreateComFabricClient(factoryPtr, comClient);
        VERIFY_IS_TRUE(!FAILED(hr));

        ComPointer<ComCallbackWaiter> callbackWaiter = make_com<ComCallbackWaiter>();
        ComPointer<IFabricAsyncOperationContext> asyncOperation;

        comClient->BeginResolveServicePartition(
            L"fabric:/System",
            FABRIC_PARTITION_KEY_TYPE_STRING,
            (void*)L"dummy",
            nullptr,
            timeout, // wait for 2 seconds
            callbackWaiter.GetRawPointer(), // callback
            asyncOperation.InitializationAddress()); // context

        // set the com pointer to null before the callback fires(2 seconds)
        comClient.Release();

        // wait for 10 seconds for the callback to finish successfully.
        bool callbackSuccessful = callbackWaiter->WaitOne(timeout + 8000);

        VERIFY_IS_TRUE(callbackSuccessful);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool ComFabricClientTest::MethodCleanup()
    {
        ClientConfig::GetConfig().Test_Reset();

        return true;
    }

}
