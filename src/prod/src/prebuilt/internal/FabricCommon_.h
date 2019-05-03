

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __fabriccommon__h__
#define __fabriccommon__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricDeploymentSpecification_FWD_DEFINED__
#define __IFabricDeploymentSpecification_FWD_DEFINED__
typedef interface IFabricDeploymentSpecification IFabricDeploymentSpecification;

#endif 	/* __IFabricDeploymentSpecification_FWD_DEFINED__ */


#ifndef __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__
#define __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__
typedef interface IFabricWinFabStoreLayoutSpecification IFabricWinFabStoreLayoutSpecification;

#endif 	/* __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__
#define __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__
typedef interface IFabricWinFabRunLayoutSpecification IFabricWinFabRunLayoutSpecification;

#endif 	/* __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification_FWD_DEFINED__
#define __IFabricBuildLayoutSpecification_FWD_DEFINED__
typedef interface IFabricBuildLayoutSpecification IFabricBuildLayoutSpecification;

#endif 	/* __IFabricBuildLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification2_FWD_DEFINED__
#define __IFabricBuildLayoutSpecification2_FWD_DEFINED__
typedef interface IFabricBuildLayoutSpecification2 IFabricBuildLayoutSpecification2;

#endif 	/* __IFabricBuildLayoutSpecification2_FWD_DEFINED__ */


#ifndef __IFabricStoreLayoutSpecification_FWD_DEFINED__
#define __IFabricStoreLayoutSpecification_FWD_DEFINED__
typedef interface IFabricStoreLayoutSpecification IFabricStoreLayoutSpecification;

#endif 	/* __IFabricStoreLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricRunLayoutSpecification_FWD_DEFINED__
#define __IFabricRunLayoutSpecification_FWD_DEFINED__
typedef interface IFabricRunLayoutSpecification IFabricRunLayoutSpecification;

#endif 	/* __IFabricRunLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricConfigStore_FWD_DEFINED__
#define __IFabricConfigStore_FWD_DEFINED__
typedef interface IFabricConfigStore IFabricConfigStore;

#endif 	/* __IFabricConfigStore_FWD_DEFINED__ */


#ifndef __IFabricConfigStore2_FWD_DEFINED__
#define __IFabricConfigStore2_FWD_DEFINED__
typedef interface IFabricConfigStore2 IFabricConfigStore2;

#endif 	/* __IFabricConfigStore2_FWD_DEFINED__ */


#ifndef __IFabricConfigStoreUpdateHandler_FWD_DEFINED__
#define __IFabricConfigStoreUpdateHandler_FWD_DEFINED__
typedef interface IFabricConfigStoreUpdateHandler IFabricConfigStoreUpdateHandler;

#endif 	/* __IFabricConfigStoreUpdateHandler_FWD_DEFINED__ */


#ifndef __IFabricStringMapResult_FWD_DEFINED__
#define __IFabricStringMapResult_FWD_DEFINED__
typedef interface IFabricStringMapResult IFabricStringMapResult;

#endif 	/* __IFabricStringMapResult_FWD_DEFINED__ */


#ifndef __IFabricDeploymentSpecification_FWD_DEFINED__
#define __IFabricDeploymentSpecification_FWD_DEFINED__
typedef interface IFabricDeploymentSpecification IFabricDeploymentSpecification;

#endif 	/* __IFabricDeploymentSpecification_FWD_DEFINED__ */


#ifndef __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__
#define __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__
typedef interface IFabricWinFabStoreLayoutSpecification IFabricWinFabStoreLayoutSpecification;

#endif 	/* __IFabricWinFabStoreLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__
#define __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__
typedef interface IFabricWinFabRunLayoutSpecification IFabricWinFabRunLayoutSpecification;

#endif 	/* __IFabricWinFabRunLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification_FWD_DEFINED__
#define __IFabricBuildLayoutSpecification_FWD_DEFINED__
typedef interface IFabricBuildLayoutSpecification IFabricBuildLayoutSpecification;

#endif 	/* __IFabricBuildLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification2_FWD_DEFINED__
#define __IFabricBuildLayoutSpecification2_FWD_DEFINED__
typedef interface IFabricBuildLayoutSpecification2 IFabricBuildLayoutSpecification2;

#endif 	/* __IFabricBuildLayoutSpecification2_FWD_DEFINED__ */


#ifndef __IFabricStoreLayoutSpecification_FWD_DEFINED__
#define __IFabricStoreLayoutSpecification_FWD_DEFINED__
typedef interface IFabricStoreLayoutSpecification IFabricStoreLayoutSpecification;

#endif 	/* __IFabricStoreLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricStoreLayoutSpecification2_FWD_DEFINED__
#define __IFabricStoreLayoutSpecification2_FWD_DEFINED__
typedef interface IFabricStoreLayoutSpecification2 IFabricStoreLayoutSpecification2;

#endif 	/* __IFabricStoreLayoutSpecification2_FWD_DEFINED__ */


#ifndef __IFabricRunLayoutSpecification_FWD_DEFINED__
#define __IFabricRunLayoutSpecification_FWD_DEFINED__
typedef interface IFabricRunLayoutSpecification IFabricRunLayoutSpecification;

#endif 	/* __IFabricRunLayoutSpecification_FWD_DEFINED__ */


#ifndef __IFabricRunLayoutSpecification2_FWD_DEFINED__
#define __IFabricRunLayoutSpecification2_FWD_DEFINED__
typedef interface IFabricRunLayoutSpecification2 IFabricRunLayoutSpecification2;

#endif 	/* __IFabricRunLayoutSpecification2_FWD_DEFINED__ */


#ifndef __IFabricConfigStore_FWD_DEFINED__
#define __IFabricConfigStore_FWD_DEFINED__
typedef interface IFabricConfigStore IFabricConfigStore;

#endif 	/* __IFabricConfigStore_FWD_DEFINED__ */


#ifndef __IFabricConfigStore2_FWD_DEFINED__
#define __IFabricConfigStore2_FWD_DEFINED__
typedef interface IFabricConfigStore2 IFabricConfigStore2;

#endif 	/* __IFabricConfigStore2_FWD_DEFINED__ */


#ifndef __IFabricConfigStoreUpdateHandler_FWD_DEFINED__
#define __IFabricConfigStoreUpdateHandler_FWD_DEFINED__
typedef interface IFabricConfigStoreUpdateHandler IFabricConfigStoreUpdateHandler;

#endif 	/* __IFabricConfigStoreUpdateHandler_FWD_DEFINED__ */


#ifndef __IFabricStringMapResult_FWD_DEFINED__
#define __IFabricStringMapResult_FWD_DEFINED__
typedef interface IFabricStringMapResult IFabricStringMapResult;

#endif 	/* __IFabricStringMapResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabriccommon__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif













extern RPC_IF_HANDLE __MIDL_itf_fabriccommon__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabriccommon__0000_0000_v0_0_s_ifspec;


#ifndef __FabricCommonInternalLib_LIBRARY_DEFINED__
#define __FabricCommonInternalLib_LIBRARY_DEFINED__

/* library FabricCommonInternalLib */
/* [version][helpstring][uuid] */ 


#pragma pack(push, 8)
typedef struct FABRIC_SERVICE_MANIFEST_DESCRIPTION
    {
    LPCWSTR Name;
    LPCWSTR Version;
    const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *ServiceTypes;
    const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *ServiceGroupTypes;
    const FABRIC_CODE_PACKAGE_DESCRIPTION_LIST *CodePackages;
    const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION_LIST *ConfigurationPackages;
    const FABRIC_DATA_PACKAGE_DESCRIPTION_LIST *DataPackages;
    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *Endpoints;
    void *Reserved;
    } 	FABRIC_SERVICE_MANIFEST_DESCRIPTION;













#pragma pack(pop)

EXTERN_C const IID LIBID_FabricCommonInternalLib;

#ifndef __IFabricDeploymentSpecification_INTERFACE_DEFINED__
#define __IFabricDeploymentSpecification_INTERFACE_DEFINED__

/* interface IFabricDeploymentSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricDeploymentSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C36B827-0297-4355-B591-7EB7211548AE")
    IFabricDeploymentSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDataRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTracesFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCrashDumpsFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationCrashDumpsFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppInstanceDataFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPerformanceCountersBinaryFolder( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTargetInformationFile( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNodeFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFabricFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentClusterManifestFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersionedClusterManifestFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR version,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstallerScriptFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstallerLogFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInfrastructureManfiestFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigurationDeploymentFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR configVersion,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataDeploymentFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodeDeploymentFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR service,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstalledBinaryFolder( 
            /* [in] */ LPCWSTR installationFolder,
            /* [in] */ LPCWSTR service,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorkFolder( 
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentFabricPackageFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersionedFabricPackageFile( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR version,
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQueryTraceFolder( 
            /* [out][retval] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableCircularTraceSession( 
            /* [out][retval] */ BOOLEAN *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableCircularTraceSession( 
            /* [in] */ BOOLEAN value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricDeploymentSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricDeploymentSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricDeploymentSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDataRoot )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataRoot )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *SetLogRoot )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogRoot )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetTracesFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCrashDumpsFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationCrashDumpsFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetAppInstanceDataFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetPerformanceCountersBinaryFolder )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetTargetInformationFile )( 
            IFabricDeploymentSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetNodeFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetFabricFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentClusterManifestFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersionedClusterManifestFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR version,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetInstallerScriptFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetInstallerLogFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfrastructureManfiestFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationDeploymentFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR configVersion,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataDeploymentFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodeDeploymentFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR service,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetInstalledBinaryFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR installationFolder,
            /* [in] */ LPCWSTR service,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetWorkFolder )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentFabricPackageFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersionedFabricPackageFile )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR version,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetQueryTraceFolder )( 
            IFabricDeploymentSpecification * This,
            /* [out][retval] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableCircularTraceSession )( 
            IFabricDeploymentSpecification * This,
            /* [out][retval] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableCircularTraceSession )( 
            IFabricDeploymentSpecification * This,
            /* [in] */ BOOLEAN value);
        
        END_INTERFACE
    } IFabricDeploymentSpecificationVtbl;

    interface IFabricDeploymentSpecification
    {
        CONST_VTBL struct IFabricDeploymentSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricDeploymentSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricDeploymentSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricDeploymentSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricDeploymentSpecification_SetDataRoot(This,value)	\
    ( (This)->lpVtbl -> SetDataRoot(This,value) ) 

#define IFabricDeploymentSpecification_GetDataRoot(This,value)	\
    ( (This)->lpVtbl -> GetDataRoot(This,value) ) 

#define IFabricDeploymentSpecification_SetLogRoot(This,value)	\
    ( (This)->lpVtbl -> SetLogRoot(This,value) ) 

#define IFabricDeploymentSpecification_GetLogRoot(This,value)	\
    ( (This)->lpVtbl -> GetLogRoot(This,value) ) 

#define IFabricDeploymentSpecification_GetLogFolder(This,result)	\
    ( (This)->lpVtbl -> GetLogFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetTracesFolder(This,result)	\
    ( (This)->lpVtbl -> GetTracesFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetCrashDumpsFolder(This,result)	\
    ( (This)->lpVtbl -> GetCrashDumpsFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetApplicationCrashDumpsFolder(This,result)	\
    ( (This)->lpVtbl -> GetApplicationCrashDumpsFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetAppInstanceDataFolder(This,result)	\
    ( (This)->lpVtbl -> GetAppInstanceDataFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetPerformanceCountersBinaryFolder(This,result)	\
    ( (This)->lpVtbl -> GetPerformanceCountersBinaryFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetTargetInformationFile(This,result)	\
    ( (This)->lpVtbl -> GetTargetInformationFile(This,result) ) 

#define IFabricDeploymentSpecification_GetNodeFolder(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetNodeFolder(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetFabricFolder(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetFabricFolder(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetCurrentClusterManifestFile(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetCurrentClusterManifestFile(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetVersionedClusterManifestFile(This,nodeName,version,result)	\
    ( (This)->lpVtbl -> GetVersionedClusterManifestFile(This,nodeName,version,result) ) 

#define IFabricDeploymentSpecification_GetInstallerScriptFile(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetInstallerScriptFile(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetInstallerLogFile(This,nodeName,codeVersion,result)	\
    ( (This)->lpVtbl -> GetInstallerLogFile(This,nodeName,codeVersion,result) ) 

#define IFabricDeploymentSpecification_GetInfrastructureManfiestFile(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetInfrastructureManfiestFile(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetConfigurationDeploymentFolder(This,nodeName,configVersion,result)	\
    ( (This)->lpVtbl -> GetConfigurationDeploymentFolder(This,nodeName,configVersion,result) ) 

#define IFabricDeploymentSpecification_GetDataDeploymentFolder(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetDataDeploymentFolder(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetCodeDeploymentFolder(This,nodeName,service,result)	\
    ( (This)->lpVtbl -> GetCodeDeploymentFolder(This,nodeName,service,result) ) 

#define IFabricDeploymentSpecification_GetInstalledBinaryFolder(This,installationFolder,service,result)	\
    ( (This)->lpVtbl -> GetInstalledBinaryFolder(This,installationFolder,service,result) ) 

#define IFabricDeploymentSpecification_GetWorkFolder(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetWorkFolder(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetCurrentFabricPackageFile(This,nodeName,result)	\
    ( (This)->lpVtbl -> GetCurrentFabricPackageFile(This,nodeName,result) ) 

#define IFabricDeploymentSpecification_GetVersionedFabricPackageFile(This,nodeName,version,result)	\
    ( (This)->lpVtbl -> GetVersionedFabricPackageFile(This,nodeName,version,result) ) 

#define IFabricDeploymentSpecification_GetQueryTraceFolder(This,result)	\
    ( (This)->lpVtbl -> GetQueryTraceFolder(This,result) ) 

#define IFabricDeploymentSpecification_GetEnableCircularTraceSession(This,result)	\
    ( (This)->lpVtbl -> GetEnableCircularTraceSession(This,result) ) 

#define IFabricDeploymentSpecification_SetEnableCircularTraceSession(This,value)	\
    ( (This)->lpVtbl -> SetEnableCircularTraceSession(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricDeploymentSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricWinFabStoreLayoutSpecification_INTERFACE_DEFINED__
#define __IFabricWinFabStoreLayoutSpecification_INTERFACE_DEFINED__

/* interface IFabricWinFabStoreLayoutSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricWinFabStoreLayoutSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D07D4CB7-0E31-4930-8AC1-DC1A1062241D")
    IFabricWinFabStoreLayoutSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPatchFile( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCabPatchFile( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClusterManifestFile( 
            /* [in] */ LPCWSTR clusterManifestVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricWinFabStoreLayoutSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricWinFabStoreLayoutSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricWinFabStoreLayoutSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetPatchFile )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCabPatchFile )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetClusterManifestFile )( 
            IFabricWinFabStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR clusterManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricWinFabStoreLayoutSpecificationVtbl;

    interface IFabricWinFabStoreLayoutSpecification
    {
        CONST_VTBL struct IFabricWinFabStoreLayoutSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricWinFabStoreLayoutSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricWinFabStoreLayoutSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricWinFabStoreLayoutSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricWinFabStoreLayoutSpecification_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricWinFabStoreLayoutSpecification_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricWinFabStoreLayoutSpecification_GetPatchFile(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetPatchFile(This,codeVersion,result) ) 

#define IFabricWinFabStoreLayoutSpecification_GetCabPatchFile(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetCabPatchFile(This,codeVersion,result) ) 

#define IFabricWinFabStoreLayoutSpecification_GetCodePackageFolder(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,codeVersion,result) ) 

#define IFabricWinFabStoreLayoutSpecification_GetClusterManifestFile(This,clusterManifestVersion,result)	\
    ( (This)->lpVtbl -> GetClusterManifestFile(This,clusterManifestVersion,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricWinFabStoreLayoutSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricWinFabRunLayoutSpecification_INTERFACE_DEFINED__
#define __IFabricWinFabRunLayoutSpecification_INTERFACE_DEFINED__

/* interface IFabricWinFabRunLayoutSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricWinFabRunLayoutSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2B299756-A1DA-450D-88A0-11B7190EA3A0")
    IFabricWinFabRunLayoutSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPatchFile( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCabPatchFile( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClusterManifestFile( 
            /* [in] */ LPCWSTR clusterManifestVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricWinFabRunLayoutSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricWinFabRunLayoutSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricWinFabRunLayoutSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetPatchFile )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCabPatchFile )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ LPCWSTR codeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetClusterManifestFile )( 
            IFabricWinFabRunLayoutSpecification * This,
            /* [in] */ LPCWSTR clusterManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricWinFabRunLayoutSpecificationVtbl;

    interface IFabricWinFabRunLayoutSpecification
    {
        CONST_VTBL struct IFabricWinFabRunLayoutSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricWinFabRunLayoutSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricWinFabRunLayoutSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricWinFabRunLayoutSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricWinFabRunLayoutSpecification_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricWinFabRunLayoutSpecification_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricWinFabRunLayoutSpecification_GetPatchFile(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetPatchFile(This,codeVersion,result) ) 

#define IFabricWinFabRunLayoutSpecification_GetCabPatchFile(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetCabPatchFile(This,codeVersion,result) ) 

#define IFabricWinFabRunLayoutSpecification_GetCodePackageFolder(This,codeVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,codeVersion,result) ) 

#define IFabricWinFabRunLayoutSpecification_GetClusterManifestFile(This,clusterManifestVersion,result)	\
    ( (This)->lpVtbl -> GetClusterManifestFile(This,clusterManifestVersion,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricWinFabRunLayoutSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification_INTERFACE_DEFINED__
#define __IFabricBuildLayoutSpecification_INTERFACE_DEFINED__

/* interface IFabricBuildLayoutSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBuildLayoutSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a5344964-3542-41bd-a271-f2f21ac541a8")
    IFabricBuildLayoutSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationManifestFile( 
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestFile( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestChecksumFile( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageChecksumFile( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigPackageFolder( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigPackageChecksumFile( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageFolder( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageChecksumFile( 
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSettingsFile( 
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBuildLayoutSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBuildLayoutSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBuildLayoutSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricBuildLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationManifestFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestChecksumFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageChecksumFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageChecksumFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageChecksumFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricBuildLayoutSpecification * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricBuildLayoutSpecificationVtbl;

    interface IFabricBuildLayoutSpecification
    {
        CONST_VTBL struct IFabricBuildLayoutSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBuildLayoutSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBuildLayoutSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBuildLayoutSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBuildLayoutSpecification_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricBuildLayoutSpecification_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricBuildLayoutSpecification_GetApplicationManifestFile(This,result)	\
    ( (This)->lpVtbl -> GetApplicationManifestFile(This,result) ) 

#define IFabricBuildLayoutSpecification_GetServiceManifestFile(This,serviceManifestName,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,serviceManifestName,result) ) 

#define IFabricBuildLayoutSpecification_GetServiceManifestChecksumFile(This,serviceManifestName,result)	\
    ( (This)->lpVtbl -> GetServiceManifestChecksumFile(This,serviceManifestName,result) ) 

#define IFabricBuildLayoutSpecification_GetCodePackageFolder(This,serviceManifestName,codePackageName,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,serviceManifestName,codePackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetCodePackageChecksumFile(This,serviceManifestName,codePackageName,result)	\
    ( (This)->lpVtbl -> GetCodePackageChecksumFile(This,serviceManifestName,codePackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetConfigPackageFolder(This,serviceManifestName,configPackageName,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,serviceManifestName,configPackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetConfigPackageChecksumFile(This,serviceManifestName,configPackageName,result)	\
    ( (This)->lpVtbl -> GetConfigPackageChecksumFile(This,serviceManifestName,configPackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetDataPackageFolder(This,serviceManifestName,dataPackageName,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,serviceManifestName,dataPackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetDataPackageChecksumFile(This,serviceManifestName,dataPackageName,result)	\
    ( (This)->lpVtbl -> GetDataPackageChecksumFile(This,serviceManifestName,dataPackageName,result) ) 

#define IFabricBuildLayoutSpecification_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBuildLayoutSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricBuildLayoutSpecification2_INTERFACE_DEFINED__
#define __IFabricBuildLayoutSpecification2_INTERFACE_DEFINED__

/* interface IFabricBuildLayoutSpecification2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBuildLayoutSpecification2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("93f7ca0f-8be0-4601-92a9-ad864b4a798d")
    IFabricBuildLayoutSpecification2 : public IFabricBuildLayoutSpecification
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubPackageArchiveFile( 
            /* [in] */ LPCWSTR packageFolder,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChecksumFile( 
            /* [in] */ LPCWSTR fileOrDirectoryName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBuildLayoutSpecification2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBuildLayoutSpecification2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBuildLayoutSpecification2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationManifestFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestChecksumFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageChecksumFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageChecksumFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageChecksumFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubPackageArchiveFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR packageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetChecksumFile )( 
            IFabricBuildLayoutSpecification2 * This,
            /* [in] */ LPCWSTR fileOrDirectoryName,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricBuildLayoutSpecification2Vtbl;

    interface IFabricBuildLayoutSpecification2
    {
        CONST_VTBL struct IFabricBuildLayoutSpecification2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBuildLayoutSpecification2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBuildLayoutSpecification2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBuildLayoutSpecification2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBuildLayoutSpecification2_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricBuildLayoutSpecification2_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricBuildLayoutSpecification2_GetApplicationManifestFile(This,result)	\
    ( (This)->lpVtbl -> GetApplicationManifestFile(This,result) ) 

#define IFabricBuildLayoutSpecification2_GetServiceManifestFile(This,serviceManifestName,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,serviceManifestName,result) ) 

#define IFabricBuildLayoutSpecification2_GetServiceManifestChecksumFile(This,serviceManifestName,result)	\
    ( (This)->lpVtbl -> GetServiceManifestChecksumFile(This,serviceManifestName,result) ) 

#define IFabricBuildLayoutSpecification2_GetCodePackageFolder(This,serviceManifestName,codePackageName,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,serviceManifestName,codePackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetCodePackageChecksumFile(This,serviceManifestName,codePackageName,result)	\
    ( (This)->lpVtbl -> GetCodePackageChecksumFile(This,serviceManifestName,codePackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetConfigPackageFolder(This,serviceManifestName,configPackageName,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,serviceManifestName,configPackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetConfigPackageChecksumFile(This,serviceManifestName,configPackageName,result)	\
    ( (This)->lpVtbl -> GetConfigPackageChecksumFile(This,serviceManifestName,configPackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetDataPackageFolder(This,serviceManifestName,dataPackageName,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,serviceManifestName,dataPackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetDataPackageChecksumFile(This,serviceManifestName,dataPackageName,result)	\
    ( (This)->lpVtbl -> GetDataPackageChecksumFile(This,serviceManifestName,dataPackageName,result) ) 

#define IFabricBuildLayoutSpecification2_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 


#define IFabricBuildLayoutSpecification2_GetSubPackageArchiveFile(This,packageFolder,result)	\
    ( (This)->lpVtbl -> GetSubPackageArchiveFile(This,packageFolder,result) ) 

#define IFabricBuildLayoutSpecification2_GetChecksumFile(This,fileOrDirectoryName,result)	\
    ( (This)->lpVtbl -> GetChecksumFile(This,fileOrDirectoryName,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBuildLayoutSpecification2_INTERFACE_DEFINED__ */


#ifndef __IFabricStoreLayoutSpecification_INTERFACE_DEFINED__
#define __IFabricStoreLayoutSpecification_INTERFACE_DEFINED__

/* interface IFabricStoreLayoutSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStoreLayoutSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a4e94cf4-a47b-4767-853f-cbd5914641d6")
    IFabricStoreLayoutSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationManifestFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInstanceFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationInstanceVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationPackageFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServicePackageFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestChecksumFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageChecksumFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigPackageFolder( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigPackageChecksumFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageFolder( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageChecksumFile( 
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSettingsFile( 
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStoreLayoutSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStoreLayoutSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStoreLayoutSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricStoreLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationManifestFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInstanceFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationInstanceVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationPackageFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicePackageFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestChecksumFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageChecksumFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageChecksumFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageChecksumFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricStoreLayoutSpecification * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricStoreLayoutSpecificationVtbl;

    interface IFabricStoreLayoutSpecification
    {
        CONST_VTBL struct IFabricStoreLayoutSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStoreLayoutSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStoreLayoutSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStoreLayoutSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStoreLayoutSpecification_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricStoreLayoutSpecification_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricStoreLayoutSpecification_GetApplicationManifestFile(This,applicationTypeName,applicationTypeVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationManifestFile(This,applicationTypeName,applicationTypeVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetApplicationInstanceFile(This,applicationTypeName,aplicationId,applicationInstanceVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationInstanceFile(This,applicationTypeName,aplicationId,applicationInstanceVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetApplicationPackageFile(This,applicationTypeName,aplicationId,applicationRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationPackageFile(This,applicationTypeName,aplicationId,applicationRolloutVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetServicePackageFile(This,applicationTypeName,aplicationId,servicePackageName,servicePackageRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetServicePackageFile(This,applicationTypeName,aplicationId,servicePackageName,servicePackageRolloutVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetServiceManifestFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetServiceManifestChecksumFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestChecksumFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetCodePackageFolder(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetCodePackageChecksumFile(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageChecksumFile(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetConfigPackageFolder(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetConfigPackageChecksumFile(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result)	\
    ( (This)->lpVtbl -> GetConfigPackageChecksumFile(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetDataPackageFolder(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetDataPackageChecksumFile(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result)	\
    ( (This)->lpVtbl -> GetDataPackageChecksumFile(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStoreLayoutSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricRunLayoutSpecification_INTERFACE_DEFINED__
#define __IFabricRunLayoutSpecification_INTERFACE_DEFINED__

/* interface IFabricRunLayoutSpecification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricRunLayoutSpecification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("53636fde-84ca-4657-9927-1d22dc37297d")
    IFabricRunLayoutSpecification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRoot( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [retval][out] */ IFabricStringResult **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationWorkFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationLogFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationTempFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationPackageFile( 
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServicePackageFile( 
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentServicePackageFile( 
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEndpointDescriptionsFile( 
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestFile( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigPackageFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageFolder( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSettingsFile( 
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricRunLayoutSpecificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricRunLayoutSpecification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricRunLayoutSpecification * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricRunLayoutSpecification * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationWorkFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationLogFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationTempFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationPackageFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicePackageFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentServicePackageFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndpointDescriptionsFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricRunLayoutSpecification * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricRunLayoutSpecificationVtbl;

    interface IFabricRunLayoutSpecification
    {
        CONST_VTBL struct IFabricRunLayoutSpecificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricRunLayoutSpecification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricRunLayoutSpecification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricRunLayoutSpecification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricRunLayoutSpecification_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricRunLayoutSpecification_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricRunLayoutSpecification_GetApplicationFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification_GetApplicationWorkFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationWorkFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification_GetApplicationLogFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationLogFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification_GetApplicationTempFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationTempFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification_GetApplicationPackageFile(This,aplicationId,applicationRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationPackageFile(This,aplicationId,applicationRolloutVersion,result) ) 

#define IFabricRunLayoutSpecification_GetServicePackageFile(This,aplicationId,servicePackageName,servicePackageRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetServicePackageFile(This,aplicationId,servicePackageName,servicePackageRolloutVersion,result) ) 

#define IFabricRunLayoutSpecification_GetCurrentServicePackageFile(This,aplicationId,servicePackageName,result)	\
    ( (This)->lpVtbl -> GetCurrentServicePackageFile(This,aplicationId,servicePackageName,result) ) 

#define IFabricRunLayoutSpecification_GetEndpointDescriptionsFile(This,aplicationId,servicePackageName,result)	\
    ( (This)->lpVtbl -> GetEndpointDescriptionsFile(This,aplicationId,servicePackageName,result) ) 

#define IFabricRunLayoutSpecification_GetServiceManifestFile(This,applicationId,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,applicationId,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricRunLayoutSpecification_GetCodePackageFolder(This,applicationId,servicePackageName,codePackageName,codePackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,applicationId,servicePackageName,codePackageName,codePackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification_GetConfigPackageFolder(This,applicationId,servicePackageName,configPackageName,configPackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,applicationId,servicePackageName,configPackageName,configPackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification_GetDataPackageFolder(This,applicationId,servicePackageName,dataPackageName,dataPackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,applicationId,servicePackageName,dataPackageName,dataPackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricRunLayoutSpecification_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigStore_INTERFACE_DEFINED__
#define __IFabricConfigStore_INTERFACE_DEFINED__

/* interface IFabricConfigStore */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("edd6091d-5230-4064-a731-5d2e6bac3436")
    IFabricConfigStore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSections( 
            /* [in] */ LPCWSTR partialSectionName,
            /* [retval][out] */ IFabricStringListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeys( 
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR partialKeyName,
            /* [retval][out] */ IFabricStringListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadString( 
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigStore * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSections )( 
            IFabricConfigStore * This,
            /* [in] */ LPCWSTR partialSectionName,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeys )( 
            IFabricConfigStore * This,
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR partialKeyName,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *ReadString )( 
            IFabricConfigStore * This,
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricConfigStoreVtbl;

    interface IFabricConfigStore
    {
        CONST_VTBL struct IFabricConfigStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigStore_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigStore_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigStore_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigStore_GetSections(This,partialSectionName,result)	\
    ( (This)->lpVtbl -> GetSections(This,partialSectionName,result) ) 

#define IFabricConfigStore_GetKeys(This,sectionName,partialKeyName,result)	\
    ( (This)->lpVtbl -> GetKeys(This,sectionName,partialKeyName,result) ) 

#define IFabricConfigStore_ReadString(This,section,key,isEncrypted,result)	\
    ( (This)->lpVtbl -> ReadString(This,section,key,isEncrypted,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigStore_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigStore2_INTERFACE_DEFINED__
#define __IFabricConfigStore2_INTERFACE_DEFINED__

/* interface IFabricConfigStore2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigStore2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c8beea34-1f9d-4d3b-970d-f26ca0e4a762")
    IFabricConfigStore2 : public IFabricConfigStore
    {
    public:
        virtual BOOLEAN STDMETHODCALLTYPE get_IgnoreUpdateFailures( void) = 0;
        
        virtual void STDMETHODCALLTYPE set_IgnoreUpdateFailures( 
            BOOLEAN value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigStore2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigStore2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigStore2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigStore2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSections )( 
            IFabricConfigStore2 * This,
            /* [in] */ LPCWSTR partialSectionName,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetKeys )( 
            IFabricConfigStore2 * This,
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR partialKeyName,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *ReadString )( 
            IFabricConfigStore2 * This,
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ IFabricStringResult **result);
        
        BOOLEAN ( STDMETHODCALLTYPE *get_IgnoreUpdateFailures )( 
            IFabricConfigStore2 * This);
        
        void ( STDMETHODCALLTYPE *set_IgnoreUpdateFailures )( 
            IFabricConfigStore2 * This,
            BOOLEAN value);
        
        END_INTERFACE
    } IFabricConfigStore2Vtbl;

    interface IFabricConfigStore2
    {
        CONST_VTBL struct IFabricConfigStore2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigStore2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigStore2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigStore2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigStore2_GetSections(This,partialSectionName,result)	\
    ( (This)->lpVtbl -> GetSections(This,partialSectionName,result) ) 

#define IFabricConfigStore2_GetKeys(This,sectionName,partialKeyName,result)	\
    ( (This)->lpVtbl -> GetKeys(This,sectionName,partialKeyName,result) ) 

#define IFabricConfigStore2_ReadString(This,section,key,isEncrypted,result)	\
    ( (This)->lpVtbl -> ReadString(This,section,key,isEncrypted,result) ) 


#define IFabricConfigStore2_get_IgnoreUpdateFailures(This)	\
    ( (This)->lpVtbl -> get_IgnoreUpdateFailures(This) ) 

#define IFabricConfigStore2_set_IgnoreUpdateFailures(This,value)	\
    ( (This)->lpVtbl -> set_IgnoreUpdateFailures(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigStore2_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigStoreUpdateHandler_INTERFACE_DEFINED__
#define __IFabricConfigStoreUpdateHandler_INTERFACE_DEFINED__

/* interface IFabricConfigStoreUpdateHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigStoreUpdateHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("792d2f8d-15bd-449f-a607-002cb6004709")
    IFabricConfigStoreUpdateHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUpdate( 
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *isProcessed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CheckUpdate( 
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [in] */ LPCWSTR value,
            /* [in] */ BOOLEAN isEncrypted,
            /* [retval][out] */ BOOLEAN *canUpdate) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigStoreUpdateHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigStoreUpdateHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigStoreUpdateHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigStoreUpdateHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdate )( 
            IFabricConfigStoreUpdateHandler * This,
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *isProcessed);
        
        HRESULT ( STDMETHODCALLTYPE *CheckUpdate )( 
            IFabricConfigStoreUpdateHandler * This,
            /* [in] */ LPCWSTR section,
            /* [in] */ LPCWSTR key,
            /* [in] */ LPCWSTR value,
            /* [in] */ BOOLEAN isEncrypted,
            /* [retval][out] */ BOOLEAN *canUpdate);
        
        END_INTERFACE
    } IFabricConfigStoreUpdateHandlerVtbl;

    interface IFabricConfigStoreUpdateHandler
    {
        CONST_VTBL struct IFabricConfigStoreUpdateHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigStoreUpdateHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigStoreUpdateHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigStoreUpdateHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigStoreUpdateHandler_OnUpdate(This,section,key,isProcessed)	\
    ( (This)->lpVtbl -> OnUpdate(This,section,key,isProcessed) ) 

#define IFabricConfigStoreUpdateHandler_CheckUpdate(This,section,key,value,isEncrypted,canUpdate)	\
    ( (This)->lpVtbl -> CheckUpdate(This,section,key,value,isEncrypted,canUpdate) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigStoreUpdateHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricStringMapResult_INTERFACE_DEFINED__
#define __IFabricStringMapResult_INTERFACE_DEFINED__

/* interface IFabricStringMapResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStringMapResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ce7ebe69-f61b-4d62-ba2c-59cf07030206")
    IFabricStringMapResult : public IUnknown
    {
    public:
        virtual FABRIC_STRING_PAIR_LIST *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStringMapResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStringMapResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStringMapResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStringMapResult * This);
        
        FABRIC_STRING_PAIR_LIST *( STDMETHODCALLTYPE *get_Result )( 
            IFabricStringMapResult * This);
        
        END_INTERFACE
    } IFabricStringMapResultVtbl;

    interface IFabricStringMapResult
    {
        CONST_VTBL struct IFabricStringMapResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStringMapResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStringMapResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStringMapResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStringMapResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStringMapResult_INTERFACE_DEFINED__ */



#ifndef __FabricCommonInternalModule_MODULE_DEFINED__
#define __FabricCommonInternalModule_MODULE_DEFINED__


/* module FabricCommonInternalModule */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricCreateFabricDeploymentSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricDeploymentSpecification);

/* [entry] */ HRESULT FabricCreateBuildLayoutSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **buildLayoutSpecification);

/* [entry] */ HRESULT FabricCreateStoreLayoutSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **storeLayoutSpecification);

/* [entry] */ HRESULT FabricCreateRunLayoutSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **runLayoutSpecification);

/* [entry] */ HRESULT FabricCreateWinFabStoreLayoutSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **winFabStoreLayoutSpecification);

/* [entry] */ HRESULT FabricCreateWinFabRunLayoutSpecification( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **winFabRunLayoutSpecification);

/* [entry] */ HRESULT FabricGetConfigStore( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in_opt IFabricConfigStoreUpdateHandler *updateHandler,
    /* [retval][out] */ __RPC__deref_out_opt void **configStore);

/* [entry] */ HRESULT FabricGetConfigStoreEnvironmentVariable( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **envVariableValue);

/* [entry] */ LONG FabricGetThreadCount( void);

/* [entry] */ void FabricSetThreadTestLimit( 
    LONG __MIDL__FabricCommonInternalModule0000);

/* [entry] */ LPCSTR FabricSymInitializeOnce( void);

/* [entry] */ HRESULT FabricDecryptValue( 
    /* [in] */ __RPC__in LPCWSTR encryptedValue,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **decryptedValue);

/* [entry] */ HRESULT FabricIsValidExpression( 
    /* [in] */ __RPC__in LPCWSTR expression,
    /* [retval][out] */ __RPC__out BOOLEAN *isValid);

/* [entry] */ HRESULT GetLinuxPackageManagerType( 
    /* [out] */ __RPC__out INT32 *packageManagerType);

/* [entry] */ HRESULT FabricGetRoot2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricRoot);

/* [entry] */ HRESULT FabricGetRoot( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricRoot);

/* [entry] */ HRESULT FabricGetBinRoot( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricBinRoot);

/* [entry] */ HRESULT FabricGetBinRoot2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricBinRoot);

/* [entry] */ HRESULT FabricGetCodePath2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricCodePath);

/* [entry] */ HRESULT FabricGetCodePath( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricCodePath);

/* [entry] */ HRESULT FabricGetDataRoot2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricDataRoot);

/* [entry] */ HRESULT FabricGetDataRoot( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricDataRoot);

/* [entry] */ HRESULT FabricGetLogRoot2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricLogRoot);

/* [entry] */ HRESULT FabricGetLogRoot( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **fabricLogRoot);

/* [entry] */ HRESULT FabricSetRoot2( 
    /* [in] */ __RPC__in LPCWSTR fabricRoot,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetRoot( 
    /* [in] */ __RPC__in LPCWSTR fabricRoot);

/* [entry] */ HRESULT FabricSetBinRoot2( 
    /* [in] */ __RPC__in LPCWSTR binRoot,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetBinRoot( 
    /* [in] */ __RPC__in LPCWSTR binRoot);

/* [entry] */ HRESULT FabricSetCodePath2( 
    /* [in] */ __RPC__in LPCWSTR codePath,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetCodePath( 
    /* [in] */ __RPC__in LPCWSTR codePath);

/* [entry] */ HRESULT FabricSetDataRoot2( 
    /* [in] */ __RPC__in LPCWSTR dataRoot,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetDataRoot( 
    /* [in] */ __RPC__in LPCWSTR dataRoot);

/* [entry] */ HRESULT FabricSetLogRoot2( 
    /* [in] */ __RPC__in LPCWSTR logRoot,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetLogRoot( 
    /* [in] */ __RPC__in LPCWSTR logRoot);

/* [entry] */ HRESULT FabricGetCommonDllVersion( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **commonDllVersion);

/* [entry] */ HRESULT FabricDirectoryCreate( 
    /* [in] */ __RPC__in LPCWSTR path);

/* [entry] */ HRESULT FabricDirectoryGetDirectories( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ __RPC__in LPCWSTR pattern,
    /* [in] */ BOOLEAN getFullPath,
    /* [in] */ BOOLEAN topDirectoryOnly,
    /* [out] */ __RPC__deref_out_opt IFabricStringListResult **result);

/* [entry] */ HRESULT FabricDirectoryGetFiles( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ __RPC__in LPCWSTR pattern,
    /* [in] */ BOOLEAN getFullPath,
    /* [in] */ BOOLEAN topDirectoryOnly,
    /* [out] */ __RPC__deref_out_opt IFabricStringListResult **result);

/* [entry] */ HRESULT FabricDirectoryCopy( 
    /* [in] */ __RPC__in LPCWSTR src,
    /* [in] */ __RPC__in LPCWSTR des,
    /* [in] */ BOOLEAN overwrite);

/* [entry] */ HRESULT FabricDirectoryRename( 
    /* [in] */ __RPC__in LPCWSTR src,
    /* [in] */ __RPC__in LPCWSTR des,
    /* [in] */ BOOLEAN overwrite);

/* [entry] */ HRESULT FabricDirectoryExists( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__out BOOLEAN *isExisted);

/* [entry] */ HRESULT FabricDirectoryDelete( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ BOOLEAN recursive,
    /* [in] */ BOOLEAN deleteReadOnlyFiles);

/* [entry] */ HRESULT FabricDirectoryIsSymbolicLink( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__out BOOLEAN *result);

/* [entry] */ HRESULT FabricGetDirectoryName( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **result);

/* [entry] */ HRESULT FabricGetFullPath( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **result);

/* [entry] */ HRESULT FabricSetLastErrorMessage( 
    /* [in] */ __RPC__in LPCWSTR message,
    /* [out] */ __RPC__out DWORD *threadId);

/* [entry] */ HRESULT FabricClearLastErrorMessage( 
    /* [in] */ DWORD threadId);

/* [entry] */ HRESULT FabricFileOpen( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ FABRIC_FILE_MODE fileMode,
    /* [in] */ FABRIC_FILE_ACCESS fileAccess,
    /* [in] */ FABRIC_FILE_SHARE fileShare,
    /* [out] */ __RPC__deref_out_opt HANDLE *fileHandle);

/* [entry] */ HRESULT FabricFileOpenEx( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ FABRIC_FILE_MODE fileMode,
    /* [in] */ FABRIC_FILE_ACCESS fileAccess,
    /* [in] */ FABRIC_FILE_SHARE fileShare,
    /* [in] */ FABRIC_FILE_ATTRIBUTES fileAttributes,
    /* [out] */ __RPC__deref_out_opt HANDLE *fileHandle);

/* [entry] */ HRESULT FabricFileCopy( 
    /* [in] */ __RPC__in LPCWSTR src,
    /* [in] */ __RPC__in LPCWSTR des,
    /* [in] */ BOOLEAN overwrite);

/* [entry] */ HRESULT FabricFileMove( 
    /* [in] */ __RPC__in LPCWSTR src,
    /* [in] */ __RPC__in LPCWSTR des);

/* [entry] */ HRESULT FabricFileExists( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__out BOOLEAN *isExisted);

/* [entry] */ HRESULT FabricFileDelete( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [in] */ BOOLEAN deleteReadonly);

/* [entry] */ HRESULT FabricFileReplace( 
    /* [in] */ __RPC__in LPCWSTR lpReplacedFileName,
    /* [in] */ __RPC__in LPCWSTR lpReplacementFileName,
    /* [in] */ __RPC__in LPCWSTR lpBackupFileName,
    /* [in] */ BOOLEAN bIgnoreMergeErrors);

/* [entry] */ HRESULT FabricFileCreateHardLink( 
    /* [in] */ __RPC__in LPCWSTR lpFileName,
    /* [in] */ __RPC__in LPCWSTR lpExistingFileName,
    /* [out] */ __RPC__out BOOLEAN *succeeded);

/* [entry] */ HRESULT FabricFileGetSize( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__out LONGLONG *size);

/* [entry] */ HRESULT FabricFileGetLastWriteTime( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__out FILETIME *lastWriteTime);

/* [entry] */ HRESULT FabricFileGetVersionInfo( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **uncPath);

/* [entry] */ HRESULT FabricFileRemoveReadOnlyAttribute( 
    /* [in] */ __RPC__in LPCWSTR path);

/* [entry] */ HRESULT FabricGetUncPath( 
    /* [in] */ __RPC__in LPCWSTR path,
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **uncPath);

/* [entry] */ HRESULT FabricGetNodeIdFromNodeName( 
    /* [in] */ __RPC__in LPCWSTR nodeName,
    /* [in] */ __RPC__in LPCWSTR rolesForWhichToUseV1generator,
    /* [in] */ BOOLEAN useV2NodeIdGenerator,
    /* [in] */ __RPC__in LPCWSTR nodeIdGeneratorVersion,
    /* [out] */ __RPC__out FABRIC_NODE_ID *fabricNodeId);

/* [entry] */ HRESULT CabExtractFiltered( 
    /* [in] */ __RPC__in LPCWSTR cabPath,
    /* [in] */ __RPC__in LPCWSTR extractPath,
    /* [in] */ __RPC__in LPCWSTR filters,
    /* [in] */ BOOLEAN inclusive);

/* [entry] */ HRESULT IsCabFile( 
    /* [in] */ __RPC__in LPCWSTR cabPath,
    /* [retval][out] */ __RPC__out BOOLEAN *isCab);

/* [entry] */ HRESULT GenerateSelfSignedCertAndImportToStore( 
    /* [in] */ __RPC__in LPCWSTR subName,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ __RPC__in LPCWSTR profile,
    /* [in] */ __RPC__in LPCWSTR DNS,
    /* [in] */ FILETIME expireDate);

/* [entry] */ HRESULT GenerateSelfSignedCertAndSaveAsPFX( 
    /* [in] */ __RPC__in LPCWSTR subName,
    /* [in] */ __RPC__in LPCWSTR fileName,
    /* [in] */ __RPC__in LPCWSTR password,
    /* [in] */ __RPC__in LPCWSTR DNS,
    /* [in] */ FILETIME expireDate);

/* [entry] */ HRESULT DeleteCertificateFromStore( 
    /* [in] */ __RPC__in LPCWSTR certName,
    /* [in] */ __RPC__in LPCWSTR store,
    /* [in] */ __RPC__in LPCWSTR profile,
    /* [in] */ BOOLEAN isExactMatch);

/* [entry] */ HRESULT WriteManagedTrace( 
    /* [in] */ __RPC__in LPCWSTR taskName,
    /* [in] */ __RPC__in LPCWSTR eventName,
    /* [in] */ __RPC__in LPCWSTR id,
    /* [in] */ USHORT level,
    /* [in] */ __RPC__in LPCWSTR text);

/* [entry] */ HRESULT WriteManagedStructuredTrace( 
    /* [in] */ __RPC__in const FABRIC_ETW_TRACE_EVENT_PAYLOAD *eventPayload);

/* [entry] */ HRESULT VerifyFileSignature( 
    /* [in] */ __RPC__in LPCWSTR filename,
    /* [out] */ __RPC__out BOOLEAN *isValid);

/* [entry] */ HRESULT FabricSetEnableCircularTraceSession2( 
    /* [in] */ BOOLEAN enableCircularTraceSession,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetEnableCircularTraceSession(
    /* [in] */ BOOLEAN enableCircularTraceSession);

/* [entry] */ HRESULT FabricSetEnableUnsupportedPreviewFeatures2(
    /* [in] */ BOOLEAN enableUnsupportedPreviewFeatures,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetEnableUnsupportedPreviewFeatures(
    /* [in] */ BOOLEAN enableUnsupportedPreviewFeatures);

/* [entry] */ HRESULT FabricSetIsSFVolumeDiskServiceEnabled2(
    /* [in] */ BOOLEAN isSFVolumeDiskServiceEnabled,
    /* [in] */ __RPC__in LPCWSTR machineName);

/* [entry] */ HRESULT FabricSetIsSFVolumeDiskServiceEnabled( 
    /* [in] */ BOOLEAN isSFVolumeDiskServiceEnabled);

/* [entry] */ HRESULT FabricGetEnableCircularTraceSession2( 
    /* [in] */ __RPC__in LPCWSTR machineName,
    /* [retval][out] */ __RPC__out BOOLEAN *enableCircularTraceSession);

/* [entry] */ HRESULT FabricGetEnableCircularTraceSession( 
    /* [retval][out] */ __RPC__out BOOLEAN *enableCircularTraceSession);

/* [entry] */ void FabricSetCrashLeasingApplicationCallback(
    /* [in] */ __RPC__in void *callback);

/* [entry] */ void FabricGetCrashLeasingApplicationCallback(
    /* [retval][out] */ __RPC__deref_out_opt void **callback);

/* [entry] */ HRESULT FabricSetSfInstalledMoby(
    /* [in] */ __RPC__in LPCWSTR fileContents);

#endif /* __FabricCommonInternalModule_MODULE_DEFINED__ */
#endif /* __FabricCommonInternalLib_LIBRARY_DEFINED__ */

#ifndef __IFabricStoreLayoutSpecification2_INTERFACE_DEFINED__
#define __IFabricStoreLayoutSpecification2_INTERFACE_DEFINED__

/* interface IFabricStoreLayoutSpecification2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStoreLayoutSpecification2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e4a20405-2cef-4da0-bd6c-b466a5845d48")
    IFabricStoreLayoutSpecification2 : public IFabricStoreLayoutSpecification
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubPackageArchiveFile( 
            /* [in] */ LPCWSTR packageFolder,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStoreLayoutSpecification2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStoreLayoutSpecification2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStoreLayoutSpecification2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationManifestFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInstanceFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationInstanceVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationPackageFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicePackageFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestChecksumFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageChecksumFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageChecksumFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageChecksumFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubPackageArchiveFile )( 
            IFabricStoreLayoutSpecification2 * This,
            /* [in] */ LPCWSTR packageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricStoreLayoutSpecification2Vtbl;

    interface IFabricStoreLayoutSpecification2
    {
        CONST_VTBL struct IFabricStoreLayoutSpecification2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStoreLayoutSpecification2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStoreLayoutSpecification2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStoreLayoutSpecification2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStoreLayoutSpecification2_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricStoreLayoutSpecification2_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricStoreLayoutSpecification2_GetApplicationManifestFile(This,applicationTypeName,applicationTypeVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationManifestFile(This,applicationTypeName,applicationTypeVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetApplicationInstanceFile(This,applicationTypeName,aplicationId,applicationInstanceVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationInstanceFile(This,applicationTypeName,aplicationId,applicationInstanceVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetApplicationPackageFile(This,applicationTypeName,aplicationId,applicationRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationPackageFile(This,applicationTypeName,aplicationId,applicationRolloutVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetServicePackageFile(This,applicationTypeName,aplicationId,servicePackageName,servicePackageRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetServicePackageFile(This,applicationTypeName,aplicationId,servicePackageName,servicePackageRolloutVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetServiceManifestFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetServiceManifestChecksumFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestChecksumFile(This,applicationTypeName,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetCodePackageFolder(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetCodePackageChecksumFile(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result)	\
    ( (This)->lpVtbl -> GetCodePackageChecksumFile(This,applicationTypeName,serviceManifestName,codePackageName,codePackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetConfigPackageFolder(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetConfigPackageChecksumFile(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result)	\
    ( (This)->lpVtbl -> GetConfigPackageChecksumFile(This,applicationTypeName,serviceManifestName,configPackageName,configPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetDataPackageFolder(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetDataPackageChecksumFile(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result)	\
    ( (This)->lpVtbl -> GetDataPackageChecksumFile(This,applicationTypeName,serviceManifestName,dataPackageName,dataPackageVersion,result) ) 

#define IFabricStoreLayoutSpecification2_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 


#define IFabricStoreLayoutSpecification2_GetSubPackageArchiveFile(This,packageFolder,result)	\
    ( (This)->lpVtbl -> GetSubPackageArchiveFile(This,packageFolder,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStoreLayoutSpecification2_INTERFACE_DEFINED__ */


#ifndef __IFabricRunLayoutSpecification2_INTERFACE_DEFINED__
#define __IFabricRunLayoutSpecification2_INTERFACE_DEFINED__

/* interface IFabricRunLayoutSpecification2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricRunLayoutSpecification2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("10AD836A-12E5-4581-9303-65DE0710EB88")
    IFabricRunLayoutSpecification2 : public IFabricRunLayoutSpecification
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetEndpointDescriptionsFile2( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageActivationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentServicePackageFile2( 
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageActivationId,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricRunLayoutSpecification2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricRunLayoutSpecification2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricRunLayoutSpecification2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRoot )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR value);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IFabricRunLayoutSpecification2 * This,
            /* [retval][out] */ IFabricStringResult **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationWorkFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationLogFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationTempFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationPackageFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR applicationRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServicePackageFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageRolloutVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentServicePackageFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndpointDescriptionsFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR aplicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR serviceManifestVersion,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR codePackageName,
            /* [in] */ LPCWSTR codePackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigPackageFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR configPackageName,
            /* [in] */ LPCWSTR configPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageFolder )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR dataPackageName,
            /* [in] */ LPCWSTR dataPackageVersion,
            /* [in] */ BOOLEAN isSharedPackage,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetSettingsFile )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR configPackageFolder,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndpointDescriptionsFile2 )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageActivationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentServicePackageFile2 )( 
            IFabricRunLayoutSpecification2 * This,
            /* [in] */ LPCWSTR applicationId,
            /* [in] */ LPCWSTR servicePackageName,
            /* [in] */ LPCWSTR servicePackageActivationId,
            /* [retval][out] */ IFabricStringResult **result);
        
        END_INTERFACE
    } IFabricRunLayoutSpecification2Vtbl;

    interface IFabricRunLayoutSpecification2
    {
        CONST_VTBL struct IFabricRunLayoutSpecification2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricRunLayoutSpecification2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricRunLayoutSpecification2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricRunLayoutSpecification2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricRunLayoutSpecification2_SetRoot(This,value)	\
    ( (This)->lpVtbl -> SetRoot(This,value) ) 

#define IFabricRunLayoutSpecification2_GetRoot(This,value)	\
    ( (This)->lpVtbl -> GetRoot(This,value) ) 

#define IFabricRunLayoutSpecification2_GetApplicationFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification2_GetApplicationWorkFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationWorkFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification2_GetApplicationLogFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationLogFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification2_GetApplicationTempFolder(This,applicationId,result)	\
    ( (This)->lpVtbl -> GetApplicationTempFolder(This,applicationId,result) ) 

#define IFabricRunLayoutSpecification2_GetApplicationPackageFile(This,aplicationId,applicationRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetApplicationPackageFile(This,aplicationId,applicationRolloutVersion,result) ) 

#define IFabricRunLayoutSpecification2_GetServicePackageFile(This,aplicationId,servicePackageName,servicePackageRolloutVersion,result)	\
    ( (This)->lpVtbl -> GetServicePackageFile(This,aplicationId,servicePackageName,servicePackageRolloutVersion,result) ) 

#define IFabricRunLayoutSpecification2_GetCurrentServicePackageFile(This,aplicationId,servicePackageName,result)	\
    ( (This)->lpVtbl -> GetCurrentServicePackageFile(This,aplicationId,servicePackageName,result) ) 

#define IFabricRunLayoutSpecification2_GetEndpointDescriptionsFile(This,aplicationId,servicePackageName,result)	\
    ( (This)->lpVtbl -> GetEndpointDescriptionsFile(This,aplicationId,servicePackageName,result) ) 

#define IFabricRunLayoutSpecification2_GetServiceManifestFile(This,applicationId,serviceManifestName,serviceManifestVersion,result)	\
    ( (This)->lpVtbl -> GetServiceManifestFile(This,applicationId,serviceManifestName,serviceManifestVersion,result) ) 

#define IFabricRunLayoutSpecification2_GetCodePackageFolder(This,applicationId,servicePackageName,codePackageName,codePackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetCodePackageFolder(This,applicationId,servicePackageName,codePackageName,codePackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification2_GetConfigPackageFolder(This,applicationId,servicePackageName,configPackageName,configPackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetConfigPackageFolder(This,applicationId,servicePackageName,configPackageName,configPackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification2_GetDataPackageFolder(This,applicationId,servicePackageName,dataPackageName,dataPackageVersion,isSharedPackage,result)	\
    ( (This)->lpVtbl -> GetDataPackageFolder(This,applicationId,servicePackageName,dataPackageName,dataPackageVersion,isSharedPackage,result) ) 

#define IFabricRunLayoutSpecification2_GetSettingsFile(This,configPackageFolder,result)	\
    ( (This)->lpVtbl -> GetSettingsFile(This,configPackageFolder,result) ) 


#define IFabricRunLayoutSpecification2_GetEndpointDescriptionsFile2(This,applicationId,servicePackageName,servicePackageActivationId,result)	\
    ( (This)->lpVtbl -> GetEndpointDescriptionsFile2(This,applicationId,servicePackageName,servicePackageActivationId,result) ) 

#define IFabricRunLayoutSpecification2_GetCurrentServicePackageFile2(This,applicationId,servicePackageName,servicePackageActivationId,result)	\
    ( (This)->lpVtbl -> GetCurrentServicePackageFile2(This,applicationId,servicePackageName,servicePackageActivationId,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricRunLayoutSpecification2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


