// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TestCommon;

#define TESTCODEPACKAGE 'ppCT'

TestComCodePackageActivationContext::TestComCodePackageActivationContext(__in LPCWSTR workingDirectory)
{
    NTSTATUS status = KString::Create(workDirectory_, GetThisAllocator(), workingDirectory);
    THROW_ON_FAILURE(status);
}

TestComCodePackageActivationContext::~TestComCodePackageActivationContext()
{
}

TestComCodePackageActivationContext::SPtr TestComCodePackageActivationContext::Create(
    __in LPCWSTR workingDirectory,
    __in KAllocator & allocator)
{
    TestComCodePackageActivationContext * pointer = _new(TESTCODEPACKAGE, allocator) TestComCodePackageActivationContext(workingDirectory);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

LPCWSTR TestComCodePackageActivationContext::get_ContextId()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

LPCWSTR TestComCodePackageActivationContext::get_CodePackageName()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

LPCWSTR TestComCodePackageActivationContext::get_CodePackageVersion()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}
            
LPCWSTR TestComCodePackageActivationContext::get_WorkDirectory()
{
    return *workDirectory_;
}

LPCWSTR TestComCodePackageActivationContext::get_LogDirectory()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

LPCWSTR TestComCodePackageActivationContext::get_TempDirectory()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * TestComCodePackageActivationContext::get_ServiceEndpointResources()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST * TestComCodePackageActivationContext::get_ServiceTypes()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST * TestComCodePackageActivationContext::get_ServiceGroupTypes()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION * TestComCodePackageActivationContext::get_ApplicationPrincipals()
{
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetCodePackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    UNREFERENCED_PARAMETER(names);
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetDataPackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    UNREFERENCED_PARAMETER(names);
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetConfigurationPackageNames(
    /*[out, retval]*/ IFabricStringListResult ** names)
{
    UNREFERENCED_PARAMETER(names);
    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetCodePackage(
    __in_z LPCWSTR CodePackageName,
    __out IFabricCodePackage **codePackage)
{
    UNREFERENCED_PARAMETER(CodePackageName);
    UNREFERENCED_PARAMETER(codePackage);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetConfigurationPackage(
    __in_z LPCWSTR ConfigPackageName,
    __out IFabricConfigurationPackage **configPackage)
{
    UNREFERENCED_PARAMETER(ConfigPackageName);
    UNREFERENCED_PARAMETER(configPackage);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetDataPackage(
    __in_z LPCWSTR DataPackageName,
    __out IFabricDataPackage **dataPackage)
{
    UNREFERENCED_PARAMETER(DataPackageName);
    UNREFERENCED_PARAMETER(dataPackage);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::GetServiceEndpointResource(
    /*[in]*/ LPCWSTR serviceEndpointResourceName,
    /*[out, retval]*/ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION ** bufferedValue)
{
    UNREFERENCED_PARAMETER(serviceEndpointResourceName);
    UNREFERENCED_PARAMETER(bufferedValue);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::RegisterCodePackageChangeHandler(
    /* [in] */ IFabricCodePackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::UnregisterCodePackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}
    
HRESULT TestComCodePackageActivationContext::RegisterConfigurationPackageChangeHandler(
    /* [in] */ IFabricConfigurationPackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::UnregisterConfigurationPackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::RegisterDataPackageChangeHandler(
    /* [in] */ IFabricDataPackageChangeHandler * callback,
    /* [out, retval] */ LONGLONG * callbackHandle)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}

HRESULT TestComCodePackageActivationContext::UnregisterDataPackageChangeHandler(
    /* [in] */ LONGLONG callbackHandle)
{
    UNREFERENCED_PARAMETER(callbackHandle);

    throw Exception(STATUS_NOT_IMPLEMENTED);
}
