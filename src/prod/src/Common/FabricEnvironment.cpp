// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

// Environment variables
GlobalWString FabricEnvironment::FabricDataRootEnvironmentVariable = make_global<wstring>(L"FabricDataRoot");
GlobalWString FabricEnvironment::FabricRootEnvironmentVariable = make_global<wstring>(L"FabricRoot");
GlobalWString FabricEnvironment::FabricBinRootEnvironmentVariable = make_global<wstring>(L"FabricBinRoot");
GlobalWString FabricEnvironment::FabricLogRootEnvironmentVariable = make_global<wstring>(L"FabricLogRoot");
GlobalWString FabricEnvironment::EnableCircularTraceSessionEnvironmentVariable = make_global<wstring>(L"EnableCircularTraceSession");
GlobalWString FabricEnvironment::FabricCodePathEnvironmentVariable = make_global<wstring>(L"FabricCodePath");
GlobalWString FabricEnvironment::HostedServiceNameEnvironmentVariable = make_global<wstring>(L"HostedServiceName");
GlobalWString FabricEnvironment::FileConfigStoreEnvironmentVariable = make_global<wstring>(L"FabricConfigFileName");
GlobalWString FabricEnvironment::PackageConfigStoreEnvironmentVariable = make_global<wstring>(L"FabricPackageFileName");
GlobalWString FabricEnvironment::SettingsConfigStoreEnvironmentVariable = make_global<wstring>(L"FabricSettingsFileName");
GlobalWString FabricEnvironment::RemoveNodeConfigurationEnvironmentVariable = make_global<wstring>(L"RemoveNodeConfiguration");
GlobalWString FabricEnvironment::WindowsFabricRemoveNodeConfigurationRegValue = make_global<wstring>(L"RemoveNodeConfiguration");
GlobalWString FabricEnvironment::FabricDnsServerIPAddressEnvironmentVariable = make_global<wstring>(L"FabricDnsServerIPAddress");
GlobalWString FabricEnvironment::FabricIsolatedNetworkInterfaceNameEnvironmentVariable = make_global<wstring>(L"FabricIsolatedNetworkInterfaceName");
GlobalWString FabricEnvironment::FabricHostServicePathEnvironmentVariable = make_global<wstring>(L"FabricHostServicePath");
GlobalWString FabricEnvironment::UpdaterServicePathEnvironmentVariable = make_global<wstring>(L"UpdaterServicePath");

#if defined(PLATFORM_UNIX)
LinuxPackageManagerType::Enum FabricEnvironment::packageManagerType_ = LinuxPackageManagerType::Enum::Unknown;
#endif

// Determining what configuration store to use uses the following logic

// If FabricConfigFileName environment variable is set, then use Cfg store 
// If FabricPackageFileName environment variable is set, the use Package store
// If either of the above variables are not set, then
//     first try to locate CFG file from the executable path and module path, if found use Cfg store
//     second try to locate Package file from the module path, if found use Package store

ErrorCode FabricEnvironment::GetStoreTypeAndLocation(
    HMODULE dllModule,
    __out ConfigStoreType::Enum & storeType,
    __out wstring & storeLocation)
{
    storeType = ConfigStoreType::Unknown;
    storeLocation.resize(0);

    if (Environment::GetEnvironmentVariableW(FabricEnvironment::FileConfigStoreEnvironmentVariable, /* out */ storeLocation, Common::NOTHROW()))
    {
        storeType = ConfigStoreType::Cfg;
        Path::MakeAbsolute(storeLocation);
        return (File::Exists(storeLocation) ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::NotFound));
    }

    if (Environment::GetEnvironmentVariableW(FabricEnvironment::PackageConfigStoreEnvironmentVariable, /* out */ storeLocation, Common::NOTHROW()))
    {
        storeType = ConfigStoreType::Package;
        Path::MakeAbsolute(storeLocation);
        return (File::Exists(storeLocation) ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::NotFound));
    }

    if (Environment::GetEnvironmentVariableW(FabricEnvironment::SettingsConfigStoreEnvironmentVariable, /* out */ storeLocation, Common::NOTHROW()))
    {
        storeType = ConfigStoreType::SettingsFile;
        Path::MakeAbsolute(storeLocation);
        return (File::Exists(storeLocation) ? ErrorCode(ErrorCodeValue::Success) : ErrorCode(ErrorCodeValue::NotFound));
    }

    wstring executableFileName;
    wstring executablePath;

    // option 1: PACKAGE: check if Fabric.Package.Current.xml is present in [exepath]\\..
    Environment::GetExecutablePath(executablePath);

    storeLocation.resize(0);
    storeLocation = executablePath;
    Path::CombineInPlace(storeLocation, L"..");
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultPackageConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::Package;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 2: PACKAGE: check if Fabric.Package.Current.xml is present in [dllModulePath]\\..
    wstring dllModulePath;
    wchar_t moduleFileName[MAX_PATH];
    if (::GetModuleFileName(dllModule, moduleFileName, MAX_PATH) == 0)
    {
        return ErrorCode::FromWin32Error(GetLastError());
    }
    dllModulePath = Path::GetDirectoryName(moduleFileName);

    storeLocation.resize(0);
    storeLocation = dllModulePath;
    Path::CombineInPlace(storeLocation, L"..");
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultPackageConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::Package;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 3: CFG: check if exe.cfg is present
    Environment::GetExecutableFileName(executableFileName);

    storeLocation.resize(0);
    storeLocation = executableFileName;
    storeLocation.append(L".cfg");
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::Cfg;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 4: CFG: check if FabricConfig.cfg is present next to the executable
    storeLocation.resize(0);
    storeLocation = executablePath;
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultFileConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::Cfg;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 5: CFG: check if FabricConfig.cfg is present next to Fabric DLLs
    storeLocation.resize(0);
    storeLocation = dllModulePath;
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultFileConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::Cfg;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 6: Settings: check if FabricHostSettings.xml is present in the dataroot
    wstring fabricDataRoot;
    auto error = GetFabricDataRoot(fabricDataRoot);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FabricDataRootNotFound))
    {
        return error;
    }

    if (error.IsSuccess())
    {
        storeLocation.resize(0);
        storeLocation = fabricDataRoot;
        Path::CombineInPlace(storeLocation, FabricConstants::DefaultSettingsConfigStoreLocation);
        if (File::Exists(storeLocation))
        {
            storeType = ConfigStoreType::SettingsFile;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }

    // option 7: Settings: check if FabricHostSettings.xml is present in the exePath
    storeLocation.resize(0);
    storeLocation = executablePath;
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultSettingsConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::SettingsFile;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 8: Settings: check if FabricClientSettings.xml is present in the current folder
    storeLocation.resize(0);
    storeLocation = Directory::GetCurrentDirectoryW();
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultClientSettingsConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::SettingsFile;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // option 9: Settings: check if FabricClientSettings.xml is present next to Fabric DLLs
    storeLocation.resize(0);
    storeLocation = dllModulePath;
    Path::CombineInPlace(storeLocation, FabricConstants::DefaultClientSettingsConfigStoreLocation);
    if (File::Exists(storeLocation))
    {
        storeType = ConfigStoreType::SettingsFile;
        return ErrorCode(ErrorCodeValue::Success);
    }

    // all options to locate configuration store information failed, use None (empty) store
    storeType = ConfigStoreType::None;
    storeLocation.resize(0);
    storeLocation = L"";

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode FabricEnvironment::GetFabricVersion(__out wstring & fabricVersion)
{
#if defined(PLATFORM_UNIX)
    wstring versionFile = Path::Combine(Environment::GetExecutablePath(), L"ClusterVersion");
    File fileReader;
    auto error = fileReader.TryOpen(versionFile, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
    if (!error.IsSuccess())
    {
        return error;
    }

    int64 fileSize = fileReader.size();
    size_t size = static_cast<size_t>(fileSize);

    string text;
    text.resize(size);
    fileReader.Read(&text[0], static_cast<int>(size));
    fileReader.Close();

    StringWriter(fabricVersion).Write(text);
#else
    fabricVersion = FileVersion::GetCurrentExeVersion();
#endif
    return ErrorCode(ErrorCodeValue::Success);
}

#if defined(PLATFORM_UNIX)
ErrorCode FabricEnvironment::GetLinuxPackageManagerType(__out LinuxPackageManagerType::Enum & packageManagerType)
{
    if (packageManagerType_ == LinuxPackageManagerType::Enum::Unknown)
    {
        if (system("/usr/bin/dpkg --search /usr/bin/dpkg >/dev/null 2>&1") == 0)
        {
            packageManagerType_ = LinuxPackageManagerType::Enum::Deb;
        }
        else if (system("/usr/bin/rpm -q -f /usr/bin/rpm >/dev/null 2>&1") == 0)
        {
            packageManagerType_ = LinuxPackageManagerType::Enum::Rpm;
        }
        else
        {
            packageManagerType_ = LinuxPackageManagerType::Enum::Unknown;
        }
    }

    packageManagerType = packageManagerType_;
       
    return ErrorCode(ErrorCodeValue::Success);
}
#endif

ErrorCode FabricEnvironment::GetFabricRoot(__out wstring & fabricRoot)
{
    return GetFabricRoot(NULL, fabricRoot);
}

ErrorCode FabricEnvironment::GetFabricRoot(LPCWSTR machineName, __out wstring & fabricRoot)
{
    return GetRegistryKeyHelper(FabricConstants::FabricRootRegKeyName, FabricRootEnvironmentVariable, ErrorCodeValue::FabricDataRootNotFound, machineName, fabricRoot);
}

ErrorCode FabricEnvironment::GetFabricBinRoot(__out wstring & fabricBinRoot)
{
    return GetFabricBinRoot(NULL, fabricBinRoot);

}

ErrorCode FabricEnvironment::GetFabricBinRoot(LPCWSTR machineName, __out wstring & fabricBinRoot)
{
    return GetRegistryKeyHelper(FabricConstants::FabricBinRootRegKeyName, FabricBinRootEnvironmentVariable, ErrorCodeValue::FabricBinRootNotFound, machineName, fabricBinRoot);
}

ErrorCode FabricEnvironment::GetFabricCodePath(__out wstring & fabricCodePath)
{
    return GetFabricCodePath(nullptr, NULL, fabricCodePath);
}

ErrorCode FabricEnvironment::GetFabricCodePath(LPCWSTR machineName, __out wstring & fabricCodePath)
{
    return GetFabricCodePath(nullptr, machineName, fabricCodePath);
}

ErrorCode FabricEnvironment::GetFabricCodePath(HMODULE dllModule, __out wstring & fabricCodePath)
{
    return GetFabricCodePath(dllModule, NULL, fabricCodePath);
}

ErrorCode FabricEnvironment::GetFabricCodePath(HMODULE dllModule, LPCWSTR machineName, __out wstring & fabricCodePath)
{
    if (machineName == NULL || machineName[0] == 0)
    {
        ErrorCode result = ErrorCode(ErrorCodeValue::FabricCodePathNotFound);
        bool fabricCodePathEnvFound = Environment::GetEnvironmentVariable(FabricCodePathEnvironmentVariable, fabricCodePath, Common::NOTHROW());
        if (fabricCodePathEnvFound)
        {
            return Environment::Expand(fabricCodePath, fabricCodePath);
        }

#if defined(PLATFORM_UNIX)
        return GetEtcConfigValue(FabricConstants::FabricCodePathRegKeyName, ErrorCodeValue::FabricCodePathNotFound, fabricCodePath);
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        if (regKey.IsValid)
        {
            result = GetRegistryKeyHelper(regKey, FabricConstants::FabricCodePathRegKeyName, ErrorCodeValue::FabricCodePathNotFound, fabricCodePath);
            if (!result.IsError(ErrorCodeValue::FabricCodePathNotFound))
            {
                return result;
            }
        }
#endif

        if (dllModule)
        {
            // incase of xcopy deployment return the path of the DLL (FabricCommon)
            wchar_t moduleFileName[MAX_PATH];
            if (::GetModuleFileName(dllModule, moduleFileName, MAX_PATH) == 0)
            {
                return ErrorCode::FromWin32Error(GetLastError());
            }

            fabricCodePath = Path::GetDirectoryName(moduleFileName);
            result = ErrorCode(ErrorCodeValue::Success);
        }

        return result;
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("Getting registry configuration from remote not supported in linux");
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, machineName, true, true);
        return GetRegistryKeyHelper(regKey, FabricConstants::FabricCodePathRegKeyName, ErrorCodeValue::FabricCodePathNotFound, fabricCodePath);
#endif  
    }
}

ErrorCode FabricEnvironment::GetFabricDataRoot(__out wstring & fabricDataRoot)
{
    return GetFabricDataRoot(NULL, fabricDataRoot);
}

ErrorCode FabricEnvironment::GetFabricDataRoot(LPCWSTR machineName, __out wstring & fabricDataRoot)
{
    return GetRegistryKeyHelper(FabricConstants::FabricDataRootRegKeyName, FabricDataRootEnvironmentVariable, ErrorCodeValue::FabricDataRootNotFound, machineName, fabricDataRoot);
}

ErrorCode FabricEnvironment::GetFabricLogRoot(__out wstring & fabricLogRoot)
{
    return GetFabricLogRoot(NULL, fabricLogRoot);
}

ErrorCode FabricEnvironment::GetFabricLogRoot(LPCWSTR machineName, __out wstring & fabricLogRoot)
{
    return GetRegistryKeyHelper(FabricConstants::FabricLogRootRegKeyName, FabricLogRootEnvironmentVariable, ErrorCodeValue::FabricLogRootNotFound, machineName, fabricLogRoot);
}

ErrorCode FabricEnvironment::GetEnableCircularTraceSession(__out BOOLEAN & enableCircularTraceSession)
{
    return GetEnableCircularTraceSession(NULL, enableCircularTraceSession);
}

ErrorCode FabricEnvironment::GetEnableCircularTraceSession(LPCWSTR machineName, __out BOOLEAN & enableCircularTraceSession)
{
    // Default to false and ignore if not present
    DWORD val = 0;
    auto err = GetRegistryKeyHelper(FabricConstants::EnableCircularTraceSessionRegKeyName, EnableCircularTraceSessionEnvironmentVariable, ErrorCodeValue::Success, machineName, val);
    enableCircularTraceSession = val != 0;
    return err;
}

ErrorCode FabricEnvironment::GetRemoveNodeConfigurationValue(__out wstring & removeNodeConfigurationValue)
{
    return GetRegistryKeyHelper(WindowsFabricRemoveNodeConfigurationRegValue, RemoveNodeConfigurationEnvironmentVariable, ErrorCodeValue::FabricRemoveConfigurationValueNotFound, NULL, removeNodeConfigurationValue);
}

ErrorCode FabricEnvironment::GetFabricDnsServerIPAddress(__out wstring & dnsServerIPAddress)
{
    return GetFabricDnsServerIPAddress(NULL, dnsServerIPAddress);
}

ErrorCode FabricEnvironment::GetFabricDnsServerIPAddress(LPCWSTR machineName, __out wstring & dnsServerIPAddress)
{
    return GetRegistryKeyHelper(FabricConstants::FabricDnsServerIPAddressRegKeyName, FabricDnsServerIPAddressEnvironmentVariable, ErrorCodeValue::DnsServerIPAddressNotFound, machineName, dnsServerIPAddress);
}

ErrorCode FabricEnvironment::GetFabricIsolatedNetworkInterfaceName(__out std::wstring & isolatedNetworkInterfaceName)
{
    return GetFabricIsolatedNetworkInterfaceName(NULL, isolatedNetworkInterfaceName);
}

ErrorCode FabricEnvironment::GetFabricIsolatedNetworkInterfaceName(LPCWSTR machineName, __out std::wstring & isolatedNetworkInterfaceName)
{
    return GetRegistryKeyHelper(FabricConstants::FabricIsolatedNetworkInterfaceRegKeyName, FabricIsolatedNetworkInterfaceNameEnvironmentVariable, 
        ErrorCodeValue::IsolatedNetworkInterfaceNameNotFound, machineName, isolatedNetworkInterfaceName);
}

ErrorCode FabricEnvironment::GetFabricHostServicePath(__out wstring & fabricHostServicePath)
{
    return GetFabricHostServicePath(NULL, fabricHostServicePath);
}

ErrorCode FabricEnvironment::GetFabricHostServicePath(LPCWSTR machineName, __out wstring & fabricHostServicePath)
{
    return GetRegistryKeyHelper(FabricConstants::FabricHostServicePathRegKeyName, FabricHostServicePathEnvironmentVariable, ErrorCodeValue::FabricHostServicePathNotFound, machineName, fabricHostServicePath);
}

ErrorCode FabricEnvironment::GetUpdaterServicePath(__out wstring & updaterServicePath)
{
    return GetUpdaterServicePath(NULL, updaterServicePath);
}

ErrorCode FabricEnvironment::GetUpdaterServicePath(LPCWSTR machineName, __out wstring & updaterServicePath)
{
    return GetRegistryKeyHelper(FabricConstants::UpdaterServicePathRegKeyName, UpdaterServicePathEnvironmentVariable, ErrorCodeValue::UpdaterServicePathNotFound, machineName, updaterServicePath);
}

ErrorCode FabricEnvironment::GetRegistryKeyHelper(const wstring & name, const wstring & environmentVariable, ErrorCode valueNotFound, LPCWSTR machineName, __out wstring & value)
{
    if (machineName == NULL || machineName[0] == 0)
    {

        bool envVarFound = Environment::GetEnvironmentVariable(environmentVariable, value, Common::NOTHROW());
        if (envVarFound)
        {
            return Environment::Expand(value, value);
        }

#if defined(PLATFORM_UNIX)
        return GetEtcConfigValue(name, valueNotFound, value);
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        return GetRegistryKeyHelper(regKey, name, valueNotFound, value);
#endif
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("Getting registry configuration from remote not supported in linux");
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, machineName, true, true);
        return GetRegistryKeyHelper(regKey, name, valueNotFound, value);
#endif      
    }
}

ErrorCode FabricEnvironment::GetRegistryKeyHelper(const wstring & name, const wstring & environmentVariable, ErrorCode valueNotFound, LPCWSTR machineName, __out DWORD & value)
{
    if (machineName == NULL || machineName[0] == 0)
    {

        wstring envValue;
        bool envVarFound = Environment::GetEnvironmentVariable(environmentVariable, envValue, Common::NOTHROW());
        if (envVarFound)
        {
            value = envValue.compare(L"0") != 0;
            return ErrorCode::Success();
        }

#if defined(PLATFORM_UNIX)
        return GetEtcConfigValue(name, valueNotFound, value);
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        return GetRegistryKeyHelper(regKey, name, valueNotFound, value);
#endif
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("Getting registry configuration from remote not supported in linux");
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, machineName, true, true);
        return GetRegistryKeyHelper(regKey, name, valueNotFound, value);
#endif      
    }
}

ErrorCode FabricEnvironment::SetEtcConfigValue(const wstring & name, const wstring & value)
{
    wstring pathToConfigSetting = Path::Combine(FabricConstants::FabricEtcConfigPath, name);
    try
    {
        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(pathToConfigSetting);
        if (!error.IsSuccess())
        {
            return ErrorCodeValue::OperationFailed;
        }

        std::string result;
        StringUtility::UnicodeToAnsi(value, result);
        fileWriter << result;
    }
    catch (std::exception const&)
    {
        return ErrorCodeValue::OperationFailed;
    }

    return ErrorCodeValue::Success;
}

ErrorCode FabricEnvironment::SetEtcConfigValue(const wstring & name, DWORD value)
{
    return SetEtcConfigValue(name, std::to_wstring((unsigned long long)value));
}

ErrorCode FabricEnvironment::GetEtcConfigValue(const wstring & name, ErrorCode valueNotFound, __out wstring & value)
{
    wstring pathToConfigSetting = Path::Combine(FabricConstants::FabricEtcConfigPath, name);
    if (!File::Exists(pathToConfigSetting))
    {
        return valueNotFound;
    }

    try
    {
        File fileReader;
        auto error = fileReader.TryOpen(pathToConfigSetting, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            return valueNotFound;
        }

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        StringWriter(value).Write(text);
    }
    catch (std::exception const&)
    {
        return valueNotFound;
    }

    return Environment::Expand(value, value);
}

ErrorCode FabricEnvironment::GetEtcConfigValue(const wstring & name, ErrorCode valueNotFound, __out DWORD & value)
{
    wstring stringValue;
    auto err = GetEtcConfigValue(name, valueNotFound, stringValue);
    if (!err.IsSuccess())
    {
        return err;
    }

    value = std::wcstoul(stringValue.c_str(), NULL, 0);

    return ErrorCode::Success();
}

ErrorCode FabricEnvironment::GetRegistryKeyHelper(RegistryKey & regKey, const wstring & name, ErrorCode valueNotFound, __out wstring & value)
{
    if (!regKey.IsValid)
    {
        return valueNotFound;
    }

    if (!regKey.GetValue(name, value))
    {
        return valueNotFound;
    }

    return Environment::Expand(value, value);
}

ErrorCode FabricEnvironment::GetRegistryKeyHelper(RegistryKey & regKey, const wstring & name, ErrorCode valueNotFound, __out DWORD & value)
{
    if (!regKey.IsValid)
    {
        return valueNotFound;
    }

    if (!regKey.GetValue(name, value))
    {
        return valueNotFound;
    }

    return ErrorCodeValue::Success;
}

ErrorCode FabricEnvironment::SetFabricRoot(const wstring & fabricRoot)
{
    return SetFabricRoot(fabricRoot, NULL);
}

ErrorCode FabricEnvironment::SetFabricRoot(const wstring & fabricRoot, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::FabricRootRegKeyName, fabricRoot, machineName);
}

ErrorCode FabricEnvironment::SetFabricBinRoot(const wstring & fabricBinRoot)
{
    return SetFabricBinRoot(fabricBinRoot, NULL);
}

ErrorCode FabricEnvironment::SetFabricBinRoot(const wstring & fabricBinRoot, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::FabricBinRootRegKeyName, fabricBinRoot, machineName);
}

ErrorCode FabricEnvironment::SetFabricCodePath(const wstring & fabricCodePath)
{
    return SetFabricCodePath(fabricCodePath, NULL);
}

ErrorCode FabricEnvironment::SetFabricCodePath(const wstring & fabricCodePath, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::FabricCodePathRegKeyName, fabricCodePath, machineName);
}

ErrorCode FabricEnvironment::SetFabricDataRoot(const wstring & fabricDataRoot)
{
    return SetFabricDataRoot(fabricDataRoot, NULL);
}

ErrorCode FabricEnvironment::SetFabricDataRoot(const wstring & fabricDataRoot, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::FabricDataRootRegKeyName, fabricDataRoot, machineName);
}

ErrorCode FabricEnvironment::SetFabricLogRoot(const wstring & fabricLogRoot)
{
    return SetFabricLogRoot(fabricLogRoot, NULL);
}

ErrorCode FabricEnvironment::SetFabricLogRoot(const wstring & fabricLogRoot, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::FabricLogRootRegKeyName, fabricLogRoot, machineName);
}

ErrorCode FabricEnvironment::SetEnableCircularTraceSession(BOOLEAN enableCircularTraceSession, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::EnableCircularTraceSessionRegKeyName, enableCircularTraceSession, machineName);
}

ErrorCode FabricEnvironment::SetEnableUnsupportedPreviewFeatures(BOOLEAN enableUnsupportedPreviewFeatures, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::EnableUnsupportedPreviewFeaturesRegKeyName, enableUnsupportedPreviewFeatures, machineName);
}

ErrorCode FabricEnvironment::GetEnableUnsupportedPreviewFeatures(bool & enableUnsupportedPreviewFeatures)
{
    // Default to false and ignore if not present
    DWORD val = 0;
    auto err = GetRegistryKeyHelper(FabricConstants::EnableUnsupportedPreviewFeaturesRegKeyName, FabricConstants::EnableUnsupportedPreviewFeaturesRegKeyName, ErrorCodeValue::Success, NULL, val);
    enableUnsupportedPreviewFeatures = val != 0;
    return err;
}

ErrorCode FabricEnvironment::SetIsSFVolumeDiskServiceEnabled(BOOLEAN isSFVolumeDiskServiceEnabled, LPCWSTR machineName)
{
    return SetRegistryKeyHelper(FabricConstants::IsSFVolumeDiskServiceEnabledRegKeyName, isSFVolumeDiskServiceEnabled, machineName);
}

ErrorCode FabricEnvironment::GetIsSFVolumeDiskServiceEnabled(bool & isSFVolumeDiskServiceEnabled)
{
    // Default to false and ignore if not present
    DWORD val = 0;
    auto err = GetRegistryKeyHelper(FabricConstants::IsSFVolumeDiskServiceEnabledRegKeyName, FabricConstants::IsSFVolumeDiskServiceEnabledRegKeyName, ErrorCodeValue::Success, NULL, val);
    isSFVolumeDiskServiceEnabled = val != 0;
    return err;
}

#if defined(PLATFORM_UNIX)
ErrorCode FabricEnvironment::SetSfInstalledMoby(LPCWSTR fileContents)
{
    return SetRegistryKeyHelper(FabricConstants::SfInstalledMobyRegKeyName, fileContents, NULL);
}
#endif

ErrorCode FabricEnvironment::SetRegistryKeyHelper(const wstring & name, const wstring & value, LPCWSTR machineName)
{
    if (machineName == NULL || machineName[0] == 0)
    {
#if defined(PLATFORM_UNIX)
        return SetEtcConfigValue(name, value);
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, false);
        return SetRegistryKeyHelper(regKey, name, value);
#endif 
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("Getting registry configuration from remote not supported in linux");
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, machineName, false);
        return SetRegistryKeyHelper(regKey, name, value);
#endif 
    }
}

ErrorCode FabricEnvironment::SetRegistryKeyHelper(const wstring & name, DWORD value, LPCWSTR machineName)
{
    if (machineName == NULL || machineName[0] == 0)
    {
#if defined(PLATFORM_UNIX)
        return SetEtcConfigValue(name, value);
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, false);
        return SetRegistryKeyHelper(regKey, name, value);
#endif 
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("Getting registry configuration from remote not supported in linux");
#else
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, machineName, false);
        return SetRegistryKeyHelper(regKey, name, value);
#endif 
    }
}

ErrorCode FabricEnvironment::SetRegistryKeyHelper(RegistryKey & regKey, const wstring & name, const wstring & value)
{
    if (regKey.IsValid)
    {
        if (regKey.SetValue(name, value))
        {
            return ErrorCodeValue::Success;
        }
    }
    return ErrorCode::FromWin32Error(regKey.Error);
}


ErrorCode FabricEnvironment::SetRegistryKeyHelper(RegistryKey & regKey, const wstring & name, DWORD value)
{
    if (regKey.IsValid)
    {
        if (regKey.SetValue(name, value))
        {
            return ErrorCodeValue::Success;
        }
    }
    return ErrorCode::FromWin32Error(regKey.Error);
}

void FabricEnvironment::GetFabricTracesTestKeyword(__out std::wstring & keyword)
{
    bool testKeywordEnvFound = Environment::GetEnvironmentVariable(L"FabricTracesTestKeyword", keyword, Common::NOTHROW());
    if (!testKeywordEnvFound)
    {
        keyword.clear();
    }
}