// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "client/client.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Naming
{
    using namespace Common;
    using namespace Client;
    using namespace std;
    using namespace Api;
    using namespace Client;
    using namespace ServiceModel;

    class ComFabricClientTest
    {
    protected:
        HRESULT CreateComFabricClient(
            USHORT connectionStringsSize,
            LPCWSTR const *connectionStrings,
            REFIID riid,
            void ** fabricClient);
    };

    //
    // Temporarily overwrites a referenced value in the constructor, and restores the old value
    // in the destructor when it goes out of scope.
    //
    template<class T>
    class ScopeChange
    {
        DENY_COPY(ScopeChange);
    public:
        ScopeChange(T & target, T value) : original_(target), target_(target) { target_ = value; }
        ~ScopeChange() { target_ = original_; }
    private:
        T original_;
        T & target_;
    };

    // Test class to convert from async to sync.
    class ComTestCallback : public IFabricAsyncOperationCallback, private Common::ComUnknownBase
    {
        DENY_COPY(ComTestCallback)
        COM_INTERFACE_LIST1(ComTestCallback, IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
    public:
        ComTestCallback() : wait_(false) { }
        void STDMETHODCALLTYPE Invoke(IFabricAsyncOperationContext *) { wait_.Set(); }
        void Wait() { wait_.WaitOne(); }
    private:
        AutoResetEvent wait_;
    };

    void CheckHRESULT(
        HRESULT hr, 
        HRESULT expectedHr)
    {
         VERIFY_IS_TRUE_FMT(hr == expectedHr, 
            "CheckHRESULT: Expected {0}, got {1}", 
            ErrorCode::FromHResult(expectedHr).ErrorCodeValueToString().c_str(), 
            ErrorCode::FromHResult(hr).ErrorCodeValueToString().c_str());

    }

    //
    // Note: Parameter Validation is part of both the COM client API Wrapper(api\ComfabricClient) 
    // as well as the Client Implementation(Client\FabricClientImpl). 
    // Public type -> Internal Type is conversion is validated at the COM layer. The correctness of 
    // the internal type object is checked in the Client implementation.
    //
    void ExpectGatewayNotReachable(
        __in ComPointer<IFabricServiceManagementClient> & client,
        __in FABRIC_SERVICE_DESCRIPTION & service)
    {
        ComPointer<ComTestCallback> callback; callback.SetNoAddRef(new ComTestCallback());
        ComPointer<IFabricAsyncOperationContext> context;
        DWORD timeout = 2 * static_cast<DWORD>(Client::ClientConfig::GetConfig().ConnectionInitializationTimeout.TotalMilliseconds());
        HRESULT hr = client->BeginCreateService(
            &service, timeout, callback.GetRawPointer(), context.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            VERIFY_IS_TRUE(hr == S_OK);
            callback->Wait();
            hr = client->EndCreateService(context.GetRawPointer());
        }

        CheckHRESULT(hr, FABRIC_E_GATEWAY_NOT_REACHABLE);
    }

    void ExpectInvalidArg(
        __in ComPointer<IFabricServiceManagementClient> & client,
        __in FABRIC_SERVICE_DESCRIPTION & service)
    {
        ComPointer<ComTestCallback> callback; callback.SetNoAddRef(new ComTestCallback());
        ComPointer<IFabricAsyncOperationContext> context;
        HRESULT hr = client->BeginCreateService(
            &service, 0, callback.GetRawPointer(), context.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            VERIFY_IS_TRUE(hr == S_OK);
            callback->Wait();
            hr = client->EndCreateService(context.GetRawPointer());
        }
        
        CheckHRESULT(hr, E_INVALIDARG);
    }

    typedef std::function<HRESULT(DWORD, IFabricAsyncOperationCallback *, IFabricAsyncOperationContext **)> BeginOperationCallback;
    typedef std::function<HRESULT(IFabricAsyncOperationContext *)> EndOperationCallback;
    void ExpectInvalidNameUriError(
        BeginOperationCallback const & beginOperationCallback, 
        EndOperationCallback const & endOperationCallback = nullptr)
    {
        DWORD timeout = 1000;
        ComPointer<ComTestCallback> callback; callback.SetNoAddRef(new ComTestCallback());
        ComPointer<IFabricAsyncOperationContext> context;
        HRESULT hr = beginOperationCallback(timeout, callback.GetRawPointer(), context.InitializationAddress());
        if (SUCCEEDED(hr) && endOperationCallback != nullptr)
        {
            VERIFY_IS_TRUE(hr == S_OK);
            callback->Wait();
            hr = endOperationCallback(context.GetRawPointer());
        }

        CheckHRESULT(hr, FABRIC_E_INVALID_NAME_URI);
    }

    void ExpectInvalidArg(BeginOperationCallback const & operationCallback)
    {
        DWORD timeout = 1000;
        ComPointer<ComTestCallback> callback; callback.SetNoAddRef(new ComTestCallback());
        ComPointer<IFabricAsyncOperationContext> context;
        HRESULT hr = operationCallback(timeout, callback.GetRawPointer(), context.InitializationAddress());

        CheckHRESULT(hr, E_INVALIDARG);
    }

    HRESULT ComFabricClientTest::CreateComFabricClient(
        USHORT connectionStringsSize,
        LPCWSTR const *connectionStrings,
        REFIID riid,
        void ** fabricClient)
    {
        IClientFactoryPtr factoryPtr;
        auto error = ClientFactory::CreateClientFactory(
            connectionStringsSize,
            connectionStrings,
            factoryPtr);

        if (!error.IsSuccess()) { return error.ToHResult(); }

        ComPointer<Api::ComFabricClient> comClient;
        auto hr = Api::ComFabricClient::CreateComFabricClient(factoryPtr, comClient);

        if (FAILED(hr)) { return hr; }

        hr = comClient->QueryInterface(riid, fabricClient);

        return hr;
    }

    BOOST_FIXTURE_TEST_SUITE(ComFabricClientTestSuite,ComFabricClientTest)

    BOOST_AUTO_TEST_CASE(TestSecuritySettingsParameterValidation)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12340";
        ComPointer<IFabricClientSettings> comFabricClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricClientSettings,
            comFabricClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        FABRIC_SECURITY_CREDENTIALS securityCredentials;
        securityCredentials.Kind = FABRIC_SECURITY_CREDENTIAL_KIND_X509;
        securityCredentials.Value = NULL;

        HRESULT hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidCredentials);

        ULONG keepAliveInterval = 10;
        hr = comFabricClient->SetKeepAlive(keepAliveInterval);
        VERIFY_SUCCEEDED(hr, L"SetKeepAlive successful");

#ifndef PLATFORM_UNIX
        wstring storeName = X509Default::StoreName();
        wstring emptyString = L"";
        FABRIC_X509_CREDENTIALS x509Credentials = {0};
        x509Credentials.AllowedCommonNames = NULL;
        x509Credentials.AllowedCommonNameCount = 0;
        x509Credentials.ProtectionLevel = FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN;

        x509Credentials.StoreLocation = FABRIC_X509_STORE_LOCATION_CURRENTUSER;

        x509Credentials.StoreName = storeName.c_str();
        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
        x509Credentials.FindValue = NULL;
        securityCredentials.Value = &x509Credentials;

        hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidSubjectName);

        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYTHUMBPRINT;

        hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidX509Thumbprint);

        x509Credentials.FindValue = (void*)emptyString.c_str();

        hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidX509Thumbprint);

        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
        x509Credentials.FindValue = (void*)emptyString.c_str();

        hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidSubjectName);

        x509Credentials.StoreName = emptyString.c_str();
        hr = comFabricClient->SetSecurityCredentials(&securityCredentials);
        CheckHRESULT(hr, ErrorCodeValue::InvalidX509StoreName);
#endif
    }

    BOOST_AUTO_TEST_CASE(TestServiceParameterValidation)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12340";

        ComPointer<IFabricClientSettings> comFabricClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricClientSettings,
            comFabricClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        ComPointer<IFabricServiceManagementClient> client;
        hrClient = comFabricClient->QueryInterface(IID_IFabricServiceManagementClient, (void**)client.InitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        FABRIC_SERVICE_DESCRIPTION service; memset(&service, 0, sizeof(service));
        FABRIC_STATEFUL_SERVICE_DESCRIPTION stateful; memset(&stateful, 0, sizeof(stateful));
        FABRIC_UNIFORM_INT64_RANGE_PARTITION_SCHEME_DESCRIPTION partition; memset(&partition, 0, sizeof(partition));

        service.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        service.Value = &stateful;

        stateful.ServiceName = L"fabric:/MyService";
        stateful.ServiceTypeName = L"MyServiceType";
        stateful.InitializationData = NULL;
        stateful.InitializationDataSize = 0;
        stateful.PartitionScheme = FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE;
        stateful.PartitionSchemeDescription = &partition;
        stateful.TargetReplicaSetSize = 3;
        stateful.MinReplicaSetSize = 2;
        stateful.PlacementConstraints = NULL;
        stateful.CorrelationCount = 0;
        stateful.Correlations = NULL;
        stateful.MetricCount = 0;
        stateful.Metrics = NULL;
        stateful.HasPersistedState = TRUE;

        partition.PartitionCount = 3;
        partition.LowKey = 0;
        partition.HighKey = 99;

        Trace.WriteInfo("ComFabricClientTestSource", "Original service description should pass validation and timeout.");
        ExpectGatewayNotReachable(client, service);


        {
            Trace.WriteInfo("ComFabricClientTestSource", "Empty service metric name should fail.");
            FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION metric = {0};
            metric.Name = NULL;
            metric.Weight = FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
            metric.PrimaryDefaultLoad = 1;
            metric.SecondaryDefaultLoad = 1;
            ScopeChange<FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION *> descChange(
                stateful.Metrics,
                &metric);
            ScopeChange<ULONG> countChange(
                stateful.MetricCount,
                1);
            ExpectInvalidArg(client, service);
        }

        {
            Trace.WriteInfo("ComFabricClientTestSource", "Invalid correlation scheme should fail.");
            FABRIC_SERVICE_CORRELATION_DESCRIPTION correlation = {0};
            correlation.ServiceName = L"fabric:/AnotherService";
            correlation.Scheme = FABRIC_SERVICE_CORRELATION_SCHEME_INVALID;
            ScopeChange<FABRIC_SERVICE_CORRELATION_DESCRIPTION *> descChange(
                stateful.Correlations,
                &correlation);
            ScopeChange<ULONG> countChange(
                stateful.CorrelationCount,
                1);
            ExpectInvalidArg(client, service);
        }

        {
            Trace.WriteInfo("ComFabricClientTestSource", "Empty correlation service name should fail.");
            FABRIC_SERVICE_CORRELATION_DESCRIPTION correlation = {0};
            correlation.ServiceName = NULL;
            correlation.Scheme = FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
            ScopeChange<FABRIC_SERVICE_CORRELATION_DESCRIPTION *> descChange(
                stateful.Correlations,
                &correlation);
            ScopeChange<ULONG> countChange(
                stateful.CorrelationCount,
                1);
            ExpectInvalidArg(client, service);
        }

        {
            Trace.WriteInfo("ComFabricClientTestSource", "Correlation service name is same to the current service name should fail.");
            FABRIC_SERVICE_CORRELATION_DESCRIPTION correlation = {0};
            correlation.ServiceName = L"fabric:/MyService";
            correlation.Scheme = FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
            ScopeChange<FABRIC_SERVICE_CORRELATION_DESCRIPTION *> descChange(
                stateful.Correlations,
                &correlation);
            ScopeChange<ULONG> countChange(
                stateful.CorrelationCount,
                1);
            ExpectInvalidArg(client, service);
        }

        {
            Trace.WriteInfo("ComFabricClientTestSource", "Correlation service name is not a valid Uri fail.");
            FABRIC_SERVICE_CORRELATION_DESCRIPTION correlation = {0};
            correlation.ServiceName = L"MyService";
            correlation.Scheme = FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
            ScopeChange<FABRIC_SERVICE_CORRELATION_DESCRIPTION *> descChange(
                stateful.Correlations,
                &correlation);
            ScopeChange<ULONG> countChange(
                stateful.CorrelationCount,
                1);
            ExpectInvalidArg(client, service);
        }

        {
            Trace.WriteInfo("ComFabricClientTestSource", "Invalid placement constraint should fail.");
            ScopeChange<LPCWSTR> change(
                stateful.PlacementConstraints,
                L"Invalid Placement Constraint (TM)");
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Non-positive partition count should fail.");
            ScopeChange<LONG> change(
                partition.PartitionCount,
                0);
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Non-positive target replica set size should fail.");
            ScopeChange<LONG> change(
                stateful.TargetReplicaSetSize,
                0);
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "LowKey larger than HighKey should fail.");
            ScopeChange<LONGLONG> change(
                partition.LowKey,
                partition.HighKey + 1);
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "one key per partition should succeed.");
            // (high-low)+1: the +1 is because high and low are inclusive.  E.g. [0,1] includes 2 keys: 0 and 1.
            ScopeChange<LONG> change(
                partition.PartitionCount,
                static_cast<LONG>(partition.HighKey - partition.LowKey) + 1);
            ExpectGatewayNotReachable(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "key-range smaller than partition count should fail.");
            // (high-low)+2: the +2 is because of +1 from previous case, and +1 to make the range 1 too large.
            ScopeChange<LONG> change(
                partition.PartitionCount,
                static_cast<LONG>((partition.HighKey - partition.LowKey) + 2));
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "All int64 values in a single partition should succeed.");
            ScopeChange<LONGLONG> lowKeyChange(
                partition.LowKey,
                numeric_limits<LONGLONG>::min());
            ScopeChange<LONGLONG> highKeyChange(
                partition.HighKey,
                numeric_limits<LONGLONG>::max());
            ScopeChange<LONG> partitionCountChange(
                partition.PartitionCount,
                1);
            ExpectGatewayNotReachable(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Non-positive MinReplicaSetSize should fail.");
            ScopeChange<LONG> change(
                stateful.MinReplicaSetSize,
                0);
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "MinReplicaSetSize greater than TargetReplicaSetSize should fail.");
            ScopeChange<LONG> change(
                stateful.MinReplicaSetSize,
                stateful.TargetReplicaSetSize + 1);
            ExpectInvalidArg(client, service);
        }
    }

    BOOST_AUTO_TEST_CASE(TestServiceParameterValidationWithNamedPartition)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12341";

        ComPointer<IFabricClientSettings> comFabricClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricClientSettings,
            comFabricClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        ComPointer<IFabricServiceManagementClient> client;
        hrClient = comFabricClient->QueryInterface(IID_IFabricServiceManagementClient, (void**)client.InitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        FABRIC_SERVICE_DESCRIPTION service; memset(&service, 0, sizeof(service));
        FABRIC_STATEFUL_SERVICE_DESCRIPTION stateful; memset(&stateful, 0, sizeof(stateful));
        FABRIC_NAMED_PARTITION_SCHEME_DESCRIPTION partition; memset(&partition, 0, sizeof(partition));

        service.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        service.Value = &stateful;

        stateful.ServiceName = L"fabric:/MyService";
        stateful.ServiceTypeName = L"MyServiceType";
        stateful.InitializationData = NULL;
        stateful.InitializationDataSize = 0;
        stateful.PartitionScheme = FABRIC_PARTITION_SCHEME_NAMED;
        stateful.PartitionSchemeDescription = &partition;
        stateful.TargetReplicaSetSize = 3;
        stateful.MinReplicaSetSize = 2;
        stateful.PlacementConstraints = NULL;
        stateful.CorrelationCount = 0;
        stateful.Correlations = NULL;
        stateful.MetricCount = 0;
        stateful.Metrics = NULL;
        stateful.HasPersistedState = TRUE;

        {
            Trace.WriteInfo("ComFabricClientTestSource", "At least one partition name must be defined: test should fail");
            partition.PartitionCount = 0;
            vector<LPCWSTR> partitionNames;
            partition.Names = partitionNames.data();
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Empty partition names are not allowed: test should fail");
            vector<LPCWSTR> partitionNames;
            partitionNames.push_back(L"");
            partition.PartitionCount = 1;
            partition.Names = partitionNames.data();
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Duplicate partition names are not allowed: test should fail");
            vector<LPCWSTR> partitionNames;
            partitionNames.push_back(L"test");
            partitionNames.push_back(L"test");
            partition.PartitionCount = 2;
            partition.Names = partitionNames.data();
            ExpectInvalidArg(client, service);
        }
        {
            Trace.WriteInfo("ComFabricClientTestSource", "Different partition names are allowed: test should succeed");
            vector<LPCWSTR> partitionNames;
            partitionNames.push_back(L"A");
            partitionNames.push_back(L"B");
            partition.PartitionCount = 2;
            partition.Names = partitionNames.data();
            ExpectGatewayNotReachable(client, service);
        }
    }

    class MockServiceChangeHandler : public IFabricServicePartitionResolutionChangeHandler, private ComUnknownBase
    {
         BEGIN_COM_INTERFACE_LIST(MockServiceChangeHandler)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricServicePartitionResolutionChangeHandler)
            COM_INTERFACE_ITEM(IID_IFabricServicePartitionResolutionChangeHandler,IFabricServicePartitionResolutionChangeHandler)
        END_COM_INTERFACE_LIST()
    public:
        void STDMETHODCALLTYPE OnChange(IFabricServiceManagementClient *, LONGLONG, IFabricResolvedServicePartitionResult *, HRESULT) { }
    };

    BOOST_AUTO_TEST_CASE(TestInvalidRegisterCallbackParameters)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12340";

        // Make the max message size smaller, to test invalid request for too large name + partition name
        ServiceModelConfig::GetConfig().MaxMessageSize = 1024;

        ComPointer<IFabricPropertyManagementClient> propertyClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricPropertyManagementClient,
            propertyClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        ComPointer<IFabricServiceManagementClient> serviceClient;
        hrClient = propertyClient->QueryInterface(IID_IFabricServiceManagementClient, (void**)serviceClient.InitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        ComPointer<MockServiceChangeHandler> handler = make_com<MockServiceChangeHandler>();
        LONGLONG handle;
        HRESULT hr;

        // No name
        Trace.WriteInfo("ComFabricClientTestSource", "RegisterServicePartitionResolutionChangeHandler with null uri: test should fail");
        hr = serviceClient->RegisterServicePartitionResolutionChangeHandler(NULL, FABRIC_PARTITION_KEY_TYPE_NONE, NULL, handler.GetRawPointer(), &handle);
        CheckHRESULT(hr, E_POINTER);

        // Invalid Uri
        Trace.WriteInfo("ComFabricClientTestSource", "RegisterServicePartitionResolutionChangeHandler with invalid uri: test should fail");
        wstring invalidName = L"fabric://RegisterServicePartitionResolutionChangeHandler";
        hr = serviceClient->RegisterServicePartitionResolutionChangeHandler(invalidName.c_str(), FABRIC_PARTITION_KEY_TYPE_NONE, NULL, handler.GetRawPointer(), &handle);
        CheckHRESULT(hr, FABRIC_E_INVALID_NAME_URI);

        Trace.WriteInfo("ComFabricClientTestSource", "RegisterServicePartitionResolutionChangeHandler with null partition name: test should fail");
        wstring validName(L"fabric:/RegisterServicePartitionResolutionChangeHandler");
        hr = serviceClient->RegisterServicePartitionResolutionChangeHandler(validName.c_str(), FABRIC_PARTITION_KEY_TYPE_STRING, NULL, handler.GetRawPointer(), &handle);
        CheckHRESULT(hr, E_POINTER);

        Trace.WriteInfo("ComFabricClientTestSource", "RegisterServicePartitionResolutionChangeHandler empty partition name: test should fail");
        wstring partitionName;
        LPWSTR p(const_cast<LPWSTR>(partitionName.c_str()));
        hr = serviceClient->RegisterServicePartitionResolutionChangeHandler(validName.c_str(), FABRIC_PARTITION_KEY_TYPE_STRING, p, handler.GetRawPointer(), &handle);
        CheckHRESULT(hr, E_INVALIDARG);

        Trace.WriteInfo("ComFabricClientTestSource", "RegisterServicePartitionResolutionChangeHandler too large name/partition name: test should fail");
        wstring temp(CommonConfig::GetConfig().MaxNamingUriLength / 2 + 1, L'A');
        wstring longName;
        StringWriter(longName).Write("fabric:/{0}", temp);
        LPWSTR propPtr(const_cast<LPWSTR>(longName.c_str()));
        hr = serviceClient->RegisterServicePartitionResolutionChangeHandler(longName.c_str(), FABRIC_PARTITION_KEY_TYPE_STRING, propPtr, handler.GetRawPointer(), &handle);
        CheckHRESULT(hr, FABRIC_E_VALUE_TOO_LARGE);

        NamingConfig::GetConfig().Test_Reset();
        ClientConfig::GetConfig().Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(TestInvalidLpcwstrValidation)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12340";

        ComPointer<IFabricPropertyManagementClient> propertyClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricPropertyManagementClient,
            propertyClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        CommonConfig::GetConfig().MaxNamingUriLength = 13;
        ServiceModelConfig::GetConfig().MaxPropertyNameLength = 13;
        
        wstring invalidName(L"fabric:/NameThatGoesOverThe/MaxNamingUriSize");
        wstring invalidProperty(L"fabric:/PropertyThatGoesOverThe/MaxPropertySize");
        wstring invalidEmptyProperty(L"");
        wstring validName(L"fabric:/name1");
        wstring validProperty(L"fabric:/prop1");
        
        // Name operations
        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginCreateName(invalidName.c_str(), timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteName(invalidName.c_str(), timeout, callback, context);
            });

        // Property operations

        // Put
        BYTE byteData(0);
        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyBinary(invalidName.c_str(), validProperty.c_str(), 1, &byteData, timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyBinary(validName.c_str(), invalidProperty.c_str(), 1, &byteData, timeout, callback, context);
            });

         ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyBinary(validName.c_str(), invalidEmptyProperty.c_str(), 1, &byteData, timeout, callback, context);
            });

        // Delete
        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteProperty(invalidName.c_str(), validProperty.c_str(), timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteProperty(validName.c_str(), invalidProperty.c_str(), timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteProperty(validName.c_str(), invalidEmptyProperty.c_str(), timeout, callback, context);
            });

        // Get
        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetProperty(invalidName.c_str(), validProperty.c_str(), timeout, callback, context);
            });

         ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetProperty(validName.c_str(), invalidProperty.c_str(), timeout, callback, context);
            });

        // GetMetadata
        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetPropertyMetadata(invalidName.c_str(), validProperty.c_str(), timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetPropertyMetadata(validName.c_str(), invalidProperty.c_str(), timeout, callback, context);
            });

        ExpectInvalidArg([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetPropertyMetadata(validName.c_str(), invalidEmptyProperty.c_str(), timeout, callback, context);
            });

        NamingConfig::GetConfig().Test_Reset();
        CommonConfig::GetConfig().Test_Reset();
     }

    BOOST_AUTO_TEST_CASE(TestInvalidNameUriValidation)
    {
        LPCWSTR connectionStringItem = L"127.0.0.1:12340";

        ComPointer<IFabricPropertyManagementClient> propertyClient;
        HRESULT hrClient = CreateComFabricClient(
            1,
            &connectionStringItem,
            IID_IFabricPropertyManagementClient,
            propertyClient.VoidInitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        // ---------------------------------------------------------------------
        // IFabricPropertyManagementClient methods

        wstring invalidName(L"fabric://CreateName");
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginCreateName(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://DeleteName";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteName(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://NameExists";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginNameExists(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://EnumerateSubNames";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginEnumerateSubNames(invalidName.c_str(), NULL, false, timeout, callback, context);
            });

        invalidName = L"fabric://PutPropertyBinary";
        wstring propertyName(L"MyProperty");
        BYTE byteData(0);
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyBinary(invalidName.c_str(), propertyName.c_str(), 1, &byteData, timeout, callback, context);
            });

        invalidName = L"fabric://PutPropertyInt64";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyInt64(invalidName.c_str(), propertyName.c_str(), 0, timeout, callback, context);
            });

        invalidName = L"fabric://PutPropertyDouble";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyDouble(invalidName.c_str(), propertyName.c_str(), 0, timeout, callback, context);
            });

        invalidName = L"fabric://PutPropertyWString";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyWString(invalidName.c_str(), propertyName.c_str(), propertyName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://PutPropertyGuid";
        ::GUID guid;
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginPutPropertyGuid(invalidName.c_str(), propertyName.c_str(), &guid, timeout, callback, context);
            });

        invalidName = L"fabric://DeleteProperty";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginDeleteProperty(invalidName.c_str(), propertyName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://GetPropertyMetadata";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetPropertyMetadata(invalidName.c_str(), propertyName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://GetProperty";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginGetProperty(invalidName.c_str(), propertyName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://SubmitPropertyBatch";
        FABRIC_PROPERTY_BATCH_OPERATION batch; memset(&batch, 0, sizeof(batch));
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginSubmitPropertyBatch(invalidName.c_str(), 1, &batch, timeout, callback, context);
            });

        invalidName = L"fabric://EnumerateProperties";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return propertyClient->BeginEnumerateProperties(invalidName.c_str(), FALSE, NULL, timeout, callback, context);
            });

        // ---------------------------------------------------------------------
        // IFabricServiceManagementClient methods

        ComPointer<IFabricServiceManagementClient> serviceClient;
        hrClient = propertyClient->QueryInterface(IID_IFabricServiceManagementClient, (void**)serviceClient.InitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        FABRIC_SERVICE_DESCRIPTION service;
        FABRIC_STATEFUL_SERVICE_DESCRIPTION stateful;

        memset(&service, 0, sizeof(service));
        memset(&stateful, 0, sizeof(stateful));

        service.Kind = FABRIC_SERVICE_DESCRIPTION_KIND_STATEFUL;
        service.Value = &stateful;

        wstring typeName(L"AppType_App0:Package:Type");
        wstring validName(L"fabric:/CreateService.Application");
        invalidName = L"fabric://CreateService.Application";
        stateful.ApplicationName = invalidName.c_str();
        stateful.ServiceName = validName.c_str();
        stateful.ServiceTypeName = typeName.c_str();
        stateful.PartitionScheme = FABRIC_PARTITION_SCHEME_SINGLETON;
        stateful.InitializationData = NULL;
        stateful.InitializationDataSize = 0;
        stateful.TargetReplicaSetSize = 3;
        stateful.MinReplicaSetSize = 2;
        stateful.PlacementConstraints = NULL;
        stateful.CorrelationCount = 0;
        stateful.Correlations = NULL;
        stateful.MetricCount = 0;
        stateful.Metrics = NULL;
        stateful.HasPersistedState = TRUE;
        
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginCreateService(&service, timeout, callback, context);
            },
            [&](IFabricAsyncOperationContext *context) -> HRESULT{ return serviceClient->EndCreateService(context); });

        validName = L"fabric:/CreateService.Service";
        invalidName = L"fabric://CreateService.Service";
        stateful.ApplicationName = validName.c_str();
        stateful.ServiceName = invalidName.c_str();
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginCreateService(&service, timeout, callback, context);
            },
            [&](IFabricAsyncOperationContext *context) -> HRESULT{ return serviceClient->EndCreateService(context); });

        validName = L"fabric:/CreateServiceFromTemplate";
        invalidName = L"fabric://CreateServiceFromTemplate";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginCreateServiceFromTemplate(invalidName.c_str(), validName.c_str(), typeName.c_str(), 0, NULL, timeout, callback, context);
            });

        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginCreateServiceFromTemplate(validName.c_str(), invalidName.c_str(), typeName.c_str(), 0, NULL, timeout, callback, context);
            });


        invalidName = L"fabric://DeleteService";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginDeleteService(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://GetServiceDescription";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginGetServiceDescription(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://ResolveService";
        FABRIC_PARTITION_KEY_TYPE keyType = FABRIC_PARTITION_KEY_TYPE_NONE;
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return serviceClient->BeginResolveServicePartition(invalidName.c_str(), keyType, NULL, NULL, timeout, callback, context);
            });

        // --------------------------------------------------------------------
        // IFabricApplicationManagementClient methods

        ComPointer<IFabricApplicationManagementClient> managementClient;
        hrClient = propertyClient->QueryInterface(IID_IFabricApplicationManagementClient, (void**)managementClient.InitializationAddress());
        CheckHRESULT(hrClient, S_OK);

        invalidName = L"fabric://CreateApplication";
        FABRIC_APPLICATION_DESCRIPTION application;
        memset(&application, 0, sizeof(application));
        application.ApplicationName = invalidName.c_str();
        application.ApplicationTypeName = typeName.c_str();
        application.ApplicationTypeVersion = typeName.c_str();
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return managementClient->BeginCreateApplication(&application, timeout, callback, context);
            },
            [&](IFabricAsyncOperationContext *context) -> HRESULT{ return managementClient->EndCreateApplication(context); });

        invalidName = L"fabric://UpgradeApplication";
        FABRIC_APPLICATION_UPGRADE_DESCRIPTION upgrade;
        memset(&upgrade, 0, sizeof(upgrade));
        FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION policyDescription;
        memset(&policyDescription, 0, sizeof(policyDescription));
        policyDescription.RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        upgrade.ApplicationName = invalidName.c_str();
        upgrade.TargetApplicationTypeVersion = typeName.c_str();
        upgrade.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
        upgrade.UpgradePolicyDescription = &policyDescription;

        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return managementClient->BeginUpgradeApplication(&upgrade, timeout, callback, context);
            },
            [&](IFabricAsyncOperationContext *context) -> HRESULT{ return managementClient->EndUpgradeApplication(context); });

        invalidName = L"fabric://GetApplicationUpgradeProgress";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return managementClient->BeginGetApplicationUpgradeProgress(invalidName.c_str(), timeout, callback, context);
            });

        invalidName = L"fabric://DeleteApplication";
        ExpectInvalidNameUriError([&](DWORD timeout, IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context) -> HRESULT
            {
                return managementClient->BeginDeleteApplication(invalidName.c_str(), timeout, callback, context);
            });
    }

    BOOST_AUTO_TEST_SUITE_END()
}
