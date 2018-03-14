// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComWinFabStoreLayoutSpecification : 
            public IFabricWinFabStoreLayoutSpecification,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComWinFabStoreLayoutSpecification)

                COM_INTERFACE_LIST1(
                ComWinFabStoreLayoutSpecification,
                IID_IFabricWinFabStoreLayoutSpecification,
                IFabricWinFabStoreLayoutSpecification)

        public:
            static HRESULT FabricCreateWinFabStoreLayoutSpecification( 
                /* [in] */ REFIID riid,
                /* [retval][out] */ void **winFabStoreLayoutSpecification);

            ComWinFabStoreLayoutSpecification();
            ~ComWinFabStoreLayoutSpecification();

            HRESULT STDMETHODCALLTYPE SetRoot( 
                /* [in] */ LPCWSTR value);
        
            HRESULT STDMETHODCALLTYPE GetRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE GetPatchFile( 
                /* [in] */ LPCWSTR version,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCabPatchFile(
                /* [in] */ LPCWSTR version,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageFolder(
                /* [in] */ LPCWSTR version,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetClusterManifestFile( 
                /* [in] */ LPCWSTR clusterManifestVersion,   
                /* [retval][out] */ IFabricStringResult **result);
            
           
        private:
            WinFabStoreLayoutSpecification winFabLayout_;
        };
    }
}
