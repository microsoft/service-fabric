// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComStoreLayoutSpecification : 
            public IFabricStoreLayoutSpecification2,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComStoreLayoutSpecification)

            BEGIN_COM_INTERFACE_LIST(ComStoreLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IUnknown,IFabricStoreLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricStoreLayoutSpecification,IFabricStoreLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricStoreLayoutSpecification2,IFabricStoreLayoutSpecification2)
            END_COM_INTERFACE_LIST()

        public:
            static HRESULT FabricCreateStoreLayoutSpecification( 
                /* [in] */ REFIID riid,
                /* [retval][out] */ void **storeLayoutSpecification);

            ComStoreLayoutSpecification();
            ~ComStoreLayoutSpecification();

            HRESULT STDMETHODCALLTYPE SetRoot( 
                /* [in] */ LPCWSTR value);
        
            HRESULT STDMETHODCALLTYPE GetRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE GetApplicationManifestFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR applicationTypeVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetApplicationInstanceFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR aplicationId,
                /* [in] */ LPCWSTR applicationInstanceVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetApplicationPackageFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR aplicationId,
                /* [in] */ LPCWSTR applicationRolloutVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetServicePackageFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR aplicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR servicePackageRolloutVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetServiceManifestFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR serviceManifestVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetServiceManifestChecksumFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR serviceManifestVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR codePackageName,
                /* [in] */ LPCWSTR codePackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageChecksumFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR codePackageName,
                /* [in] */ LPCWSTR codePackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetConfigPackageFolder( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR configPackageName,
                /* [in] */ LPCWSTR configPackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetConfigPackageChecksumFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR configPackageName,
                /* [in] */ LPCWSTR configPackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetDataPackageFolder( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR dataPackageName,
                /* [in] */ LPCWSTR dataPackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetDataPackageChecksumFile( 
                /* [in] */ LPCWSTR applicationTypeName,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR dataPackageName,
                /* [in] */ LPCWSTR dataPackageVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetSettingsFile( 
                /* [in] */ LPCWSTR configPackageFolder,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetSubPackageArchiveFile( 
                /* [in] */ LPCWSTR packageFolder,
                /* [retval][out] */ IFabricStringResult **result);

        private:
            StoreLayoutSpecification storeLayout_;
        };
    }
}
