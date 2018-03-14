// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ComFabricDeploymentSpecification : 
            public IFabricDeploymentSpecification,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComFabricDeploymentSpecification)

                COM_INTERFACE_LIST1(
                ComFabricDeploymentSpecification,
                IID_IFabricDeploymentSpecification,
                IFabricDeploymentSpecification)

        public:
            static HRESULT FabricCreateFabricDeploymentSpecification( 
                /* [in] */ REFIID riid,
                /* [retval][out] */ void **fabricDeploymentSpecification);

            ComFabricDeploymentSpecification();
            ~ComFabricDeploymentSpecification();

            HRESULT STDMETHODCALLTYPE SetDataRoot( 
                /* [in] */ LPCWSTR value);

            HRESULT STDMETHODCALLTYPE GetDataRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE SetLogRoot( 
                /* [in] */ LPCWSTR value);

            HRESULT STDMETHODCALLTYPE GetLogRoot( 
                /* [retval][out] */ IFabricStringResult **value);

            HRESULT STDMETHODCALLTYPE GetLogFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetTracesFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCrashDumpsFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetApplicationCrashDumpsFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetAppInstanceDataFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetPerformanceCountersBinaryFolder(                 
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetTargetInformationFile(
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetNodeFolder( 
                /* [in] */ LPCWSTR nodeName,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetFabricFolder( 
                /* [in] */ LPCWSTR nodeName,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCurrentClusterManifestFile( 
                /* [in] */ LPCWSTR nodeName,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetVersionedClusterManifestFile( 
                /* [in] */ LPCWSTR nodeName,   
                /* [in] */ LPCWSTR version,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetInstallerScriptFile( 
                /* [in] */ LPCWSTR nodeName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetInstallerLogFile( 
                /* [in] */ LPCWSTR nodeName,
                /* [in] */ LPCWSTR codeVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetInfrastructureManfiestFile(
                /* [in] */ LPCWSTR nodeName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetConfigurationDeploymentFolder(
                /* [in] */ LPCWSTR nodeName,
                /* [in] */ LPCWSTR configVersion,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetDataDeploymentFolder(
                /* [in] */ LPCWSTR nodeName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCodeDeploymentFolder(
                /* [in] */ LPCWSTR nodeName,
                /* [in] */ LPCWSTR service,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetInstalledBinaryFolder(
                /* [in] */ LPCWSTR installationFolder,
                /* [in] */ LPCWSTR service,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetWorkFolder(
                /* [in] */ LPCWSTR nodeName,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetCurrentFabricPackageFile( 
                /* [in] */ LPCWSTR nodeName,   
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetVersionedFabricPackageFile( 
                /* [in] */ LPCWSTR nodeName,   
                /* [in] */ LPCWSTR version,
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetQueryTraceFolder(
                /* [retval][out] */ IFabricStringResult **result);

            HRESULT STDMETHODCALLTYPE GetEnableCircularTraceSession(
                /* [retval][out] */ BOOLEAN *result);

            HRESULT STDMETHODCALLTYPE SetEnableCircularTraceSession(
                /* [in] */ BOOLEAN value);

        private:
            FabricDeploymentSpecification fabricDeployment_;
        };
    }
}
;
