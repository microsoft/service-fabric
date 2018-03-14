// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport 
{
    namespace SecuritySettingsNames
    {
        std::wstring const CredentialType = L"CredentialType";

        std::wstring const AllowedCommonNames = L"AllowedCommonNames";
        std::wstring const IssuerThumbprints = L"IssuerThumbprints";
        std::wstring const RemoteCertThumbprints = L"RemoteCertThumbprints";
        std::wstring const FindType = L"FindType";
        std::wstring const FindValue = L"FindValue";
        std::wstring const FindValueSecondary = L"FindValueSecondary";
        std::wstring const StoreLocation = L"StoreLocation";
        std::wstring const StoreName = L"StoreName";
        std::wstring const ProtectionLevel = L"ProtectionLevel";

        // Kerberos transport security settings:
        std::wstring const ServicePrincipalName = L"ServicePrincipalName"; // Service principal name of all replication listeners for this service group
        std::wstring const WindowsIdentities = L"WindowsIdentities"; // Used to authorize incoming replication connections for this service group

        std::wstring const ApplicationIssuerStorePrefix = L"ApplicationIssuerStore/";
    }

    class ComSecurityCredentialsResult
        : public Common::TextTraceComponent<Common::TraceTaskCodes::Transport>,
        public IFabricSecurityCredentialsResult,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComSecurityCredentialsResult)

        COM_INTERFACE_LIST1(
        ComSecurityCredentialsResult,
        IID_IFabricSecurityCredentialsResult,
        IFabricSecurityCredentialsResult)

    public:
        static HRESULT FromConfig(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in std::wstring const & configurationPackageName,
            __in std::wstring const & sectionName,
            __out IFabricSecurityCredentialsResult ** securityCredentialsResult);

        static HRESULT ClusterSecuritySettingsFromConfig(
            __out IFabricSecurityCredentialsResult ** securityCredentialsResult);
        
        ComSecurityCredentialsResult(SecuritySettings const & value);
        virtual ~ComSecurityCredentialsResult();
        const FABRIC_SECURITY_CREDENTIALS * STDMETHODCALLTYPE get_SecurityCredentials();

        static HRESULT STDMETHODCALLTYPE ReturnSecurityCredentialsResult(
            SecuritySettings && settings,
            IFabricSecurityCredentialsResult ** value);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SECURITY_CREDENTIALS> credentials_;
    };
}
