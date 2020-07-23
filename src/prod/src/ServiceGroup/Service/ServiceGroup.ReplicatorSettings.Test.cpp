// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Common/Common.h"
#include "ServiceGroup/Service/ServiceGroup.ReplicatorSettings.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceGroupTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceGroup;

#define HAS_FLAG(replicatorSettings, flag) \
    (flag == (flag & (replicatorSettings->Flags)))

#define VERIFY_HAS_FLAG(replicatorSettings, flag) \
    VERIFY_IS_TRUE(HAS_FLAG(replicatorSettings, flag))

#define VERIFY_NOT_HAS_FLAG(replicatorSettings, flag) \
    VERIFY_IS_FALSE(HAS_FLAG(replicatorSettings, flag))

    class DummyConfig : public IFabricConfigurationPackage2,
                        public IFabricCodePackageActivationContext,
                        private Common::ComUnknownBase
    {
        DENY_COPY(DummyConfig)

        BEGIN_COM_INTERFACE_LIST(DummyConfig)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricConfigurationPackage)
            COM_INTERFACE_ITEM(IID_IFabricConfigurationPackage, IFabricConfigurationPackage)
            COM_INTERFACE_ITEM(IID_IFabricConfigurationPackage2, IFabricConfigurationPackage2)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext, IFabricCodePackageActivationContext)
        END_COM_INTERFACE_LIST()

    public:

        DummyConfig(wstring const & sectionName) :
            sectionName_(sectionName),
            endpointList_(nullptr)
        {
        }

        ~DummyConfig()
        {
        }

        //
        // IFabricConfigurationPackage members
        //
        const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION *STDMETHODCALLTYPE get_Description(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_Path(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        const FABRIC_CONFIGURATION_SETTINGS *STDMETHODCALLTYPE get_Settings(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetSection( 
            __in LPCWSTR sectionName,
            __out const FABRIC_CONFIGURATION_SECTION **bufferedValue)
        {
            UNREFERENCED_PARAMETER(sectionName);
            UNREFERENCED_PARAMETER(bufferedValue);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetValue( 
            __in LPCWSTR sectionName,
            __in LPCWSTR parameterName,
            __out BOOLEAN *isEncrypted,
            __out LPCWSTR *bufferedValue)
        {
            if (sectionName == nullptr || parameterName == nullptr || bufferedValue == nullptr)
            {
                Common::Assert::CodingError("Parameters must not be null");
            }

            if (wstring(sectionName) != sectionName_)
            {
                return FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND;
            }

            auto entry = settings_.find(wstring(parameterName));
            if (settings_.end() == entry)
            {
                return FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND;
            }
            
            *isEncrypted = false;
            *bufferedValue = entry->second.c_str();

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetValues(
            __in LPCWSTR sectionName,
            __in LPCWSTR parameterPrefix,
            __out FABRIC_CONFIGURATION_PARAMETER_LIST ** bufferedValue)
        {
            if ((sectionName == nullptr) || (parameterPrefix == nullptr) || (bufferedValue == nullptr))
            {
                Common::Assert::CodingError("Parameters must not be null");
            }

            if (wstring(sectionName) != sectionName_)
            {
                return FABRIC_E_CONFIGURATION_SECTION_NOT_FOUND;
            }

            std::list<ConfigParameter> values;
            for (auto it = settings_.begin(); it != settings_.end(); ++it)
            {
                if (StringUtility::StartsWithCaseInsensitive<wstring>(it->first, parameterPrefix))
                {
                    ConfigParameter param;
                    param.Name = it->first;
                    param.Value = it->second;
                    values.emplace_back(param);
                }
            }

            if (values.empty())
            {
                return ::FABRIC_E_CONFIGURATION_PARAMETER_NOT_FOUND;
            }

            auto parameterList = heap_.AddItem<FABRIC_CONFIGURATION_PARAMETER_LIST>();
            parameterList->Count = static_cast<ULONG>(values.size());
            auto parameterItems = heap_.AddArray<FABRIC_CONFIGURATION_PARAMETER>(values.size());
            parameterList->Items = parameterItems.GetRawArray();
            int i = 0;
            for (auto iter = values.begin(); iter != values.end(); ++iter)
            {
                iter->ToPublicApi(heap_, parameterItems[i]);
                i++;
            }

            *bufferedValue = parameterList.GetRawPointer();

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE DecryptValue(
            __in LPCWSTR encryptedValue,
            __out IFabricStringResult ** decryptedValue)
        {
            UNREFERENCED_PARAMETER(encryptedValue);
            UNREFERENCED_PARAMETER(decryptedValue);
            // The value returned by GetValue method is always unencrypted.
            // So no implementation of DecryptValue is provided.
            return E_NOTIMPL;
        }


       void AddValue(wstring const & name, wstring const & value)
        {
            settings_.insert(make_pair(name, value));
        }

       void RemoveValue(wstring const & name)
        {
            settings_.erase(name);
        }

        //
        // IFabricCodePackageActivationContextMembers
        //
        LPCWSTR STDMETHODCALLTYPE get_ContextId(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_CodePackageName(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_CodePackageVersion(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_WorkDirectory(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_LogDirectory(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        LPCWSTR STDMETHODCALLTYPE get_TempDirectory(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceTypes(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceGroupTypes(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *STDMETHODCALLTYPE get_ApplicationPrincipals(void)
        {
            Common::Assert::CodingError("Unexpected call");
        }
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceEndpointResources(void)
        {
            if (nullptr != endpointList_)
            {
                return endpointList_;
            }
            
            auto endpointList = endpointHeap_.AddItem<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST>();

            endpointList->Count = (ULONG)endpoints_.size();
            auto endpoints = endpointHeap_.AddArray<FABRIC_ENDPOINT_RESOURCE_DESCRIPTION>(endpoints_.size());

            size_t cnt = 0;
            for (auto it = begin(endpoints_); it != end(endpoints_); ++it, ++cnt)
            {
                endpoints[cnt].Name = endpointHeap_.AddString(it->first);
                endpoints[cnt].Type = endpointHeap_.AddString(it->second.Type);
                endpoints[cnt].Protocol = endpointHeap_.AddString(it->second.Protocol);
                endpoints[cnt].CertificateName = endpointHeap_.AddString(it->second.CertificateName);
                endpoints[cnt].Port = it->second.Port;
                endpoints[cnt].Reserved = nullptr;
            }

            endpointList->Items = endpoints.GetRawArray();
            endpointList_ = endpointList.GetRawPointer();

            return endpointList_;
        }
        
        HRESULT STDMETHODCALLTYPE GetServiceEndpointResource( 
            __in LPCWSTR serviceEndpointResourceName,
            __out const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue)
        {
            UNREFERENCED_PARAMETER(serviceEndpointResourceName);
            UNREFERENCED_PARAMETER(bufferedValue);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetCodePackageNames( 
            __out IFabricStringListResult **names)
        {
            UNREFERENCED_PARAMETER(names);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetConfigurationPackageNames( 
            __out IFabricStringListResult **names)
        {
            UNREFERENCED_PARAMETER(names);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetDataPackageNames( 
            __out IFabricStringListResult **names)
        {
            UNREFERENCED_PARAMETER(names);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetCodePackage( 
            __in LPCWSTR codePackageName,
            __out IFabricCodePackage **codePackage)
        {
            UNREFERENCED_PARAMETER(codePackageName);
            UNREFERENCED_PARAMETER(codePackage);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetConfigurationPackage( 
            __in LPCWSTR configPackageName,
            __out IFabricConfigurationPackage **configPackage)
        {
            UNREFERENCED_PARAMETER(configPackageName);
            UNREFERENCED_PARAMETER(configPackage);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE GetDataPackage( 
            __in LPCWSTR dataPackageName,
            __out IFabricDataPackage **dataPackage)
        {
            UNREFERENCED_PARAMETER(dataPackageName);
            UNREFERENCED_PARAMETER(dataPackage);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE RegisterCodePackageChangeHandler( 
            __in IFabricCodePackageChangeHandler *callback,
            __out LONGLONG *callbackHandle)
        {
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE UnregisterCodePackageChangeHandler( 
            __in LONGLONG callbackHandle)
        {
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE RegisterConfigurationPackageChangeHandler( 
            __in IFabricConfigurationPackageChangeHandler *callback,
            __out LONGLONG *callbackHandle)
        {
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE UnregisterConfigurationPackageChangeHandler( 
            __in LONGLONG callbackHandle)
        {
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE RegisterDataPackageChangeHandler( 
            __in IFabricDataPackageChangeHandler *callback,
            __out LONGLONG *callbackHandle)
        {
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }
        
        HRESULT STDMETHODCALLTYPE UnregisterDataPackageChangeHandler( 
            __in LONGLONG callbackHandle)
        {
            UNREFERENCED_PARAMETER(callbackHandle);
            Common::Assert::CodingError("Unexpected call");
        }

        void AddEndpoint(wstring const & name, wstring const & protocol, wstring const & type, ULONG port, wstring const & certname)
        {
            endpointList_ = nullptr;

            EndpointResource resource;
            resource.Protocol = protocol;
            resource.Type = type;
            resource.Port = port;
            resource.CertificateName = certname;

            endpoints_.insert(make_pair(name, resource));
        }

        void RemoveEndpoint(wstring const & name)
        {
            endpointList_ = nullptr;

            endpoints_.erase(name);
        }

    private:

        struct EndpointResource
        {
            wstring Protocol;
            wstring Type;
            ULONG Port;
            wstring CertificateName;
        };

        map<wstring, wstring> settings_;
        map<wstring, EndpointResource> endpoints_;

        Common::ScopedHeap endpointHeap_;
        FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST* endpointList_;

        wstring sectionName_;
        ScopedHeap heap_;
    };

    class ReplicatorSettingsTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(ReplicatorSettingsTestSuite,ReplicatorSettingsTest)

    BOOST_AUTO_TEST_CASE(CreateFromEmptyConfigurationPackage)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);
    }

    BOOST_AUTO_TEST_CASE(CreateFromEmptyConfigurationPackageWithMissingSection)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        //
        // make sure the ReplicatorSettings section does not exist
        //
        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"SomeUnrelatedSection");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);
    }

    BOOST_AUTO_TEST_CASE(CreateWithEndpointNoMatch)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        //
        // Missspelled/missing endpoint resource
        //
        config->AddEndpoint(L"MyReplicatorEndpoint_ResourceName", L"tcp", L"Internal", 40000, L"");
        config->AddValue(ReplicatorSettingsNames::ReplicatorEndpoint, L"MyReplicatorEndpoint_WrongResourceName");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        //
        // Specified endpoint is missing. This should fail.
        //
        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(FAILED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);
    }

    BOOST_AUTO_TEST_CASE(CreateWithInvalidEndpointSettings)
    {
        Common::Guid partitionId;
        

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        //
        // Input endpoints are not supported as replicator endpoints
        //
        config->AddEndpoint(L"MyReplicatorEndpoint", L"tcp", L"Input", 40000, L"");
        config->AddValue(ReplicatorSettingsNames::ReplicatorEndpoint, L"MyReplicatorEndpoint");

        ReplicatorSettings settingsInputEndpoint(partitionId);
        HRESULT hr = settingsInputEndpoint.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(FAILED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settingsInputEndpoint.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);

        //
        // Non tcp endpoints are not supported as replicator endpoints
        //
        config->RemoveEndpoint(L"MyReplicatorEndpoint");
        config->AddEndpoint(L"MyReplicatorEndpoint", L"http", L"Internal", 40000, L"");
        
        ReplicatorSettings settingsHttpEndpoint(partitionId);
        hr = settingsHttpEndpoint.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(FAILED(hr));

        publicApiValue = settingsHttpEndpoint.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);
    }

    BOOST_AUTO_TEST_CASE(CreateWithSecuritySettings)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        config->AddValue(ReplicatorSettingsNames::AllowedCommonNames, L"bob");
        config->AddValue(ReplicatorSettingsNames::FindType, wformatString(X509FindType::FindBySubjectName));
        config->AddValue(ReplicatorSettingsNames::FindValue, L"CN=WinFabric-Test-SAN1-Alice");
        config->AddValue(ReplicatorSettingsNames::FindValueSecondary, L"CN=WinFabric-Test-SAN1-Bob");
        config->AddValue(ReplicatorSettingsNames::CredentialType, L"X509");
        config->AddValue(ReplicatorSettingsNames::StoreLocation, wformatString(X509Default::StoreLocation()));
        config->AddValue(ReplicatorSettingsNames::StoreName, X509Default::StoreName()); 
        config->AddValue(ReplicatorSettingsNames::ProtectionLevel, wformatString(Transport::ProtectionLevel::EncryptAndSign));

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NOT_NULL(publicApiValue);

        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECURITY);
        VERIFY_IS_NOT_NULL(publicApiValue->SecurityCredentials);

        VERIFY_IS_TRUE(FABRIC_SECURITY_CREDENTIAL_KIND_X509 == publicApiValue->SecurityCredentials->Kind);
        VERIFY_IS_NOT_NULL(publicApiValue->SecurityCredentials->Value);

        FABRIC_X509_CREDENTIALS const * credentials = (FABRIC_X509_CREDENTIALS const*)(publicApiValue->SecurityCredentials->Value);
        FABRIC_X509_CREDENTIALS_EX1 const * x509Ex1 = (FABRIC_X509_CREDENTIALS_EX1 const *)credentials->Reserved;
        FABRIC_X509_CREDENTIALS_EX2 const * x509Ex2 = (FABRIC_X509_CREDENTIALS_EX2 const *)x509Ex1->Reserved;

        VERIFY_IS_NOT_NULL(credentials->StoreName);
        VERIFY_IS_NOT_NULL(credentials->FindValue);
        VERIFY_IS_TRUE(FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN == credentials->ProtectionLevel);
        VERIFY_IS_TRUE(X509Default::StoreLocation_Public() == credentials->StoreLocation);
        VERIFY_IS_TRUE(X509Default::StoreName() == wstring(credentials->StoreName));
        VERIFY_IS_TRUE(wstring(L"CN=WinFabric-Test-SAN1-Alice") == wstring((LPCWSTR)credentials->FindValue));
        VERIFY_IS_TRUE(wstring(L"CN=WinFabric-Test-SAN1-Bob") == wstring((LPCWSTR)x509Ex2->FindValueSecondary));
        VERIFY_IS_TRUE(FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME == credentials->FindType);
    }

    BOOST_AUTO_TEST_CASE(CreateWithSecuritySettingsWithIssuerStores)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        config->AddValue(ReplicatorSettingsNames::AllowedCommonNames, L"WinFabric-Test-SAN1-Bob");
        config->AddValue(ReplicatorSettingsNames::ApplicationIssuerStorePrefix + L"WinFabric-Test-TA-CA", L"Root");
        config->AddValue(ReplicatorSettingsNames::ApplicationIssuerStorePrefix + L"WinFabric-Test-Expired", L"Root");
        config->AddValue(ReplicatorSettingsNames::FindType, wformatString(X509FindType::FindBySubjectName));
        config->AddValue(ReplicatorSettingsNames::FindValue, L"WinFabric-Test-SAN1-Alice");
        config->AddValue(ReplicatorSettingsNames::CredentialType, L"X509");
        config->AddValue(ReplicatorSettingsNames::StoreLocation, wformatString(X509Default::StoreLocation()));
        config->AddValue(ReplicatorSettingsNames::StoreName, X509Default::StoreName());
        config->AddValue(ReplicatorSettingsNames::ProtectionLevel, wformatString(Transport::ProtectionLevel::EncryptAndSign));

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NOT_NULL(publicApiValue);

        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECURITY);
        VERIFY_IS_NOT_NULL(publicApiValue->SecurityCredentials);

        VERIFY_IS_TRUE(FABRIC_SECURITY_CREDENTIAL_KIND_X509 == publicApiValue->SecurityCredentials->Kind);
        VERIFY_IS_NOT_NULL(publicApiValue->SecurityCredentials->Value);

        FABRIC_X509_CREDENTIALS const * credentials = (FABRIC_X509_CREDENTIALS const*)(publicApiValue->SecurityCredentials->Value);
        FABRIC_X509_CREDENTIALS_EX1 const * x509Ex1 = (FABRIC_X509_CREDENTIALS_EX1 const *)credentials->Reserved;
        FABRIC_X509_CREDENTIALS_EX2 const * x509Ex2 = (FABRIC_X509_CREDENTIALS_EX2 const *)x509Ex1->Reserved;
        FABRIC_X509_CREDENTIALS_EX3 const * x509Ex3 = (FABRIC_X509_CREDENTIALS_EX3 const *)x509Ex2->Reserved;

        VERIFY_IS_NOT_NULL(credentials->StoreName);
        VERIFY_IS_NOT_NULL(credentials->FindValue);
        VERIFY_IS_TRUE(FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN == credentials->ProtectionLevel);
        VERIFY_IS_TRUE(X509Default::StoreLocation_Public() == credentials->StoreLocation);
        VERIFY_IS_TRUE(X509Default::StoreName() == wstring(credentials->StoreName));
        VERIFY_IS_TRUE(wstring(L"WinFabric-Test-SAN1-Alice") == wstring((LPCWSTR)credentials->FindValue));
        VERIFY_IS_TRUE(FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME == credentials->FindType);
        VERIFY_IS_TRUE(x509Ex2->RemoteX509NameCount == 1);

        VERIFY_IS_TRUE(x509Ex3->RemoteCertIssuerCount == 2);
        VERIFY_IS_TRUE(x509Ex3->RemoteCertIssuers[0].IssuerStoreCount == 1);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3->RemoteCertIssuers[0].Name, L"WinFabric-Test-Expired"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3->RemoteCertIssuers[0].IssuerStores[0], L"Root"));
        VERIFY_IS_TRUE(x509Ex3->RemoteCertIssuers[1].IssuerStoreCount == 1);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3->RemoteCertIssuers[1].Name, L"WinFabric-Test-TA-CA"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3->RemoteCertIssuers[1].IssuerStores[0], L"Root"));
    }

    BOOST_AUTO_TEST_CASE(CreateWithInvalidSecuritySettings)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        //
        // Any of this will cause security settings to fail
        //
        config->AddValue(ReplicatorSettingsNames::AllowedCommonNames, L"bob");
        config->AddValue(ReplicatorSettingsNames::FindType, L"FindLongestC++File");
        config->AddValue(ReplicatorSettingsNames::FindValue, L"the dude");
        config->AddValue(ReplicatorSettingsNames::CredentialType, L"1701");
        config->AddValue(ReplicatorSettingsNames::StoreLocation, L"LeftMachine");
        config->AddValue(ReplicatorSettingsNames::StoreName, L"Yours");
        config->AddValue(ReplicatorSettingsNames::ProtectionLevel, L"Pretend");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_FALSE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NULL(publicApiValue);
    }

    BOOST_AUTO_TEST_CASE(CreateWithSettings)
    {
        Common::Guid partitionId;
        ReplicatorSettings settings(partitionId);

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        config->AddValue(ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds, L"20");
        config->AddValue(ReplicatorSettingsNames::InitialCopyQueueSize, L"30");
        config->AddValue(ReplicatorSettingsNames::InitialReplicationQueueSize, L"31");
        config->AddValue(ReplicatorSettingsNames::MaxCopyQueueSize, L"60");
        config->AddValue(ReplicatorSettingsNames::MaxReplicationQueueSize, L"61");
        config->AddValue(ReplicatorSettingsNames::RequireServiceAck, L"true");
        config->AddValue(ReplicatorSettingsNames::RetryIntervalMilliseconds, L"70");
        config->AddValue(ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations, L"true");
        config->AddValue(ReplicatorSettingsNames::MaxReplicationQueueMemorySize, L"1024");
        config->AddValue(ReplicatorSettingsNames::MaxReplicationMessageSize, L"9999");
        config->AddValue(ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck, L"false"); // true is not supported
        config->AddValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize, L"1024");
        config->AddValue(ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize, L"33");
        config->AddValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize, L"63");
        config->AddValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize, L"124");
        config->AddValue(ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize, L"38");
        config->AddValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize, L"43");
        config->AddValue(ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds, L"78");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
        VERIFY_IS_TRUE(SUCCEEDED(hr));

        FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
        VERIFY_IS_NOT_NULL(publicApiValue);

        FABRIC_REPLICATOR_SETTINGS_EX1* publicEx1ApiValue = (FABRIC_REPLICATOR_SETTINGS_EX1*)publicApiValue->Reserved;
        VERIFY_IS_NOT_NULL(publicEx1ApiValue);

        FABRIC_REPLICATOR_SETTINGS_EX2* publicEx2ApiValue = (FABRIC_REPLICATOR_SETTINGS_EX2*)publicEx1ApiValue->Reserved;
        VERIFY_IS_NOT_NULL(publicEx2ApiValue);

        FABRIC_REPLICATOR_SETTINGS_EX3* publicEx3ApiValue = (FABRIC_REPLICATOR_SETTINGS_EX3*)publicEx2ApiValue->Reserved;
        VERIFY_IS_NOT_NULL(publicEx3ApiValue);

        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_RETRY_INTERVAL);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE);
        VERIFY_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT);

        VERIFY_NOT_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_SECURITY);
        VERIFY_NOT_HAS_FLAG(publicApiValue, FABRIC_REPLICATOR_ADDRESS);

        VERIFY_IS_NULL(publicApiValue->SecurityCredentials);

        VERIFY_IS_TRUE(20 == publicApiValue->BatchAcknowledgementIntervalMilliseconds);
        VERIFY_IS_TRUE(30 == publicApiValue->InitialCopyQueueSize);
        VERIFY_IS_TRUE(31 == publicApiValue->InitialReplicationQueueSize);
        VERIFY_IS_TRUE(60 == publicApiValue->MaxCopyQueueSize);
        VERIFY_IS_TRUE(61 == publicApiValue->MaxReplicationQueueSize);
        VERIFY_IS_TRUE(TRUE == publicApiValue->RequireServiceAck);
        VERIFY_IS_TRUE(70 == publicApiValue->RetryIntervalMilliseconds);
        VERIFY_IS_TRUE(TRUE == publicEx1ApiValue->SecondaryClearAcknowledgedOperations);
        VERIFY_IS_TRUE(1024 == publicEx1ApiValue->MaxReplicationQueueMemorySize);
        VERIFY_IS_TRUE(9999 == publicEx1ApiValue->MaxReplicationMessageSize);
        VERIFY_IS_TRUE(FALSE == publicEx2ApiValue->UseStreamFaultsAndEndOfStreamOperationAck);
        VERIFY_IS_TRUE(1024 == publicEx3ApiValue->MaxSecondaryReplicationQueueMemorySize);
        VERIFY_IS_TRUE(33 == publicEx3ApiValue->InitialSecondaryReplicationQueueSize);
        VERIFY_IS_TRUE(63 == publicEx3ApiValue->MaxSecondaryReplicationQueueSize);
        VERIFY_IS_TRUE(124 == publicEx3ApiValue->MaxPrimaryReplicationQueueMemorySize);
        VERIFY_IS_TRUE(38 == publicEx3ApiValue->InitialPrimaryReplicationQueueSize);
        VERIFY_IS_TRUE(43 == publicEx3ApiValue->MaxPrimaryReplicationQueueSize);
        VERIFY_IS_TRUE(78 == publicEx3ApiValue->PrimaryWaitForPendingQuorumsTimeoutMilliseconds);
    }

    BOOST_AUTO_TEST_CASE(CreateWithInvalidSettings)
    {
        Common::Guid partitionId;

        ComPointer<DummyConfig> config = make_com<DummyConfig>(L"ReplicatorSettings");

        config->AddValue(ReplicatorSettingsNames::RetryIntervalMilliseconds, L"70");

        ComPointer<IFabricCodePackageActivationContext> context;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricCodePackageActivationContext, context.VoidInitializationAddress())));

        ComPointer<IFabricConfigurationPackage> package;
        VERIFY_IS_TRUE(SUCCEEDED(config->QueryInterface(IID_IFabricConfigurationPackage, package.VoidInitializationAddress())));

        {
            config->AddValue(ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds, L"twenty");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::BatchAcknowledgementIntervalMilliseconds);
        }

        {
            config->AddValue(ReplicatorSettingsNames::InitialCopyQueueSize, L"thirty");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::InitialCopyQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::InitialReplicationQueueSize, L"thirty+1");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::InitialReplicationQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxCopyQueueSize, L"sixty");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::InitialCopyQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxReplicationQueueSize, L"2*30+1");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxReplicationQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::RequireServiceAck, L"maybe");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::RequireServiceAck);
        }

        {
            config->AddValue(ReplicatorSettingsNames::RetryIntervalMilliseconds, L"seventy");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::RetryIntervalMilliseconds);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxReplicationQueueMemorySize, L"ten");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxReplicationQueueMemorySize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations, L"maybe");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::SecondaryClearAcknowledgedOperations);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxReplicationMessageSize, L"ninehundred");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxReplicationMessageSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck, L"true");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck);
        }

        {
            config->AddValue(ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::UseStreamFaultsAndEndOfStreamOperationAck);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueMemorySize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxSecondaryReplicationQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::InitialSecondaryReplicationQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueMemorySize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::MaxPrimaryReplicationQueueSize);
        }

        {
            config->AddValue(ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::InitialPrimaryReplicationQueueSize);
        }
        
        {
            config->AddValue(ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds, L"hello");

            ReplicatorSettings settings(partitionId);
            HRESULT hr = settings.ReadFromConfigurationPackage(package.GetRawPointer(), context.GetRawPointer());
            VERIFY_IS_FALSE(SUCCEEDED(hr));

            FABRIC_REPLICATOR_SETTINGS* publicApiValue = settings.GetRawPointer();
            VERIFY_IS_NULL(publicApiValue);

            config->RemoveValue(ReplicatorSettingsNames::PrimaryWaitForPedingQuorumsTimeoutMilliseconds);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
