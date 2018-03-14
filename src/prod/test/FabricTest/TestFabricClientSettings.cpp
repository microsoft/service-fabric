// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Client;
using namespace Naming;
using namespace TestCommon;
using namespace Api;

const StringLiteral TraceSource("FabricTest.FabricClientSettings");

TestFabricClientSettings::TestFabricClientSettings() 
    : comFabricClient_()
    , client_()
    , queryClient_()
{
}

void TestFabricClientSettings::CreateFabricSettingsClient(
    __in FabricTestFederation & testFederation)
{
    if (client_.GetRawPointer() == nullptr)
    {
        ComPointer<IFabricClientSettings2> result;
        comFabricClient_ = TestFabricClient::FabricCreateComFabricClient(testFederation);
        HRESULT hr = comFabricClient_->QueryInterface(
            IID_IFabricClientSettings2, 
            (void **)result.InitializationAddress());
        TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricClientSettings2");
        client_ = move(result);
    }
}

bool TestFabricClientSettings::ForceOpen()
{
    if (queryClient_.GetRawPointer() == nullptr)
    {
        ComPointer<IFabricQueryClient> result2;
        HRESULT hr = comFabricClient_->QueryInterface(IID_IFabricQueryClient, (void**)result2.InitializationAddress());
        TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricQueryClient");
        queryClient_ = move(result2);
    }

    StringCollection queryParams;
    queryParams.push_back(L"getnodelist");

    return TestFabricClientQuery::Query(queryParams, queryClient_);
}

bool TestFabricClientSettings::SetSettings(StringCollection const & params)
{
    if (params.size() < 1)
    {
        return false;
    }

    CommandLineParser parser(params, 0);

    // Create client if it doesn't exist and remember current settings
    CreateFabricSettingsClient(FABRICSESSION.FabricDispatcher.Federation);

    Common::ComPointer<IFabricClientSettingsResult> settingsCPtr;
    auto hr = client_->GetSettings(settingsCPtr.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "GetSettings returned {0}", hr);

    FABRIC_CLIENT_SETTINGS const * crtSettings = settingsCPtr->get_Settings();

    int keepAliveIntervalInSeconds;
    parser.TryGetInt(
        L"keepalive", 
        keepAliveIntervalInSeconds, 
        static_cast<int>(crtSettings->KeepAliveIntervalInSeconds));

    int connectionInitializationTimeoutInSeconds;
    parser.TryGetInt(
        L"connectioninitializationtimeout", 
        connectionInitializationTimeoutInSeconds, 
        static_cast<int>(crtSettings->ConnectionInitializationTimeoutInSeconds));

    int serviceChangePollIntervalInSeconds;
    parser.TryGetInt(
        L"pollinterval", 
        serviceChangePollIntervalInSeconds, 
        static_cast<int>(crtSettings->ServiceChangePollIntervalInSeconds));

    int partitionLocationCacheLimit;
    parser.TryGetInt(
        L"cachesize", 
        partitionLocationCacheLimit, 
        static_cast<int>(crtSettings->PartitionLocationCacheLimit));

    int healthOperationTimeoutInSeconds;
    parser.TryGetInt(
        L"healthoperationtimeout", 
        healthOperationTimeoutInSeconds, 
        static_cast<int>(crtSettings->HealthOperationTimeoutInSeconds));

    int healthReportSendIntervalInSeconds;
    parser.TryGetInt(
        L"healthreportsendinterval", 
        healthReportSendIntervalInSeconds, 
        static_cast<int>(crtSettings->HealthReportSendIntervalInSeconds));

    std::wstring error;
    parser.TryGetString(L"error", error, L"Success");
    ErrorCodeValue::Enum errorValue = FABRICSESSION.FabricDispatcher.ParseErrorCode(error);
    HRESULT expectedError = ErrorCode(errorValue).ToHResult();
        
    ScopedHeap heap;
    auto newSettings = heap.AddItem<FABRIC_CLIENT_SETTINGS>();
    
    newSettings->KeepAliveIntervalInSeconds = static_cast<ULONG>(keepAliveIntervalInSeconds);
    newSettings->ConnectionInitializationTimeoutInSeconds = static_cast<ULONG>(connectionInitializationTimeoutInSeconds);
    newSettings->ServiceChangePollIntervalInSeconds = static_cast<ULONG>(serviceChangePollIntervalInSeconds);
    newSettings->PartitionLocationCacheLimit = static_cast<ULONG>(partitionLocationCacheLimit);
    newSettings->HealthOperationTimeoutInSeconds = static_cast<ULONG>(healthOperationTimeoutInSeconds);
    newSettings->HealthReportSendIntervalInSeconds = static_cast<ULONG>(healthReportSendIntervalInSeconds);

    TestSession::WriteInfo(
        TraceSource, 
        "SetSettings: keepAlive={0}/{1}, connectionInitializationTimeout={2}/{3}, pollInterval={4}/{5}, cachesize={6}/{7}",
        crtSettings->KeepAliveIntervalInSeconds,
        newSettings->KeepAliveIntervalInSeconds,
        crtSettings->ConnectionInitializationTimeoutInSeconds,
        newSettings->ConnectionInitializationTimeoutInSeconds,
        crtSettings->ServiceChangePollIntervalInSeconds,
        newSettings->ServiceChangePollIntervalInSeconds,
        crtSettings->PartitionLocationCacheLimit,
        newSettings->PartitionLocationCacheLimit);

    TestSession::WriteInfo(
        TraceSource, 
        "SetSettings: Health: timeout={0}/{1}, sendInterval={2}/{3}",
        crtSettings->HealthOperationTimeoutInSeconds,
        newSettings->HealthOperationTimeoutInSeconds,
        crtSettings->HealthReportSendIntervalInSeconds,
        newSettings->HealthReportSendIntervalInSeconds);

    bool forceOpen = parser.GetBool(L"forceopen");
    if (forceOpen)
    {
        if (!ForceOpen())
        {
            return false;
        }
    }

    hr = client_->SetSettings(newSettings.GetRawPointer());
    if (hr != expectedError)
    {
        TestSession::WriteError(
            TraceSource, 
            "SetSettings: expected error = {0}, actual {1}",
            ErrorCode::FromHResult(expectedError),
            ErrorCode::FromHResult(hr));
        return false;
    }

    // Get new settings and check values
    Common::ComPointer<IFabricClientSettingsResult> newSettingsCPtr;
    hr = client_->GetSettings(newSettingsCPtr.InitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "GetSettings returned {0}", hr);

    FABRIC_CLIENT_SETTINGS const * setSettings = newSettingsCPtr->get_Settings();

    FABRIC_CLIENT_SETTINGS const * expectedSettings;
    if (expectedError == S_OK)
    {
        expectedSettings = newSettings.GetRawPointer();
    }
    else
    {
        expectedSettings = crtSettings;
    }

    // Check that values are equal to expected ones
    if (expectedSettings->KeepAliveIntervalInSeconds != setSettings->KeepAliveIntervalInSeconds)
    {
        TestSession::WriteError(
            TraceSource, 
            "KeepAlive: expected {0} != {1}",
            expectedSettings->KeepAliveIntervalInSeconds,
            setSettings->KeepAliveIntervalInSeconds);
        return false;
    }

    if (expectedSettings->ServiceChangePollIntervalInSeconds != setSettings->ServiceChangePollIntervalInSeconds)
    {
        TestSession::WriteError(
            TraceSource, 
            "serviceChangePollIntervalInSeconds: expected {0} != {1}",
            expectedSettings->ServiceChangePollIntervalInSeconds,
            setSettings->ServiceChangePollIntervalInSeconds);
        return false;
    }

    if (expectedSettings->ConnectionInitializationTimeoutInSeconds != setSettings->ConnectionInitializationTimeoutInSeconds)
    {
        TestSession::WriteError(
            TraceSource, 
            "connectionInitializationTimeoutInSeconds: expected {0} != {1}",
            expectedSettings->ConnectionInitializationTimeoutInSeconds,
            setSettings->ConnectionInitializationTimeoutInSeconds);
        return false;
    }

    if (expectedSettings->PartitionLocationCacheLimit != setSettings->PartitionLocationCacheLimit)
    {
        TestSession::WriteError(
            TraceSource, 
            "partitionLocationCacheLimit: expected {0} != {1}",
            expectedSettings->PartitionLocationCacheLimit,
            setSettings->PartitionLocationCacheLimit);
        return false;
    }

    if (expectedSettings->HealthOperationTimeoutInSeconds != setSettings->HealthOperationTimeoutInSeconds)
    {
        TestSession::WriteError(
            TraceSource, 
            "healthOperationTimeoutInSeconds: expected {0} != {1}",
            expectedSettings->HealthOperationTimeoutInSeconds,
            setSettings->HealthOperationTimeoutInSeconds);
        return false;
    }

    if (expectedSettings->HealthReportSendIntervalInSeconds != setSettings->HealthReportSendIntervalInSeconds)
    {
        TestSession::WriteError(
            TraceSource, 
            "HealthReportSendIntervalInSeconds: expected {0} != {1}",
            expectedSettings->HealthReportSendIntervalInSeconds,
            setSettings->HealthReportSendIntervalInSeconds);
        return false;
    }

    return true;
}
