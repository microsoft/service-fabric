// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TestCommon
    {
        class TestComCodePackageActivationContext 
            : public KObject<TestComCodePackageActivationContext>
            , public KShared<TestComCodePackageActivationContext>
            , public IFabricCodePackageActivationContext
            , public IFabricTransactionalReplicatorRuntimeConfigurations
        {
            K_FORCE_SHARED(TestComCodePackageActivationContext)

            K_BEGIN_COM_INTERFACE_LIST(TestComCodePackageActivationContext)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricCodePackageActivationContext)
                COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext, IFabricCodePackageActivationContext)
                COM_INTERFACE_ITEM(IID_IFabricTransactionalReplicatorRuntimeConfigurations, IFabricTransactionalReplicatorRuntimeConfigurations)
            K_END_COM_INTERFACE_LIST()

        public:

            static SPtr Create(
                __in LPCWSTR workingDirectory,
                __in KAllocator & allocator);

            LPCWSTR STDMETHODCALLTYPE get_ContextId(void);

            LPCWSTR STDMETHODCALLTYPE get_CodePackageName(void);

            LPCWSTR STDMETHODCALLTYPE get_CodePackageVersion(void);

            LPCWSTR STDMETHODCALLTYPE get_WorkDirectory(void);

            LPCWSTR STDMETHODCALLTYPE get_LogDirectory(void);

            LPCWSTR STDMETHODCALLTYPE get_TempDirectory(void);

            const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * STDMETHODCALLTYPE get_ServiceEndpointResources(void);

            const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST * STDMETHODCALLTYPE get_ServiceTypes(void);

            const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST * STDMETHODCALLTYPE get_ServiceGroupTypes(void);

            const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION * STDMETHODCALLTYPE get_ApplicationPrincipals(void);

            HRESULT STDMETHODCALLTYPE GetCodePackageNames(
                /*[out, retval]*/ IFabricStringListResult ** names);

            HRESULT STDMETHODCALLTYPE GetDataPackageNames(
                /*[out, retval]*/ IFabricStringListResult ** names);

            HRESULT STDMETHODCALLTYPE GetConfigurationPackageNames(
                /*[out, retval]*/ IFabricStringListResult ** names);

            HRESULT STDMETHODCALLTYPE GetCodePackage( 
                /* [in] */ LPCWSTR codePackageName,
                /* [retval][out] */ IFabricCodePackage **codePackage);

            HRESULT STDMETHODCALLTYPE GetConfigurationPackage( 
                /* [in] */ LPCWSTR configPackageName,
                /* [retval][out] */ IFabricConfigurationPackage **configPackage);

            HRESULT STDMETHODCALLTYPE GetDataPackage( 
                /* [in] */ LPCWSTR dataPackageName,
                /* [retval][out] */ IFabricDataPackage **dataPackage);

            HRESULT STDMETHODCALLTYPE GetServiceEndpointResource(
                /*[in]*/ LPCWSTR serviceEndpointResourceName,
                /*[out, retval]*/ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION ** bufferedValue);

            HRESULT STDMETHODCALLTYPE RegisterCodePackageChangeHandler(
                /* [in] */ IFabricCodePackageChangeHandler * callback,
                /* [out, retval] */ LONGLONG * callbackHandle);

            HRESULT STDMETHODCALLTYPE UnregisterCodePackageChangeHandler(
                /* [in] */ LONGLONG callbackHandle);
                
            HRESULT STDMETHODCALLTYPE RegisterConfigurationPackageChangeHandler(
                /* [in] */ IFabricConfigurationPackageChangeHandler * callback,
                /* [out, retval] */ LONGLONG * callbackHandle);

            HRESULT STDMETHODCALLTYPE UnregisterConfigurationPackageChangeHandler(
                /* [in] */ LONGLONG callbackHandle);

            HRESULT STDMETHODCALLTYPE RegisterDataPackageChangeHandler(
                /* [in] */ IFabricDataPackageChangeHandler * callback,
                /* [out, retval] */ LONGLONG * callbackHandle);

            HRESULT STDMETHODCALLTYPE UnregisterDataPackageChangeHandler(
                /* [in] */ LONGLONG callbackHandle);

        private: // Constructor
            TestComCodePackageActivationContext(__in LPCWSTR workingDirectory);

        private: // Variables
            KString::SPtr workDirectory_;
        };
    }
}

