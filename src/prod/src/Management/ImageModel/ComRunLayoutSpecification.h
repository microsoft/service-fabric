// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComRunLayoutSpecification : 
            public IFabricRunLayoutSpecification2,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComRunLayoutSpecification)

            BEGIN_COM_INTERFACE_LIST(ComRunLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricRunLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricRunLayoutSpecification, IFabricRunLayoutSpecification)
                COM_INTERFACE_ITEM(IID_IFabricRunLayoutSpecification2, IFabricRunLayoutSpecification2)
            END_COM_INTERFACE_LIST()

        public:
            static HRESULT FabricCreateRunLayoutSpecification( 
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **runLayoutSpecification);

            ComRunLayoutSpecification();
            ~ComRunLayoutSpecification();

            HRESULT STDMETHODCALLTYPE SetRoot( 
                /* [in] */ LPCWSTR value);
        
            HRESULT STDMETHODCALLTYPE GetRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetApplicationFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetApplicationWorkFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetApplicationLogFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [retval][out] */ IFabricStringResult **result);
            
            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetApplicationTempFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetApplicationPackageFile( 
                /* [in] */ LPCWSTR aplicationId,
                /* [in] */ LPCWSTR applicationRolloutVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetServicePackageFile( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR servicePackageRolloutVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetCurrentServicePackageFile( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetCurrentServicePackageFile2(
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR servicePackageActivationId,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetEndpointDescriptionsFile( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetEndpointDescriptionsFile2(
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR servicePackageActivationId,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetServiceManifestFile( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR serviceManifestName,
                /* [in] */ LPCWSTR serviceManifestVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetCodePackageFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR codePackageName,
                /* [in] */ LPCWSTR codePackageVersion,
                /* [in] */ BOOLEAN isSharedPackage,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetConfigPackageFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR configPackageName,
                /* [in] */ LPCWSTR configPackageVersion,
                /* [in] */ BOOLEAN isSharedPackage,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetDataPackageFolder( 
                /* [in] */ LPCWSTR applicationId,
                /* [in] */ LPCWSTR servicePackageName,
                /* [in] */ LPCWSTR dataPackageName,
                /* [in] */ LPCWSTR dataPackageVersion,
                /* [in] */ BOOLEAN isSharedPackage,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE STDMETHODCALLTYPE GetSettingsFile( 
                /* [in] */ LPCWSTR configPackageFolder,
                /* [retval][out] */ IFabricStringResult **result);

        private:
            RunLayoutSpecification runLayout_;
        };
    }
}
