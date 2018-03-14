// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace Transport;
using namespace HttpServer;

StringLiteral const TraceType("HttpClaimsAuthHandler");

HttpClaimsAuthHandler::HttpClaimsAuthHandler(
    __in Transport::SecurityProvider::Enum handlerType,
    __in FabricClientWrapperSPtr &client)
    : HttpAuthHandler(handlerType)
    , client_(client)
    , clientClaims_()
    , adminClientClaims_()
    , isAadServiceEnabled_(false)
{
    LoadIsAadServiceEnabled();
}

ErrorCode HttpClaimsAuthHandler::OnInitialize(SecuritySettings const& securitySettings)
{
    clientClaims_ = securitySettings.ClientClaims();
    adminClientClaims_ = securitySettings.AdminClientClaims();

    WriteInfo(TraceType, "AdminClientClaims - {0}", adminClientClaims_);
    WriteInfo(TraceType, "UserClientClaims - {0}", clientClaims_);
    return ErrorCode::Success();
}

ErrorCode HttpClaimsAuthHandler::OnSetSecurity(SecuritySettings const& securitySettings)
{
    return OnInitialize(securitySettings);
}

void HttpClaimsAuthHandler::OnCheckAccess(AsyncOperationSPtr const& operation)
{
    auto castedOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);

    wstring authHeader;
    auto error = castedOperation->request_.GetRequestHeader(Constants::AuthorizationHeader, authHeader);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType, 
            "Authorization header not found: uri={0} error={1}",
            castedOperation->request_.GetUrl(),
            error);

        castedOperation->TryComplete(operation, ErrorCodeValue::InvalidCredentials);
        return;
    }

    if (AzureActiveDirectory::ServerWrapper::IsAadEnabled() && !isAadServiceEnabled_)
    {
        this->ValidateAadToken(operation, authHeader);
    }
    else
    {
        if (isAadServiceEnabled_)
        {
            wstring jwt;
            error = ExtractJwtFromAuthHeader(authHeader, jwt);
            if (!error.IsSuccess())
            {
                operation->TryComplete(operation, error);

                return;
            }

            authHeader = move(jwt);
        }

        auto innerOperation = client_->TvsClient->BeginValidateToken(
            authHeader,
            castedOperation->RemainingTime,
            [this](AsyncOperationSPtr const& innerOperation) { this->OnValidateTokenComplete(innerOperation, false); },
            operation);

        this->OnValidateTokenComplete(innerOperation, true);
    }
}

void HttpClaimsAuthHandler::OnValidateTokenComplete(Common::AsyncOperationSPtr const& innerOperation, bool expectedCompletedSynchronously)
{
    if (innerOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & operation = innerOperation->Parent;
    auto castedOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);  

    TimeSpan expirationTime;
    std::vector<ServiceModel::Claim> claimList;
    auto error = client_->TvsClient->EndValidateToken(
        innerOperation,
        expirationTime,
        claimList);

    if (!error.IsSuccess()) 
    {
        WriteInfo(TraceType, "Invalid claims token specified in Authorization header for uri: {0}", castedOperation->request_.GetUrl());
        operation->TryComplete(operation, error);
        return;
    }

    SecuritySettings::RoleClaims parsedClaims;
    if (!SecuritySettings::ClaimListToRoleClaim(claimList, parsedClaims))
    {
        WriteWarning(TraceType, "ClaimListToRoleClaim failed for claimlist: {0}", claimList);
        operation->TryComplete(operation, ErrorCodeValue::InvalidCredentials);
        return;
    }

    this->FinishAccessCheck(operation, castedOperation->request_, parsedClaims);
}

ErrorCode HttpClaimsAuthHandler::ExtractJwtFromAuthHeader(
    wstring const & authHeader,
    __out wstring & jwt)
{
    size_t jwtIndex = authHeader.find(*Constants::Bearer);

    if (jwtIndex == wstring::npos)
    {
        WriteWarning(
            TraceType, 
            "{0} token not found in Authorization header",
            Constants::Bearer);

        return ErrorCodeValue::InvalidCredentials;
    }

    // strip off "Bearer" and leading space
    //
    jwt = authHeader.substr(jwtIndex + Constants::Bearer->size() + 1);

    return ErrorCodeValue::Success;
}

void HttpClaimsAuthHandler::ValidateAadToken(
    AsyncOperationSPtr const & operation,
    wstring const & authHeader)
{
    wstring jwt;
    auto error = ExtractJwtFromAuthHeader(authHeader, jwt);
    if (!error.IsSuccess())
    {
        operation->TryComplete(operation, error);

        return;
    }

    wstring claims;
    TimeSpan unusedExpiration;
    error = AzureActiveDirectory::ServerWrapper::ValidateClaims(
        jwt,
        claims, // out
        unusedExpiration); // out

    SecuritySettings::RoleClaims roleClaims;

    if (error.IsSuccess())
    {
        error = SecuritySettings::StringToRoleClaims(claims, roleClaims);
    }

    if (error.IsSuccess())
    {
        auto castedOperation = AsyncOperation::Get<AccessCheckAsyncOperation>(operation);

        this->FinishAccessCheck(operation, castedOperation->request_, roleClaims);
    }
    else
    {
        operation->TryComplete(operation, error);
    }
}

void HttpClaimsAuthHandler::FinishAccessCheck(
    AsyncOperationSPtr const & operation,
    IRequestMessageContext & requestContext,
    SecuritySettings::RoleClaims & roleClaims)
{
    if (!roleClaims.IsInRole(clientClaims_))
    {
        WriteWarning(
            TraceType,
            "Client claims '{0}' given for uri {1} does not meet requirement '{2}'",
            roleClaims,
            requestContext.GetUrl(),
            clientClaims_);
        operation->TryComplete(operation, ErrorCodeValue::InvalidCredentials);
        return;
    }

    if (roleClaims.IsInRole(adminClientClaims_))
    {
        role_ = RoleMask::Admin;
        WriteInfo(
            TraceType,
            "client claims '{0}' given for uri {1} meets Admin role requirements '{2}'",
            roleClaims,
            requestContext.GetUrl(),
            adminClientClaims_);
    }
    else
    {
        role_ = RoleMask::User;
        WriteInfo(
            TraceType,
            "client claims '{0}' given for uri {1} meets User role requirements '{2}'",
            roleClaims,
            requestContext.GetUrl(),
            clientClaims_);
    }

    operation->TryComplete(operation, ErrorCodeValue::Success);
}

void HttpClaimsAuthHandler::LoadIsAadServiceEnabled()
{
    isAadServiceEnabled_ = AzureActiveDirectory::ServerWrapper::IsAadServiceEnabled();
}
