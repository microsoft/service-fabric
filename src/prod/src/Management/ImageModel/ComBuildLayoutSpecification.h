// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComBuildLayoutSpecification : 
            public IFabricBuildLayoutSpecification2,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComBuildLayoutSpecification)

            BEGIN_COM_INTERFACE_LIST(ComBuildLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IUnknown,IFabricBuildLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricBuildLayoutSpecification,IFabricBuildLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricBuildLayoutSpecification2,IFabricBuildLayoutSpecification2)
            END_COM_INTERFACE_LIST()

        public:
            static HRESULT FabricCreateBuildLayoutSpecification( 
                /* [in] */ REFIID riid,
                /* [retval][out] */ void **buildLayoutSpecification);

            ComBuildLayoutSpecification();
            ~ComBuildLayoutSpecification();

            HRESULT STDMETHODCALLTYPE SetRoot( 
                /* [in] */ LPCWSTR value);
        
            HRESULT STDMETHODCALLTYPE GetRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE GetApplicationManifestFile( 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetServiceManifestFile( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetServiceManifestChecksumFile( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR codePackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageChecksumFile( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR codePackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetConfigPackageFolder( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR configPackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetConfigPackageChecksumFile( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR configPackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetDataPackageFolder( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR dataPackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetDataPackageChecksumFile( 
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR dataPackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetSettingsFile( 
                /* [in] */ LPCWSTR configPackageFolder,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetSubPackageArchiveFile( 
                /* [in] */ LPCWSTR packageFolder,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetChecksumFile( 
                /* [in] */ LPCWSTR fileOrDirectoryName,
                /* [retval][out] */ IFabricStringResult **result);

        private:
            BuildLayoutSpecification buildLayout_;
        };
    }
}

