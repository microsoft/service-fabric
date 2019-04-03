// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

#if !DotNetCoreClrIOT
using namespace System;
using namespace System::Globalization;

#using "System.Fabric.AzureActiveDirectory.Server.dll" as_friend
#endif


extern "C" DllExport int __cdecl IsAdminRole(
    wchar_t const * expectedIssuer, 
    wchar_t const * expectedAudience, 
    wchar_t const * expectedRoleClaimKey, 
    wchar_t const * expectedAdminRoleValue, 
    wchar_t const * expectedUserRoleValue, 
    wchar_t const * certEndpoint, 
    __int64 const certRolloverCheckIntervalTicks,
    wchar_t const * jwt, 
    bool * isAdmin,
    int * expirationSeconds,
    wchar_t * errorMessageBuffer, 
    int errorMessageBufferSize)
{
    CHECK_NULL(expectedIssuer)
    CHECK_NULL(expectedAudience)
    CHECK_NULL(expectedRoleClaimKey)
    CHECK_NULL(expectedAdminRoleValue)
    CHECK_NULL(expectedUserRoleValue)
    CHECK_NULL(certEndpoint)
    CHECK_NULL(jwt)
    CHECK_NULL(isAdmin)
    CHECK_NULL(expirationSeconds)

#if DotNetCoreClrIOT
    UNREFERENCED_PARAMETER(errorMessageBuffer);
    UNREFERENCED_PARAMETER(certRolloverCheckIntervalTicks);
    UNREFERENCED_PARAMETER(errorMessageBufferSize);
    return 1;
#else
    try
    {
        auto managedExpectedIssuer = gcnew String(expectedIssuer);
        auto managedExpectedAudience = gcnew String(expectedAudience);
        auto managedExpectedRoleClaimKey = gcnew String(expectedRoleClaimKey);
        auto managedExpectedAdminRoleValue = gcnew String(expectedAdminRoleValue);
        auto managedExpectedUserRoleValue = gcnew String(expectedUserRoleValue);
        auto managedCertEndpoint = gcnew String(certEndpoint);
        auto managedJwt = gcnew String(jwt);

        auto claims = System::Fabric::AzureActiveDirectory::Server::ServerUtility::Validate(
            managedExpectedIssuer, 
            managedExpectedAudience,
            managedExpectedRoleClaimKey,
            managedExpectedAdminRoleValue,
            managedExpectedUserRoleValue,
            managedCertEndpoint,
            certRolloverCheckIntervalTicks,
            managedJwt);

        *isAdmin = claims->IsAdmin;
        *expirationSeconds = (int)claims->Expiration.TotalSeconds;
    }
    catch (Exception^ e)
    {
        CopyFromManaged(errorMessageBuffer, errorMessageBufferSize, e->ToString());

        return 1;
    }
    return 0;
#endif  
}
