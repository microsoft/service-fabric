// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ComCodePackageActivationContext :
        public ITentativeCodePackageActivationContext,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComCodePackageActivationContext);

        BEGIN_COM_INTERFACE_LIST(ComCodePackageActivationContext)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricCodePackageActivationContext)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext, IFabricCodePackageActivationContext)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext2, IFabricCodePackageActivationContext2)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext3, IFabricCodePackageActivationContext3)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext4, IFabricCodePackageActivationContext4)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext5, IFabricCodePackageActivationContext5)
            COM_INTERFACE_ITEM(IID_IFabricCodePackageActivationContext6, IFabricCodePackageActivationContext6)
        END_COM_INTERFACE_LIST()

    public:
        ComCodePackageActivationContext(CodePackageActivationContextSPtr const & codePackageActivationContextSPtr);
        virtual ~ComCodePackageActivationContext();

        __declspec(property(get = get_ActivationContext)) CodePackageActivationContextSPtr const & ActivationContext;
        CodePackageActivationContextSPtr const & get_ActivationContext() const { return codePackageActivationContextSPtr_; }

        virtual ULONG STDMETHODCALLTYPE TryAddRef();

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

        FABRIC_URI STDMETHODCALLTYPE get_ApplicationName(void);

        LPCWSTR STDMETHODCALLTYPE get_ApplicationTypeName(void);

        HRESULT STDMETHODCALLTYPE GetServiceManifestName(
            /* [retval][out] */ IFabricStringResult **serviceManifestName);

        HRESULT STDMETHODCALLTYPE GetServiceManifestVersion(
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);

        //
        // IFabricCodePackageActivationContext3
        //
        HRESULT STDMETHODCALLTYPE ReportApplicationHealth(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation);

        HRESULT STDMETHODCALLTYPE ReportDeployedApplicationHealth(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation);

        HRESULT STDMETHODCALLTYPE ReportDeployedServicePackageHealth(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation);

        //
        // IFabricCodePackageActivationContext4
        //
        HRESULT STDMETHODCALLTYPE ReportApplicationHealth2(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation,
            /* [in] */ FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);

        HRESULT STDMETHODCALLTYPE ReportDeployedApplicationHealth2(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation,
            /* [in] */ FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);

        HRESULT STDMETHODCALLTYPE ReportDeployedServicePackageHealth2(
            /* [in] */ FABRIC_HEALTH_INFORMATION const *healthInformation,
            /* [in] */ FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);

        //
        // IFabricCodePackageActivationContext5
        //

        LPCWSTR STDMETHODCALLTYPE get_ServiceListenAddress(void);

        LPCWSTR STDMETHODCALLTYPE get_ServicePublishAddress(void);

        //
        // IFabricCodePackageActivationContext6
        //

        HRESULT STDMETHODCALLTYPE GetDirectory(
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [out, retval] */ IFabricStringResult **directoryPath);

        __declspec(property(get = get_Test_CodePackageActivationContextObj)) CodePackageActivationContext & Test_CodePackageActivationContextObj;
        CodePackageActivationContext & get_Test_CodePackageActivationContextObj() { return *codePackageActivationContextSPtr_; }

    private:
        CodePackageActivationContextSPtr const codePackageActivationContextSPtr_;
        std::wstring id_;
    };
}
