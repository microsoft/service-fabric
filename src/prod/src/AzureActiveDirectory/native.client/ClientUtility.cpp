// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

#if !DotNetCoreClrIOT
using namespace System;
using namespace System::Globalization;

#using "System.Fabric.AzureActiveDirectory.Client.dll" as_friend
#endif

extern "C" DllExport int __cdecl GetAccessToken(
    wchar_t const * authority, 
    wchar_t const * audience, 
    wchar_t const * client, 
    wchar_t const * redirectUri, 
    bool refreshSession,
    wchar_t * outBuffer, 
    int outBufferSize)
{

#if DotNetCoreClrIOT
    UNREFERENCED_PARAMETER(authority);
    UNREFERENCED_PARAMETER(audience);
    UNREFERENCED_PARAMETER(client);
    UNREFERENCED_PARAMETER(redirectUri);
    UNREFERENCED_PARAMETER(outBuffer);
    UNREFERENCED_PARAMETER(outBufferSize);
    UNREFERENCED_PARAMETER(refreshSession);
    return 1;
#else
    CHECK_NULL_OUT(authority)
    CHECK_NULL_OUT(audience)
    CHECK_NULL_OUT(client)
    CHECK_NULL_OUT(redirectUri)

    try
    {
        auto managedAuthority = gcnew String(authority);
        auto managedAudience = gcnew String(audience);
        auto managedClient = gcnew String(client);
        auto managedRedirectUri = gcnew String(redirectUri);

        auto token = System::Fabric::AzureActiveDirectory::Client::ClientUtility::GetAccessToken(
            managedAuthority, 
            managedAudience,
            managedClient,
            managedRedirectUri,
            refreshSession);

        if (token->Length > outBufferSize)
        {
            throw gcnew InvalidOperationException(String::Format(CultureInfo::InvariantCulture,
                "Token length exceeds buffer: token={0} buffer={1}",
                token->Length,
                outBufferSize));
        }
        else
        {
            CopyFromManaged(outBuffer, outBufferSize, token);
        }
    }
    catch (Exception^ e)
    {
        CopyFromManaged(outBuffer, outBufferSize, e->ToString());
        return 1;
    }
    return 0;
#endif
}
