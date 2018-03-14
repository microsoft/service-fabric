// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ClientTest
{
    using namespace Common;
    using namespace Client;
    using namespace std;
    using namespace Transport;
    using namespace Federation;
    using namespace ServiceModel;
        
    const StringLiteral FabricClientImplTestSource = "FabricClientImplTest";

    class ComDummyAddressChangeHandler : 
        public IFabricServicePartitionResolutionChangeHandler
        , public Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComDummyAddressChangeHandler)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServicePartitionResolutionChangeHandler)
            COM_INTERFACE_ITEM(IID_IFabricServicePartitionResolutionChangeHandler,IFabricServicePartitionResolutionChangeHandler)
        END_COM_INTERFACE_LIST()
    public:
        void STDMETHODCALLTYPE OnChange(
            /* [in] */ IFabricServiceManagementClient *,
            /* [in] */ LONGLONG handlerId,
            /* [in] */ IFabricResolvedServicePartitionResult *,
            /* [in] */ HRESULT error)
        {
            Trace.WriteInfo(FabricClientImplTestSource, "OnChange for {0}: error {1}", handlerId, error);
        }
    };

    class FabricClientImplTest
    {
    protected:
        ~FabricClientImplTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP( MethodCleanup );

    };

            
    BOOST_FIXTURE_TEST_SUITE(FabricClientImplTestSuite,FabricClientImplTest)

    BOOST_AUTO_TEST_CASE(FabricClientSettingsTest)
    {
        Trace.WriteInfo(FabricClientImplTestSource, "*** FabricClientSettingsTest");
        
        // Set initial config values (for easier validation)
        auto connectionInitializationTimeout = TimeSpan::FromSeconds(5);
        auto serviceChangePollInterval = TimeSpan::FromSeconds(10);
        ULONG partitionLocationCacheLimit = 133u;
        auto healthOperationTimeout = TimeSpan::FromSeconds(40);
        auto healthReportSendInterval = TimeSpan::FromSeconds(50);
        
        ULONG keepAliveIntervalInSeconds = ClientConfig::GetConfig().KeepAliveIntervalInSeconds;

        // There is no config for ConnectionIdleTimeoutInSeconds
        ULONG connectionIdleTimeoutInSeconds = 42;
        
        ClientConfig::GetConfig().ConnectionInitializationTimeout = connectionInitializationTimeout;
        ClientConfig::GetConfig().ServiceChangePollInterval = serviceChangePollInterval;
        ClientConfig::GetConfig().PartitionLocationCacheLimit = partitionLocationCacheLimit;
        ClientConfig::GetConfig().HealthOperationTimeout = healthOperationTimeout;
        ClientConfig::GetConfig().HealthReportSendInterval = healthReportSendInterval;

        vector<wstring> entreeServiceAddresses;
        entreeServiceAddresses.push_back(L"127.0.0.1:22222");

        shared_ptr<FabricClientImpl> client = make_shared<FabricClientImpl>(move(entreeServiceAddresses));
        
        // Check fabric client settings match config values
        FabricClientInternalSettingsSPtr crtSettings = client->Settings;
        VERIFY_IS_FALSE(crtSettings->IsStale);
        VERIFY_ARE_EQUAL(connectionInitializationTimeout, crtSettings->ConnectionInitializationTimeout);
        VERIFY_ARE_EQUAL(serviceChangePollInterval, crtSettings->ServiceChangePollInterval);
        VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
        VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
        VERIFY_ARE_EQUAL(healthOperationTimeout, crtSettings->HealthOperationTimeout);
        VERIFY_ARE_EQUAL(healthReportSendInterval, crtSettings->HealthReportSendInterval);
        VERIFY_ARE_EQUAL(0, crtSettings->ConnectionIdleTimeoutInSeconds);

        FabricClientInternalSettingsSPtr initialSettings = move(crtSettings);

        // Change the KeepAliveInterval through "IFabricClientSettings" methods
        keepAliveIntervalInSeconds = 66;
        VERIFY_IS_TRUE(client->SetKeepAlive(keepAliveIntervalInSeconds).IsSuccess());
        crtSettings = client->Settings;
        VERIFY_IS_TRUE(initialSettings->IsStale);
        VERIFY_IS_FALSE(crtSettings->IsStale);
        VERIFY_ARE_EQUAL(connectionInitializationTimeout, crtSettings->ConnectionInitializationTimeout);
        VERIFY_ARE_EQUAL(serviceChangePollInterval, crtSettings->ServiceChangePollInterval);
        VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
        VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
        VERIFY_ARE_EQUAL(healthOperationTimeout, crtSettings->HealthOperationTimeout);
        VERIFY_ARE_EQUAL(healthReportSendInterval, crtSettings->HealthReportSendInterval);
        
        // Change settings through new method
        ULONG connectionInitializationTimeoutInSeconds = 22u;
        ULONG serviceChangePollIntervalInSeconds = 123u;
        partitionLocationCacheLimit = 150u;
        keepAliveIntervalInSeconds = 77u;
        ULONG healthOperationTimeoutInSeconds = 44u;
        ULONG healthReportSendIntervalInSeconds = 55u;

        {
            FabricClientSettings newSettings(
                partitionLocationCacheLimit,
                keepAliveIntervalInSeconds,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds);

            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsSuccess(), L"client.SetSettings returned success");

            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_TRUE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);
            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }
        
        // Open the client
        VERIFY_IS_TRUE(client->Open().IsSuccess());
        
        // Setting new cache size not allowed
        {
            FabricClientSettings newSettings(
                partitionLocationCacheLimit + 45,
                keepAliveIntervalInSeconds,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds);

            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState), L"client.SetSettings returned InvalidArgument: cache size not dynamic");
        
            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_FALSE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);
            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }

        // Setting new keep alive interval is not allowed
        {
            FabricClientSettings newSettings(
                partitionLocationCacheLimit,
                keepAliveIntervalInSeconds + 23,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds);
            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState), L"client.SetSettings returned InvalidArgument: KeepAlive not dynamic");

            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_FALSE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);
            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }
        
        // Setting new connection idle timeout is not allowed
        {
            FabricClientSettings newSettings(
                partitionLocationCacheLimit,
                keepAliveIntervalInSeconds,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds + 1);
            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidState), L"client.SetSettings returned InvalidArgument: idle timeout not dynamic");

            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_FALSE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);
            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }


        // Changing connection initialization timeout is allowed
        {
            connectionInitializationTimeoutInSeconds = 4;
            FabricClientSettings newSettings(
                partitionLocationCacheLimit,
                keepAliveIntervalInSeconds,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds);
            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsSuccess(), L"client.SetSettings returned success");

            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_TRUE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);

            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }

        // Changing health operation timeout and send report interval is allowed
        {
            healthOperationTimeoutInSeconds = 80;
            healthReportSendIntervalInSeconds = 90;
            FabricClientSettings newSettings(
                partitionLocationCacheLimit,
                keepAliveIntervalInSeconds,
                serviceChangePollIntervalInSeconds,
                connectionInitializationTimeoutInSeconds,
                healthOperationTimeoutInSeconds,
                healthReportSendIntervalInSeconds,
                connectionIdleTimeoutInSeconds);
            auto error = client->SetSettings(move(newSettings));
            VERIFY_IS_TRUE(error.IsSuccess(), L"client.SetSettings returned success");

            initialSettings = move(crtSettings);
            crtSettings = client->Settings;
            VERIFY_IS_TRUE(initialSettings->IsStale);
            VERIFY_IS_FALSE(crtSettings->IsStale);

            VERIFY_ARE_EQUAL(connectionInitializationTimeoutInSeconds, crtSettings->ConnectionInitializationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(serviceChangePollIntervalInSeconds, crtSettings->ServiceChangePollInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(partitionLocationCacheLimit, crtSettings->PartitionLocationCacheLimit);
            VERIFY_ARE_EQUAL(keepAliveIntervalInSeconds, crtSettings->KeepAliveIntervalInSeconds);
            VERIFY_ARE_EQUAL(healthOperationTimeoutInSeconds, crtSettings->HealthOperationTimeout.TotalSeconds());
            VERIFY_ARE_EQUAL(healthReportSendIntervalInSeconds, crtSettings->HealthReportSendInterval.TotalSeconds());
            VERIFY_ARE_EQUAL(connectionIdleTimeoutInSeconds, crtSettings->ConnectionIdleTimeoutInSeconds);
        }
        
        VERIFY_IS_TRUE(client->Close().IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(FabricImageStoreClientTest)
    {
        Trace.WriteInfo(FabricClientImplTestSource, "*** FabricImageStoreClientTest");

        wstring imageStoreDirectory = Path::Combine(Directory::GetCurrentDirectoryW(), Guid::NewGuid().ToString());
        wstring imageStoreConnectionString = wformatString("file:{0}", imageStoreDirectory);

        auto error = Directory::Create2(imageStoreDirectory);
        VERIFY_IS_TRUE(error.IsSuccess(), L"Create ImageStore directory");

        vector<wstring> entreeServiceAddresses;
        entreeServiceAddresses.push_back(L"127.0.0.1:22222");

        shared_ptr<FabricClientImpl> client = make_shared<FabricClientImpl>(move(entreeServiceAddresses));

        wstring sourceFullPath = Path::Combine(Directory::GetCurrentDirectoryW(), Guid::NewGuid().ToString());
        FileWriter writer;
        writer.TryOpen(sourceFullPath);
        writer.WriteString("TestFile");
        writer.Close();

        wstring destinationRelativePath = Guid::NewGuid().ToString();
        error = client->Upload(
            imageStoreConnectionString,
            sourceFullPath,
            destinationRelativePath,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ImageStoreClient Upload.");
        VERIFY_IS_TRUE(File::Exists(Path::Combine(imageStoreDirectory, destinationRelativePath)), L"File Exists after upload");

        error = client->Delete(
            imageStoreConnectionString,
            destinationRelativePath);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ImageStoreClient Delete");
        VERIFY_IS_TRUE(!File::Exists(Path::Combine(imageStoreDirectory, destinationRelativePath)), L"File Exists after delete");

        File::Delete2(sourceFullPath);
        Directory::Delete(imageStoreDirectory, true, true);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool FabricClientImplTest::MethodCleanup()
    {
        ClientConfig::GetConfig().Test_Reset();

        return true;
    }

}
