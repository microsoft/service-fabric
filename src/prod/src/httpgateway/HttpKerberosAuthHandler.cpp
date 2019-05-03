// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace Transport;
using namespace HttpServer;

StringLiteral const TraceType("HttpKerberosAuthHandler");
static GENERIC_MAPPING genericMapping = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };

void HttpKerberosAuthHandler::OnCheckAccess(AsyncOperationSPtr const& operation)
{
    auto thisOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);
    HANDLE hToken = INVALID_HANDLE_VALUE;
    auto error = thisOperation->request_.GetClientToken(hToken);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType, "Token not found for uri: {0}", thisOperation->request_.GetUrl());
        UpdateAuthenticationHeaderOnFailure(operation);
        operation->TryComplete(operation, ErrorCodeValue::InvalidCredentials);
        return;
    }

    if (!CheckToken(thisOperation->request_, hToken, allowedClientsSecurityDescriptor_->PSecurityDescriptor, GENERIC_EXECUTE, &genericMapping))
    {
        UpdateAuthenticationHeaderOnFailure(operation);
        operation->TryComplete(operation, ErrorCodeValue::InvalidCredentials);
        return;
    }

    if (isClientRoleInEffect_)
    {
        if (CheckToken(thisOperation->request_, hToken, adminClientsSecurityDescriptor_->PSecurityDescriptor, GENERIC_EXECUTE, &genericMapping))
        {
            // client role is admin
            role_ = RoleMask::Admin;
        }
        else
        {
            role_ = RoleMask::User;
        }
    }
    else
    {
        role_ = RoleMask::None;
    }

    operation->TryComplete(operation, ErrorCodeValue::Success); 
}

bool HttpKerberosAuthHandler::CheckToken(
    __in IRequestMessageContext const& request,
    HANDLE const& hToken,
    PSECURITY_DESCRIPTOR const& securityDescriptor,
    __in DWORD desiredAccess,
    __in PGENERIC_MAPPING genMapping) const
{
    MapGenericMask(&desiredAccess, genMapping);

    PRIVILEGE_SET privilegeSet;
    DWORD privSetSize = sizeof(PRIVILEGE_SET);
    DWORD grantedAccess;
    BOOL accessStatus;

    BOOL callSucceeded = AccessCheck(
        securityDescriptor,
        hToken,
        desiredAccess,
        genMapping,
        &privilegeSet,
        &privSetSize,
        &grantedAccess,
        &accessStatus);

    if (callSucceeded == FALSE)
    {
        WriteInfo(TraceType, "AccessCheck call failed for uri: {0}, LastError: {1}", request.GetUrl(), GetLastError());
        return false;
    }

    if (accessStatus == FALSE)
    {
        WriteInfo(TraceType, "AccessCheck: output assessStatus = FALSE for uri: {0}, LastError: {1}", request.GetUrl(), GetLastError());
        return false;
    }

    bool accessCheckPassed = (grantedAccess == desiredAccess);
    if (!accessCheckPassed)
    {
        WriteInfo(TraceType, "AccessCheck: different access granted for uri: {0}, desired = {1}, granted = {2}", request.GetUrl(), desiredAccess, grantedAccess);
    }

    return accessCheckPassed;
}

ErrorCode HttpKerberosAuthHandler::OnInitialize(SecuritySettings const& securitySettings)
{
    HANDLE processToken;
    BOOL retval = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken);
    if (!retval) { return ErrorCodeValue::NotFound; }

    auto error = TransportSecurity::CreateSecurityDescriptorFromAllowedList(
        processToken,
        true,
        securitySettings.RemoteIdentities(),
        FILE_GENERIC_EXECUTE,
        allowedClientsSecurityDescriptor_);

    if (!error.IsSuccess()) { return error; }

    if (isClientRoleInEffect_)
    {
        error = TransportSecurity::CreateSecurityDescriptorFromAllowedList(
            processToken,
            true, // TODO: should this be configurable?
            securitySettings.AdminClientIdentities(), 
            FILE_GENERIC_EXECUTE,
            adminClientsSecurityDescriptor_);

        if (!error.IsSuccess()) { return error; }
    }
    
    return ErrorCode::Success();
}

ErrorCode HttpKerberosAuthHandler::OnSetSecurity(SecuritySettings const& securitySettings)
{
    return OnInitialize(securitySettings);
}

void HttpKerberosAuthHandler::UpdateAuthenticationHeaderOnFailure(AsyncOperationSPtr const& operation)
{
    auto thisOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);

    thisOperation->httpStatusOnError_ = Constants::StatusAuthenticate;
    thisOperation->authHeaderName_ = Constants::WWWAuthenticateHeader;
    thisOperation->authHeaderValue_ = Constants::NegotiateHeaderValue;
}
