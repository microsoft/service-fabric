// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComWinFabRunLayoutSpecification : 
            public IFabricWinFabRunLayoutSpecification,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComWinFabRunLayoutSpecification)

                COM_INTERFACE_LIST1(
                ComWinFabRunLayoutSpecification,
                IID_IFabricWinFabRunLayoutSpecification,
                IFabricWinFabRunLayoutSpecification)

        public:
            static HRESULT FabricCreateWinFabRunLayoutSpecification( 
                /* [in] */ REFIID riid,
                /* [retval][out] */ void **winFabRunLayoutSpecification);

            ComWinFabRunLayoutSpecification();
            ~ComWinFabRunLayoutSpecification();

            HRESULT STDMETHODCALLTYPE SetRoot( 
                /* [in] */ LPCWSTR value);
        
            HRESULT STDMETHODCALLTYPE GetRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE GetPatchFile( 
                /* [in] */ LPCWSTR codeVersion,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCabPatchFile(
                /* [in] */ LPCWSTR codeVersion,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodePackageFolder(
                /* [in] */ LPCWSTR codeVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetClusterManifestFile( 
                /* [in] */ LPCWSTR clusterManifestVersion,   
                /* [retval][out] */ IFabricStringResult **result);            
           
        private:
            WinFabRunLayoutSpecification winFabRunLayout_;
        };
    }
}
