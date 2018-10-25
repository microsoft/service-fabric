// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/FabricGlobals.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Federation;
using namespace Transport;

HRESULT ConfigLoader::FabricGetConfigStore(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in_opt IFabricConfigStoreUpdateHandler *updateHandler,
    /* [out, retval] */ void ** configStore)
{
    if (configStore == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    ComPointer<IFabricConfigStoreUpdateHandler> updateHandlerCPtr;
    if (updateHandler)
    {
        updateHandlerCPtr.SetAndAddRef(updateHandler);
    }

    ComPointer<ComConfigStore> store = make_com<ComConfigStore>(FabricGlobals::Get().GetConfigStore().Store, updateHandlerCPtr);

    return ComUtility::OnPublicApiReturn(store->QueryInterface(riid, reinterpret_cast<void**>(configStore)));
}

ConfigStoreDescriptionUPtr ConfigLoader::CreateConfigStore(HMODULE dllModule)
{
    ConfigStoreType::Enum storeType;
    wstring storeLocation;

    auto error = FabricEnvironment::GetStoreTypeAndLocation(dllModule, storeType, storeLocation);
    ASSERT_IF(!error.IsSuccess(), "FabricEnvironment::GetStoreTypeAndLocation failed with {0}", error);

    ConfigEventSource::Events.ConfigStoreInitialized(ConfigStoreType::ToString(storeType), storeLocation);

    if (storeType == ConfigStoreType::Cfg)
    {
        return make_unique<ConfigStoreDescription>(
            make_shared<FileConfigStore>(storeLocation),
            *FabricEnvironment::FileConfigStoreEnvironmentVariable,
            storeLocation);
    }

    if (storeType == ConfigStoreType::Package)
    {
        auto store = PackageConfigStore::Create(
            storeLocation, 
            L"Fabric.Config",
            [](Common::ConfigSettings & settings) { ConfigLoader::ProcessFabricConfigSettings(settings); });

        return make_unique<ConfigStoreDescription>(
            store,
            *FabricEnvironment::PackageConfigStoreEnvironmentVariable,
            storeLocation);
    }

    if (storeType == ConfigStoreType::SettingsFile)
    {
        auto store = FileXmlSettingsStore::Create(
            storeLocation,
            [](Common::ConfigSettings & settings) { ConfigLoader::ProcessFabricConfigSettings(settings); });

        return make_unique<ConfigStoreDescription>(
            store,
            *FabricEnvironment::SettingsConfigStoreEnvironmentVariable,
            storeLocation);
    }

    if (storeType == ConfigStoreType::None)
    {
        return make_unique<ConfigStoreDescription>(
            make_shared<ConfigSettingsConfigStore>(move(ConfigSettings())),
            L"",
            L"");
    }

    Assert::CodingError("unknown config store type {0}", static_cast<int>(storeType));
}

void ConfigLoader::ProcessFabricConfigSettings(ConfigSettings & configSettings)
{
    auto error = GenerateNodeIds(configSettings);
    ASSERT_IFNOT(error.IsSuccess(), "NodeId generation failed with error {0}", error.ToHResult());

    error = ReplaceIPAddresses(configSettings);
    ASSERT_IFNOT(error.IsSuccess(), "IPAddress replcement failed with error {0}", error.ToHResult());
}

void ConfigLoader::GetNodeIdGeneratorConfig(ConfigSettings & configSettings, wstring & rolesForWhichToUseV1NodIdGenerator, bool & useV2NodeIdGenerator, wstring & nodeIdGeneratorVersion)
{
    useV2NodeIdGenerator = false;
    nodeIdGeneratorVersion = L"";
    rolesForWhichToUseV1NodIdGenerator = L"";
    auto fabricNodeSection = configSettings.Sections.find(L"Federation");
    if (fabricNodeSection == configSettings.Sections.end())
    {
        return;
    }
    
    auto instanceNameParam = fabricNodeSection->second.Parameters.find(L"NodeNamePrefixesForV1Generator");
    if (instanceNameParam != fabricNodeSection->second.Parameters.end())
    {
        rolesForWhichToUseV1NodIdGenerator = instanceNameParam->second.Value;
    }
    
    instanceNameParam = fabricNodeSection->second.Parameters.find(L"UseV2NodeIdGenerator");
    if (instanceNameParam != fabricNodeSection->second.Parameters.end())
    {
        useV2NodeIdGenerator = StringUtility::AreEqualCaseInsensitive(instanceNameParam->second.Value, L"true");
    }

    instanceNameParam = fabricNodeSection->second.Parameters.find(L"NodeIdGeneratorVersion");
    if (instanceNameParam != fabricNodeSection->second.Parameters.end())
    {
        nodeIdGeneratorVersion = instanceNameParam->second.Value;
    }
}

ErrorCode ConfigLoader::GenerateNodeIds(ConfigSettings & configSettings)
{
    bool useV2NodeIdGenerator = false;
    wstring nodeIdGeneratorVersion = L"";
    wstring rolesForWhichToUseV1NodeIdGeneraor = L"";
    ConfigLoader::GetNodeIdGeneratorConfig(configSettings, rolesForWhichToUseV1NodeIdGeneraor, useV2NodeIdGenerator, nodeIdGeneratorVersion);

    // process FabricNode section and generate node id
    auto fabricNodeSection = configSettings.Sections.find(L"FabricNode");
    if (fabricNodeSection != configSettings.Sections.end())
    {
        auto instanceNameParam = fabricNodeSection->second.Parameters.find(L"InstanceName");
        if (instanceNameParam != fabricNodeSection->second.Parameters.end())
        {
            ConfigParameter nodeId;
            nodeId.Name = L"NodeId";
            NodeId id;
            ErrorCode errorCode = NodeIdGenerator::GenerateFromString(instanceNameParam->second.Value, id, rolesForWhichToUseV1NodeIdGeneraor, useV2NodeIdGenerator, nodeIdGeneratorVersion);
            if (!errorCode.IsSuccess()) { return errorCode; }

            nodeId.Value = id.ToString();
            fabricNodeSection->second.Parameters[nodeId.Name] = nodeId;
        }
        else
        {
            // use machine name 
            ConfigParameter instanceName;
            instanceName.Name = L"InstanceName";
            instanceName.Value = Environment::GetMachineName();

            ConfigParameter nodeId;
            nodeId.Name = L"NodeId";
            NodeId id;
            ErrorCode errorCode = NodeIdGenerator::GenerateFromString(instanceNameParam->second.Value, id, rolesForWhichToUseV1NodeIdGeneraor, useV2NodeIdGenerator, nodeIdGeneratorVersion);
            if (!errorCode.IsSuccess()) { return errorCode; }
            nodeId.Value = id.ToString();

            fabricNodeSection->second.Parameters[instanceName.Name] = instanceName;
            fabricNodeSection->second.Parameters[nodeId.Name] = nodeId;
        }
    }

    // generate node id for the votes section
    auto votesSection = configSettings.Sections.find(L"Votes");
    if (votesSection != configSettings.Sections.end())
    {
        ConfigSection newVotesSection;
        newVotesSection.Name = L"Votes";

        for (auto voteIter = votesSection->second.Parameters.begin(); 
            voteIter != votesSection->second.Parameters.end(); 
            ++voteIter)
        {

            ConfigParameter vote;
            if (StringUtility::Contains<wstring>(voteIter->second.Name, Federation::Constants::SqlServerVoteType))
            {
                vote.Name = voteIter->second.Name;
            }
            else
            {
                NodeId id;
                ErrorCode errorCode = NodeIdGenerator::GenerateFromString(voteIter->second.Name, id, rolesForWhichToUseV1NodeIdGeneraor, useV2NodeIdGenerator, nodeIdGeneratorVersion);
                if (!errorCode.IsSuccess()) { return errorCode;}

                vote.Name = id.ToString();
            }
            vote.Value = voteIter->second.Value;
            newVotesSection.Parameters[vote.Name] = vote;
        }

        configSettings.Sections[newVotesSection.Name] = newVotesSection;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ConfigLoader::ReplaceIPAddresses(ConfigSettings & configSettings)
{
    wstring ipAddressOrFqdn = GetIPAddressOrFQDN(configSettings);
    wstring ipv4Address;
    wstring ipv6Address;

    for(auto sectionIter = configSettings.Sections.begin();
        sectionIter != configSettings.Sections.end();
        ++sectionIter)
    {
        for(auto paramIter = sectionIter->second.Parameters.begin();
            paramIter != sectionIter->second.Parameters.end();
            ++paramIter)
        {
            wstring name = paramIter->second.Name;
            wstring value = paramIter->second.Value;

            if(name == L"IPAddressOrFQDN")
            {
                continue;
            }

            else if ((name.find(L"Address") != wstring::npos) && (value.find(L":") == wstring::npos))
            {
                if (name == L"RuntimeServiceAddress" || name == L"RuntimeSslServiceAddress")
                {
                    paramIter->second.Value = wformatString("localhost:{0}", value);
                }
                // this is a non-qualified address parameter, convert the value from port to fully qualified address
                else if (ipAddressOrFqdn.empty())
                {
                    wstring outValue;
                    auto error = ReplaceIPAddress(ipv4Address, ipv6Address, value, outValue);
                    if (!error.IsSuccess()) { return error; }

                    paramIter->second.Value = outValue;
                }
                else
                {
                    wstring outValue;
                    ReplaceSpecifiedAddressOrFQDN(ipAddressOrFqdn, value, outValue);                    
                    paramIter->second.Value = outValue;
                }
            }

        } // parameters
    } //sections

    return ErrorCode(ErrorCodeValue::Success);
}

void ConfigLoader::ReplaceSpecifiedAddressOrFQDN(wstring const & ipAddressOrFQDN, wstring const & input, __out wstring & output)
{
    if (input.find(L"[") == wstring::npos)
    {
        output = ipAddressOrFQDN + L":" + input;
    }
    else
    {
        output = input.substr(1, input.length() - 2);
        output =  ipAddressOrFQDN + L":" + output;
    }
}

ErrorCode ConfigLoader::ReplaceIPAddress(
    __inout std::wstring & ipv4Address,
    __inout std::wstring & ipv6Address,
    std::wstring const & input, 
    __out std::wstring & output)
{
    output = input;

    if (input.find(L"[") == wstring::npos)
    {
        if (ipv4Address.empty())
        {
            auto error = GetIPv4Address(ipv4Address);
            if (!error.IsSuccess()) { return error; }
        }

        output = ipv4Address + L":" + input;
    }
    else
    {
        if (ipv6Address.empty())
        {
            auto error = GetIPv6Address(ipv6Address);
            if (!error.IsSuccess()) { return error; }
        }

        output = input.substr(1, input.length() - 2);
        output = L"[" + ipv6Address + L"]:" + output;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ConfigLoader::GetIPv4Address(__out wstring & ipv4Address)
{
    vector<Endpoint> ipv4Endpoints;
    auto error = Transport::TcpTransportUtility::GetLocalAddresses(ipv4Endpoints, ResolveOptions::IPv4);
    if (!error.IsSuccess())
    {
        return error;
    }

    for(auto iter = ipv4Endpoints.begin(); iter != ipv4Endpoints.end(); ++iter)
    {
        if (iter->IsLoopback())
        {
            continue;
        }

        ipv4Address = iter->ToString();
        break;
    }

    if (ipv4Address.empty())
    {
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ConfigLoader::GetIPv6Address(__out wstring & ipv6Address)
{
    vector<Endpoint> ipv6Endpoints;
    auto error = Transport::TcpTransportUtility::GetLocalAddresses(ipv6Endpoints, ResolveOptions::IPv6);
    if (!error.IsSuccess())
    {
        return error;
    }

    for(auto iter = ipv6Endpoints.begin(); iter != ipv6Endpoints.end(); ++iter)
    {
        if (iter->IsLoopback())
        {
            continue;
        }

        ipv6Address = iter->ToString();
        break;
    }

    if (ipv6Address.empty())
    {
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

wstring ConfigLoader::GetIPAddressOrFQDN(ConfigSettings & configSettings)
{
    wstring ipAddrOrFqdn = L"";

    // check if the FabricNode section has IPAddressOrFQDN parameter, if so use that
    auto fabricNodeSection = configSettings.Sections.find(L"FabricNode");
    if (fabricNodeSection != configSettings.Sections.end())
    {
        auto ipAddrOrFqdnParam = fabricNodeSection->second.Parameters.find(L"IPAddressOrFQDN");
        if (ipAddrOrFqdnParam != fabricNodeSection->second.Parameters.end())
        {
            ipAddrOrFqdn = ipAddrOrFqdnParam->second.Value;
        }
    }

    return ipAddrOrFqdn;
}