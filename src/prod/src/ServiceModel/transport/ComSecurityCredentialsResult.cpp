// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace ServiceModel;
    
StringLiteral const TraceType("SecurityCredentialsResult");

ComSecurityCredentialsResult::ComSecurityCredentialsResult(SecuritySettings const & value)
    : ComUnknownBase()
    , heap_()
    , credentials_()
{
    credentials_ = heap_.AddItem<FABRIC_SECURITY_CREDENTIALS>();
    value.ToPublicApi(heap_, *credentials_);
}

ComSecurityCredentialsResult::~ComSecurityCredentialsResult()
{
}

const FABRIC_SECURITY_CREDENTIALS * STDMETHODCALLTYPE ComSecurityCredentialsResult::get_SecurityCredentials(void)
{
    return credentials_.GetRawPointer();
}


HRESULT STDMETHODCALLTYPE ComSecurityCredentialsResult::ReturnSecurityCredentialsResult(
    SecuritySettings && settings,
    IFabricSecurityCredentialsResult ** value)
{
    ComPointer<IFabricSecurityCredentialsResult> securityCredentialsResult = make_com<ComSecurityCredentialsResult, IFabricSecurityCredentialsResult>(settings);
    *value = securityCredentialsResult.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT ComSecurityCredentialsResult::FromConfig(
    __in ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in std::wstring const & configurationPackageName,
    __in std::wstring const & sectionName,
    __out IFabricSecurityCredentialsResult ** securityCredentialsResult)
{
    // Validate code package activation context is not null
    if (codePackageActivationContextCPtr.GetRawPointer() == NULL) 
    {
        WriteError(TraceType, "CodePackageActivationContext is NULL");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    
    ComPointer<IFabricConfigurationPackage> configPackageCPtr;
    HRESULT hr = codePackageActivationContextCPtr->GetConfigurationPackage(configurationPackageName.c_str(), configPackageCPtr.InitializationAddress());

    // Validate the configuration package exists
    if (hr != S_OK ||
        configPackageCPtr.GetRawPointer() == NULL)
    {
        WriteError(TraceType, "Failed to load configuration package - {0}, HRESULT = {1}", configurationPackageName, hr);
        return ComUtility::OnPublicApiReturn(hr);
    }

    wstring credentialType;
    wstring allowedCommonNames;
    wstring findType;
    wstring findValue;
    wstring findValueSecondary;
    wstring storeLocation;
    wstring storeName;
    wstring protectionLevel;

    // credential type
    bool hasValue = false;
    ErrorCode error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::CredentialType, credentialType, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // allowed common names
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::AllowedCommonNames, allowedCommonNames, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    wstring issuerThumbprints;
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::IssuerThumbprints, issuerThumbprints, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    wstring remoteCertThumbprints;
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::RemoteCertThumbprints, remoteCertThumbprints, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // issuer store entries
    ComPointer<IFabricConfigurationPackage2> configPackage2CPtr;
    hr = configPackageCPtr->QueryInterface(
        IID_IFabricConfigurationPackage2,
        configPackage2CPtr.VoidInitializationAddress());
    if (hr != S_OK)
    {
        WriteError(TraceType, "Failed to load configuration package for IFabricConfigurationPackage2 - {0}, HRESULT = {1}", configurationPackageName, hr);
        return ComUtility::OnPublicApiReturn(hr);
    }
    Common::StringMap issuerStores;
    error = Parser::ReadSettingsValue(configPackage2CPtr, sectionName, SecuritySettingsNames::ApplicationIssuerStorePrefix, issuerStores, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // find type
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::FindType, findType, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // find value
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::FindValue, findValue, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // find value secondary
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::FindValueSecondary, findValueSecondary, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // store location
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::StoreLocation, storeLocation, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // store name
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::StoreName, storeName, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // protection level
    hasValue = false;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::ProtectionLevel, protectionLevel, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // service principal name
    wstring servicePrincipalName;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::ServicePrincipalName, servicePrincipalName, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    // Windows identity list of all replicas
    wstring windowsIdentities;
    error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, SecuritySettingsNames::WindowsIdentities, windowsIdentities, hasValue);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    SecuritySettings localSecuritySettings;

    error = SecuritySettings::FromConfiguration(
        credentialType,
        storeName,
        storeLocation,
        findType,
        findValue,
        findValueSecondary,
        protectionLevel,
        remoteCertThumbprints,
        SecurityConfig::X509NameMap(), // todo, add support X509Name config section in app config
        SecurityConfig::IssuerStoreKeyValueMap::Parse(issuerStores),
        allowedCommonNames,
        issuerThumbprints,
        servicePrincipalName,
        windowsIdentities,
        localSecuritySettings);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }
    else
    {
        return ComSecurityCredentialsResult::ReturnSecurityCredentialsResult(move(localSecuritySettings), securityCredentialsResult);
    }
}
 
HRESULT ComSecurityCredentialsResult::ClusterSecuritySettingsFromConfig(
    __out IFabricSecurityCredentialsResult ** securityCredentialsResult)
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();  
    FabricNodeConfigSPtr config_ = make_shared<FabricNodeConfig>();
    SecuritySettings clusterSecuritySettings;
    ErrorCode error = SecuritySettings::FromConfiguration(
        securityConfig.ClusterCredentialType,
        config_->ClusterX509StoreName,
        wformatString(X509StoreLocation::LocalMachine),
        config_->ClusterX509FindType,
        config_->ClusterX509FindValue,
        config_->ClusterX509FindValueSecondary,
        securityConfig.ClusterProtectionLevel,
        securityConfig.ClusterCertThumbprints,
        securityConfig.ClusterX509Names,
        securityConfig.ClusterCertificateIssuerStores,
        securityConfig.ClusterAllowedCommonNames,
        securityConfig.ClusterCertIssuers,
        securityConfig.ClusterSpn,
        securityConfig.ClusterIdentities,
        clusterSecuritySettings);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }
    else
    {
        return ComSecurityCredentialsResult::ReturnSecurityCredentialsResult(move(clusterSecuritySettings), securityCredentialsResult);
    }
}

