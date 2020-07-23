// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Federation;
using namespace Transport;

ConfigLoader * ConfigLoader::singleton_;
INIT_ONCE ConfigLoader::initOnceConfigLoader_;

ConfigLoader::ConfigLoader()
    : store_(),
    lock_(),
    storeEnvironmentVariableName_(),
    storeEnvironmentVariableValue_()
{
}

ConfigLoader::~ConfigLoader()
{
}

HRESULT ConfigLoader::FabricGetConfigStore(
    HMODULE dllModule,
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in_opt IFabricConfigStoreUpdateHandler *updateHandler,
    /* [out, retval] */ void ** configStore)
{
    if (configStore == NULL) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    ConfigLoader & loader = ConfigLoader::GetConfigLoader();
    ComPointer<IFabricConfigStoreUpdateHandler> updateHandlerCPtr;
    if (updateHandler)
    {
        updateHandlerCPtr.SetAndAddRef(updateHandler);
    }
    ComPointer<ComConfigStore> store = make_com<ComConfigStore>(loader.FabricGetConfigStore(dllModule), updateHandlerCPtr);
    return ComUtility::OnPublicApiReturn(store->QueryInterface(riid, reinterpret_cast<void**>(configStore)));
}

HRESULT ConfigLoader::FabricGetConfigStoreEnvironmentVariable( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableValue)
{
    if ((envVariableName == NULL) || (envVariableValue == NULL)) 
    { 
        return ComUtility::OnPublicApiReturn(E_POINTER); 
    }

    ConfigLoader & loader = ConfigLoader::GetConfigLoader();

    wstring envVariableNameStr;
    wstring envVariableValueStr;
    loader.FabricGetConfigStoreEnvironmentVariable(envVariableNameStr, envVariableValueStr);

    ComPointer<IFabricStringResult> envVariableNameResult = make_com<ComStringResult, IFabricStringResult>(envVariableNameStr);
    ComPointer<IFabricStringResult> envVariableValueResult = make_com<ComStringResult, IFabricStringResult>(envVariableValueStr);

    *envVariableName = envVariableNameResult.DetachNoRelease();
    *envVariableValue = envVariableValueResult.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}

ConfigStoreSPtr ConfigLoader::FabricGetConfigStore(HMODULE dllModule)
{
    {
        AcquireExclusiveLock lock(lock_);
        if (this->store_ == nullptr)
        {
            auto error = CreateConfigStore_CallerHoldsLock(dllModule);
            if (!error.IsSuccess())
            {
                Assert::CodingError("CreateConfigStore failed with error {0}", error);
            }
        }
    }

    return this->store_;
}

void ConfigLoader::FabricGetConfigStoreEnvironmentVariable(__out std::wstring & envVariableName, __out std::wstring & envVariableValue)
{
    {
        AcquireExclusiveLock lock(lock_);
        envVariableName = storeEnvironmentVariableName_;
        envVariableValue = storeEnvironmentVariableValue_;
    }
}

ErrorCode ConfigLoader::CreateConfigStore_CallerHoldsLock(HMODULE dllModule)
{
    ConfigStoreType::Enum storeType;
    wstring storeLocation;

    auto error = FabricEnvironment::GetStoreTypeAndLocation(dllModule, storeType, storeLocation);
    if (!error.IsSuccess()) { return error; }

    ConfigEventSource::Events.ConfigStoreInitialized(ConfigStoreType::ToString(storeType), storeLocation);

    if (storeType == ConfigStoreType::Cfg)
    {
        this->store_ = make_shared<FileConfigStore>(storeLocation);
        this->storeEnvironmentVariableName_ = (const wstring) FabricEnvironment::FileConfigStoreEnvironmentVariable;
        this->storeEnvironmentVariableValue_ = storeLocation;

        return ErrorCode(ErrorCodeValue::Success);
    }

    if (storeType == ConfigStoreType::Package)
    {
        this->store_ = PackageConfigStore::Create(
            storeLocation, 
            L"Fabric.Config",
            [](Common::ConfigSettings & settings) { ConfigLoader::ProcessFabricConfigSettings(settings); });

        this->storeEnvironmentVariableName_ = *FabricEnvironment::PackageConfigStoreEnvironmentVariable;
        this->storeEnvironmentVariableValue_ = storeLocation;

        return ErrorCode(ErrorCodeValue::Success);
    }

    if (storeType == ConfigStoreType::SettingsFile)
    {
        this->store_ = FileXmlSettingsStore::Create(
            storeLocation,
            [](Common::ConfigSettings & settings) { ConfigLoader::ProcessFabricConfigSettings(settings); });

        this->storeEnvironmentVariableName_ = *FabricEnvironment::SettingsConfigStoreEnvironmentVariable;
        this->storeEnvironmentVariableValue_ = storeLocation;

        return ErrorCode(ErrorCodeValue::Success);
    }

    if (storeType == ConfigStoreType::None)
    {
        this->store_ = make_shared<ConfigSettingsConfigStore>(move(ConfigSettings()));
        this->storeEnvironmentVariableName_ = L"";
        this->storeEnvironmentVariableValue_ = L"";

        return ErrorCode(ErrorCodeValue::Success);
    }

    return ErrorCode(ErrorCodeValue::NotImplemented);
}

void ConfigLoader::MakeAbsolute(wstring & storeLocation)
{
    if (Path::IsPathRooted(storeLocation)) { return; }

    wstring path(Directory::GetCurrentDirectory());
    Path::CombineInPlace(path, storeLocation);
    if (File::Exists(path))
    {
        storeLocation = path;
        return;
    }

    path.resize(0);
    Environment::GetExecutablePath(path);
    Path::CombineInPlace(path, storeLocation);
    if(File::Exists(path))
    {
        storeLocation = path;
        return;
    }
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

ConfigLoader & ConfigLoader::GetConfigLoader()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = FALSE;

    bStatus = ::InitOnceExecuteOnce(
        &ConfigLoader::initOnceConfigLoader_,
        ConfigLoader::InitConfigLoaderFunction,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize ConfigLoader singleton");
    return *(ConfigLoader::singleton_);
}

BOOL CALLBACK ConfigLoader::InitConfigLoaderFunction(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = new ConfigLoader();
    return TRUE;
}
