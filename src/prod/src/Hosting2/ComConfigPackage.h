// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComConfigPackage :
        public IFabricConfigurationPackage2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComConfigPackage)

        BEGIN_COM_INTERFACE_LIST(ComConfigPackage)
          COM_INTERFACE_ITEM(IID_IUnknown, IFabricConfigurationPackage)
          COM_INTERFACE_ITEM(IID_IFabricConfigurationPackage, IFabricConfigurationPackage)
          COM_INTERFACE_ITEM(IID_IFabricConfigurationPackage2, IFabricConfigurationPackage2)
        END_COM_INTERFACE_LIST()

    public:
        ComConfigPackage(
            ServiceModel::ConfigPackageDescription const & configPackageDescription, 
            std::wstring const& serviceManifestName,
            std::wstring const& serviceManifestVersion,
            std::wstring const& path,
            Common::ConfigSettings const & configSettings);

        virtual ~ComConfigPackage();

        const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION * STDMETHODCALLTYPE get_Description(void);

        LPCWSTR STDMETHODCALLTYPE get_Path(void);

        const FABRIC_CONFIGURATION_SETTINGS * STDMETHODCALLTYPE get_Settings(void);
        
        HRESULT STDMETHODCALLTYPE GetSection( 
            /* [in] */ LPCWSTR sectionName,
            /* [retval][out] */ const FABRIC_CONFIGURATION_SECTION **bufferedValue);
        
        HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterName,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ LPCWSTR *bufferedValue);

        HRESULT STDMETHODCALLTYPE GetValues(
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterPrefix,
            /* [out, retval] */ FABRIC_CONFIGURATION_PARAMETER_LIST ** bufferedValue);

        HRESULT STDMETHODCALLTYPE DecryptValue(
            /* [in] */ LPCWSTR encryptedValue,
            /* [out, retval] */ IFabricStringResult ** decryptedValue);


    private:
        static HRESULT FindSection(
            const FABRIC_CONFIGURATION_SETTINGS * settings,
            LPCWSTR sectionName,
            const FABRIC_CONFIGURATION_SECTION **section);

        static HRESULT FindParameter(
            const FABRIC_CONFIGURATION_SECTION * section,
            LPCWSTR parameterName,
            const FABRIC_CONFIGURATION_PARAMETER **parameter);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION> configPackageDescription_;
        std::wstring path_;
        Common::ReferencePointer<FABRIC_CONFIGURATION_SETTINGS> configSettings_;
    };
}
