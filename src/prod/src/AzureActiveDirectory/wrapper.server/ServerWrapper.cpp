// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace AzureActiveDirectory;

static const StringLiteral TraceComponent("Aad::ServerWrapper");

#if !defined(PLATFORM_UNIX)
#define DllImport __declspec( dllimport )
#define ERROR_MESSAGE_BUFFER_SIZE 4096

extern "C" DllImport int IsAdminRole(
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
    int errorMessageBufferSize);
#endif

ErrorCode ServerWrapper::ValidateClaims(
    wstring const & jwt,
    __out wstring & claims,
    __out TimeSpan & expiration)
{
#if DotNetCoreClrIOT
    return ValidateClaims_CoreIOT(jwt, claims, expiration);
#elif !defined(PLATFORM_UNIX)
    return ValidateClaims_Windows(jwt, claims, expiration);
#else
    return ValidateClaims_Linux(jwt, claims, expiration);
#endif
}

#if !defined(PLATFORM_UNIX)
ErrorCode ServerWrapper::ValidateClaims_Windows(
    wstring const & jwt,
    __out wstring & claims,
    __out TimeSpan & expiration)
{
    claims.clear();

    wchar_t errMessageBuffer[ERROR_MESSAGE_BUFFER_SIZE];
    auto const & securityConfig = SecurityConfig::GetConfig();

    auto expectedIssuer = wformatString(securityConfig.AADTokenIssuerFormat, securityConfig.AADTenantId);
    auto certEndpoint = wformatString(securityConfig.AADCertEndpointFormat, securityConfig.AADTenantId);
    auto const & expectedAudience = securityConfig.AADClusterApplication;
    auto const & expectedRoleClaimKey = securityConfig.AADRoleClaimKey;
    auto const & signingCertRolloverCheckIntervalTicks = securityConfig.AADSigningCertRolloverCheckInterval.get_Ticks();

    bool isAdmin = false;
    int expirationSeconds = 0;

    // Claims details not needed for simple admin/user check. The full list of claims
    // can be returned if we decide to support higher fidelity claims.
    //
    auto err = IsAdminRole(
        expectedIssuer.c_str(),
        expectedAudience.c_str(),
        expectedRoleClaimKey.c_str(),
        securityConfig.AADAdminRoleClaimValue.c_str(),
        securityConfig.AADUserRoleClaimValue.c_str(),
        certEndpoint.c_str(),
        signingCertRolloverCheckIntervalTicks,
        jwt.c_str(),
        &isAdmin,
        &expirationSeconds,
        &errMessageBuffer[0], 
        ERROR_MESSAGE_BUFFER_SIZE);

    bool success = (err == 0);

    if (!success)
    {
        auto errorMessage = wstring(move(&errMessageBuffer[0]));

        auto msg = wformatString(GET_COMMON_RC( IsAdmin_Role_Failed ), 
            expectedIssuer,
            expectedAudience,
            expectedRoleClaimKey,
            certEndpoint,
            errorMessage);

        Trace.WriteWarning(TraceComponent, "{0}", msg);

        return ErrorCode(ErrorCodeValue::InvalidCredentials, move(msg));
    }
    else
    {
        Trace.WriteInfo(
            TraceComponent,
            "IsAdminRole succeeded: issuer={0} audience={1} roleClaim={2} cert={3} isAdmin={4}",
            expectedIssuer,
            expectedAudience,
            expectedRoleClaimKey,
            certEndpoint,
            isAdmin);

        claims = isAdmin ? CreateAdminRoleClaim(securityConfig) : CreateUserRoleClaim(securityConfig);
        expiration = TimeSpan::FromSeconds(expirationSeconds);

        return ErrorCodeValue::Success;
    }
}
#endif

ErrorCode ServerWrapper::ValidateClaims_Linux(
    wstring const &,
    __out wstring &,
    __out TimeSpan &)
{
    Trace.WriteError(
        TraceComponent,
        "Linux does not support in-proc claims validation. TokenValidationService must be enabled with AAD provider.");

    return ErrorCodeValue::NotImplemented;
}

#if DotNetCoreClrIOT
ErrorCode ServerWrapper::ValidateClaims_CoreIOT(
    wstring const &,
    __out wstring &,
    __out TimeSpan &)
{
    Trace.WriteError(
        TraceComponent,
        "Windows CoreIOT does not support in-proc claims validation. TokenValidationService must be enabled with AAD provider.");

    return ErrorCodeValue::NotImplemented;
}
#endif

bool ServerWrapper::TryOverrideClaimsConfigForAad()
{
    auto & securityConfig = SecurityConfig::GetConfig();

    if (IsAadEnabled())
    {
        securityConfig.ClientClaimAuthEnabled = true;
        securityConfig.AdminClientClaims = CreateAdminRoleClaim(securityConfig);
        securityConfig.ClientClaims = CreateUserRoleClaim(securityConfig);

        return true;
    }
    else
    {
        return false;
    }
}

bool ServerWrapper::IsAadEnabled()
{
    return !SecurityConfig::GetConfig().AADTenantId.empty();
}

bool ServerWrapper::IsAadServiceEnabled()
{
    if (IsAadEnabled())
    {
        vector<wstring> sectionNames;
        SecurityConfig::GetConfig().FabricGetConfigStore().GetSections(sectionNames);

        for (auto const & sectionName : sectionNames)
        {
            if (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalTokenValidationServiceName) ||
                StringUtility::StartsWithCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalTokenValidationServicePrefix))
            {
                return true;
            }
        }
    }

    return false;
}

wstring ServerWrapper::CreateAdminRoleClaim(SecurityConfig const & securityConfig)
{
    return wformatString("{0}={1}", securityConfig.AADRoleClaimKey, securityConfig.AADAdminRoleClaimValue);
}

wstring ServerWrapper::CreateUserRoleClaim(SecurityConfig const & securityConfig)
{
    return wformatString("{0}={1}", securityConfig.AADRoleClaimKey, securityConfig.AADUserRoleClaimValue);
}
