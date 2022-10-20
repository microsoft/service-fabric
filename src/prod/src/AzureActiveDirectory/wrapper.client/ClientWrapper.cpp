// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace AzureActiveDirectory;

static const StringLiteral TraceComponent("Aad::ClientWrapper");

#define DllImport __declspec( dllimport )

extern "C" DllImport int GetAccessToken(
    wchar_t const * authority, 
    wchar_t const * audience, 
    wchar_t const * client, 
    wchar_t const * redirectUri, 
    bool refreshSession,
    wchar_t * outBuffer, 
    int outBufferSize);

ErrorCode ClientWrapper::GetToken(
    shared_ptr<ClaimsRetrievalMetadata> const & metadata,
    size_t bufferSize,
    bool refreshClaimsToken,
    __out wstring & token)
{

#if DotNetCoreClrIOT
    UNREFERENCED_PARAMETER(metadata);
    UNREFERENCED_PARAMETER(bufferSize);
    UNREFERENCED_PARAMETER(refreshClaimsToken);
    UNREFERENCED_PARAMETER(token);

    return ErrorCodeValue::NotImplemented;
#else

    vector<wchar_t> outBuffer;
    outBuffer.resize(bufferSize);

    auto authority = metadata->AAD_Authority;
    auto cluster = metadata->AAD_Cluster;
    auto client = metadata->AAD_Client;
    auto redirect = metadata->AAD_Redirect;
    
    auto err = GetAccessToken(
        authority.c_str(),
        cluster.c_str(),
        client.c_str(),
        redirect.c_str(),
        refreshClaimsToken,
        outBuffer.data(), 
        static_cast<int>(outBuffer.size()));

    bool success = (err == 0);

    if (!success)
    {
        auto errorMessage = wstring(move(&outBuffer[0]));

        auto msg = wformatString(GET_COMMON_RC( Access_Token_Failed ), authority, cluster, client, errorMessage);

        Trace.WriteWarning(TraceComponent, "{0}", msg);

        return ErrorCode(ErrorCodeValue::InvalidCredentials, move(msg));
    }
    else
    {
        token = move(&outBuffer[0]);

        Trace.WriteInfo(
            TraceComponent,
            "GetAccessToken succeeded: authority={0} cluster={1} client={2}",
            authority,
            cluster,
            client);

        return ErrorCodeValue::Success;
    }
#endif
} 
