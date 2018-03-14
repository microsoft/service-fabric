// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <windns.h>
#include "Common/Config.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using Common::Environment;
using Common::StringCollection;
using Common::TimeSpan;
using namespace std;
using namespace Common;

namespace
{
    const wstring TransportSection = L"Transport";
    const wstring MaxHopKey = L"MaxHop";
    const wstring MessageTimeoutKey = L"MessageTimeout";
    const wstring TcpTransportDefaultHostKey = L"TcpTransportDefaultHost";
}

class ConfigForUpdateTest : ComponentConfig
{
    DECLARE_COMPONENT_CONFIG(ConfigForUpdateTest, "ConfigForUpdateTest")

    TEST_CONFIG_ENTRY(wstring, L"section1", key11, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    TEST_CONFIG_ENTRY(wstring, L"section1", key12, L"", Common::ConfigEntryUpgradePolicy::Static);
    TEST_CONFIG_ENTRY(wstring, L"section2", key21, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    TEST_CONFIG_ENTRY(wstring, L"section2", key22, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
};

class ReplaceConfigStoreInThisScope
{
public:
    ReplaceConfigStoreInThisScope(ConfigStoreSPtr const & configStore) : savedStore_(Config::Test_DefaultConfigStore())
    {
        Config::SetConfigStore(configStore);
    }

    ~ReplaceConfigStoreInThisScope()
    {
        Config::SetConfigStore(savedStore_);
    }

private:
    ConfigStoreSPtr savedStore_;
};

BOOST_AUTO_TEST_SUITE2(ConfigTests)

BOOST_AUTO_TEST_CASE(UpdateTestWithSectionAddingAndRemoving)
{
    ENTER;

    ConfigSettings configSettings;
    {
        ConfigParameter parameter11;
        parameter11.Name = L"key11";
        parameter11.Value = L"value11";

        ConfigParameter parameter12;
        parameter12.Name = L"key12";
        parameter12.Value = L"value12";

        ConfigSection section1;
        section1.Name = L"section1";
        section1.Parameters[parameter11.Name] = parameter11;
        section1.Parameters[parameter12.Name] = parameter12;

        ConfigParameter parameter21;
        parameter21.Name = L"key21";
        parameter21.Value = L"value21";

        ConfigParameter parameter22;
        parameter22.Name = L"key22";
        parameter22.Value = L"value22";

        ConfigSection section2;
        section2.Name = L"section2";
        section2.Parameters[parameter21.Name] = parameter21;
        section2.Parameters[parameter22.Name] = parameter22;

        configSettings.Sections[section1.Name] = section1;
        configSettings.Sections[section2.Name] = section2;

        Trace.WriteInfo(TraceType, "initial config settings: {0}", configSettings);
    }

    // Set up ComProxyConfigStore
    shared_ptr<ConfigSettingsConfigStore> configSettingsConfigStoreSPtr = make_shared<ConfigSettingsConfigStore>(configSettings);
    auto comProxyConfigStoreSPtr = ComProxyConfigStore::Create(
        [configSettingsConfigStoreSPtr](REFIID riid, IFabricConfigStoreUpdateHandler * updateHandler, void ** configStore)->HRESULT
        {
            ComPointer<IFabricConfigStoreUpdateHandler> updateHandlerCPtr;
            if (updateHandler)
            {
                updateHandlerCPtr.SetAndAddRef(updateHandler);
            }
            ComPointer<ComConfigStore> store = make_com<ComConfigStore>(configSettingsConfigStoreSPtr, updateHandlerCPtr);
            return ComUtility::OnPublicApiReturn(store->QueryInterface(riid, reinterpret_cast<void**>(configStore)));
        });
    comProxyConfigStoreSPtr->SetIgnoreUpdateFailures(true);

    ReplaceConfigStoreInThisScope replaceConfigStoreInThisScope(comProxyConfigStoreSPtr);
    ConfigSettings newConfigSettings;
    {
        ConfigParameter parameter11;
        parameter11.Name = L"key11";
        parameter11.Value = L"value11new";

        ConfigParameter parameter13;
        parameter13.Name = L"key13";
        parameter13.Value = L"value13";

        ConfigParameter parameter14;
        parameter14.Name = L"key14";
        parameter14.Value = L"value14";

        ConfigSection section1;
        section1.Name = L"section1";
        section1.Parameters[parameter11.Name] = parameter11;
        section1.Parameters[parameter13.Name] = parameter13;
        section1.Parameters[parameter14.Name] = parameter14;

        ConfigParameter parameter31;
        parameter31.Name = L"key31";
        parameter31.Value = L"value31";

        ConfigParameter parameter32;
        parameter32.Name = L"key32";
        parameter32.Value = L"value32";

        ConfigSection section3;
        section3.Name = L"section3";
        section3.Parameters[parameter31.Name] = parameter31;
        section3.Parameters[parameter32.Name] = parameter32;

        newConfigSettings.Sections[section1.Name] = section1;
        newConfigSettings.Sections[section3.Name] = section3;

        Trace.WriteInfo(TraceType, "new config settings: {0}", newConfigSettings);
    }

    // Register update handlers
    ConfigForUpdateTest testConfig;
    AutoResetEvent key11Updated(false);
    testConfig.key11Entry.AddHandler(
        [&testConfig, &key11Updated] (EventArgs const &)
        {
            BOOST_REQUIRE(testConfig.key11 == L"value11new");
            key11Updated.Set();
        });

    bool key12Updated = false;
    testConfig.key12Entry.AddHandler(
        [&testConfig, &key12Updated] (EventArgs const &)
        {
            key12Updated = true;
        });

    AutoResetEvent key21Updated(false);
    testConfig.key21Entry.AddHandler(
        [&testConfig, &key21Updated] (EventArgs const &)
        {
            BOOST_REQUIRE(testConfig.key21 == L"");
            key21Updated.Set();
        });

    AutoResetEvent key22Updated(false);
    testConfig.key22Entry.AddHandler(
        [&testConfig, &key22Updated] (EventArgs const &)
        {
            BOOST_REQUIRE(testConfig.key22 == L"");
            key22Updated.Set();
        });

    // Trigger config updates
    auto updateSuccess = configSettingsConfigStoreSPtr->Update(newConfigSettings);

    BOOST_REQUIRE(key11Updated.WaitOne(TimeSpan::FromSeconds(5)));
    BOOST_REQUIRE(key21Updated.WaitOne(TimeSpan::FromSeconds(5)));
    BOOST_REQUIRE(key22Updated.WaitOne(TimeSpan::FromSeconds(5)));

    Trace.WriteInfo(TraceType, "key11 = {0}", testConfig.key11);
    Trace.WriteInfo(TraceType, "key12 = {0}", testConfig.key12);
    Trace.WriteInfo(TraceType, "key21 = {0}", testConfig.key21);
    Trace.WriteInfo(TraceType, "key22 = {0}", testConfig.key22);

    BOOST_REQUIRE(!updateSuccess);
    BOOST_REQUIRE(!key12Updated);
    BOOST_REQUIRE(testConfig.key12 == L"");

    LEAVE;
}

BOOST_AUTO_TEST_CASE(BasicUpdatetest)
{
    ENTER;

    ConfigSettings configSettings;
    {
        ConfigParameter parameter11;
        parameter11.Name = L"key11";
        parameter11.Value = L"value11";

        ConfigParameter parameter12;
        parameter12.Name = L"key12";
        parameter12.Value = L"value12";

        ConfigSection section1;
        section1.Name = L"section1";
        section1.Parameters[parameter11.Name] = parameter11;
        section1.Parameters[parameter12.Name] = parameter12;

        ConfigParameter parameter21;
        parameter21.Name = L"key21";
        parameter21.Value = L"value21";

        ConfigParameter parameter22;
        parameter22.Name = L"key22";
        parameter22.Value = L"value22";

        ConfigSection section2;
        section2.Name = L"section2";
        section2.Parameters[parameter21.Name] = parameter21;
        section2.Parameters[parameter22.Name] = parameter22;

        configSettings.Sections[section1.Name] = section1;
        configSettings.Sections[section2.Name] = section2;

        Trace.WriteInfo(TraceType, "initial config settings: {0}", configSettings);
    }

    // Set up ComProxyConfigStore
    shared_ptr<ConfigSettingsConfigStore> configSettingsConfigStoreSPtr = make_shared<ConfigSettingsConfigStore>(configSettings);
    auto comProxyConfigStoreSPtr = ComProxyConfigStore::Create(
        [configSettingsConfigStoreSPtr](REFIID riid, IFabricConfigStoreUpdateHandler * updateHandler, void ** configStore)->HRESULT
        {
            ComPointer<IFabricConfigStoreUpdateHandler> updateHandlerCPtr;
            if (updateHandler)
            {
                updateHandlerCPtr.SetAndAddRef(updateHandler);
            }
            ComPointer<ComConfigStore> store = make_com<ComConfigStore>(configSettingsConfigStoreSPtr, updateHandlerCPtr);
            return ComUtility::OnPublicApiReturn(store->QueryInterface(riid, reinterpret_cast<void**>(configStore)));
        });
    configSettingsConfigStoreSPtr->SetIgnoreUpdateFailures(true);

    ReplaceConfigStoreInThisScope replaceConfigStoreInThisScope(comProxyConfigStoreSPtr);

    ConfigSettings newConfigSettings;
    {
        ConfigParameter parameter11;
        parameter11.Name = L"key11";
        parameter11.Value = L"value11new";

        ConfigParameter parameter12;
        parameter12.Name = L"key12";
        parameter12.Value = L"value12new";

        ConfigSection section1;
        section1.Name = L"section1";
        section1.Parameters[parameter11.Name] = parameter11;
        section1.Parameters[parameter12.Name] = parameter12;

        ConfigParameter parameter21;
        parameter21.Name = L"key21";
        parameter21.Value = L"value21";

        ConfigParameter parameter22;
        parameter22.Name = L"key22";
        parameter22.Value = L"value22new";

        ConfigSection section2;
        section2.Name = L"section2";
        section2.Parameters[parameter21.Name] = parameter21;
        section2.Parameters[parameter22.Name] = parameter22;

        newConfigSettings.Sections[section1.Name] = section1;
        newConfigSettings.Sections[section2.Name] = section2;

        Trace.WriteInfo(TraceType, "new config settings: {0}", newConfigSettings);
    }

    // Register update handlers
    ConfigForUpdateTest testConfig;
    AutoResetEvent key11Updated(false);
    testConfig.key11Entry.AddHandler(
        [&testConfig, &key11Updated] (EventArgs const &)
        {
            BOOST_REQUIRE(testConfig.key11 == L"value11new");
            key11Updated.Set();
        });

    bool key12Updated = false;
    testConfig.key12Entry.AddHandler(
        [&testConfig, &key12Updated] (EventArgs const &)
        {
            key12Updated = true;
        });

    bool key21Updated = false;
    testConfig.key21Entry.AddHandler(
        [&testConfig, &key21Updated] (EventArgs const &)
        {
            key21Updated = true;
        });

    AutoResetEvent key22Updated(false);
    testConfig.key22Entry.AddHandler(
        [&testConfig, &key22Updated] (EventArgs const &)
        {
            BOOST_REQUIRE(testConfig.key22 == L"value22new");
            key22Updated.Set();
        });

    // Trigger config updates
    auto updateSuccess = configSettingsConfigStoreSPtr->Update(newConfigSettings);

    BOOST_REQUIRE(key11Updated.WaitOne(TimeSpan::FromSeconds(10)));
    BOOST_REQUIRE(key22Updated.WaitOne(TimeSpan::FromSeconds(10)));

    Trace.WriteInfo(TraceType, "key11 = {0}", testConfig.key11);
    Trace.WriteInfo(TraceType, "key12 = {0}", testConfig.key12);
    Trace.WriteInfo(TraceType, "key21 = {0}", testConfig.key21);
    Trace.WriteInfo(TraceType, "key22 = {0}", testConfig.key22);

    BOOST_REQUIRE(!updateSuccess);
    BOOST_REQUIRE(!key12Updated);
    BOOST_REQUIRE(testConfig.key12 == L"value12new");

    BOOST_REQUIRE(!key21Updated);
    BOOST_REQUIRE(testConfig.key21 == L"value21");

    LEAVE;
}

BOOST_AUTO_TEST_CASE(SmokeConfig)
{
    ENTER;

    // The test expects the file Common.Test.exe.config to exist
    Config config;

    // Check that config has the correct sections
    StringCollection sections;
    config.GetSections(sections);
    BOOST_REQUIRE(7 == sections.size());

    sections.clear();
    config.GetSections(sections, L"Trace");
    BOOST_REQUIRE(3 == sections.size());

    StringCollection keys;
    config.GetKeys(sections[0], keys);
    BOOST_REQUIRE(2 == keys.size());

    int level;
    BOOST_REQUIRE(config.ReadUnencryptedConfig<int>(sections[0], keys[1], level, 666));
    BOOST_REQUIRE(3 == level);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(CheckConfig)
{
    ENTER;

    Config config;
    int level = 0;
    config.ReadUnencryptedConfig<int>(L"SomeSection/SomeItem", L"SomeProperty", level, 0);
    BOOST_REQUIRE(level == 42);

    LEAVE;
}

BOOST_AUTO_TEST_CASE(SmokeConfigRead)
{
    ENTER;

    // The test expects the file Common.Test.exe.config to exist
    Config config;
    TimeSpan messageTimeout;
    wstring tcpDefaultHost;
    int maxHop;

    config.ReadUnencryptedConfig<int>(TransportSection, MaxHopKey, maxHop, 3);
    config.ReadUnencryptedConfig<TimeSpan>(TransportSection, MessageTimeoutKey, messageTimeout, Common::TimeSpan::FromSeconds(22));
    config.ReadUnencryptedConfig<wstring>(TransportSection, TcpTransportDefaultHostKey, tcpDefaultHost, Common::Environment::GetMachineName());

    BOOST_REQUIRE(3 == maxHop);
    BOOST_REQUIRE(L"MySpecialHost" == tcpDefaultHost);
    BOOST_REQUIRE(TimeSpan::FromSeconds(15.3) == messageTimeout);

    LEAVE;
}

BOOST_AUTO_TEST_SUITE_END()
