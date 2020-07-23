// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Globalization;
using Microsoft.IdentityModel.Clients.ActiveDirectory;

namespace System.Fabric.AzureActiveDirectory.Client
{
    internal sealed class ClientUtility
    {
        public static string GetAccessToken(
            string authority,
            string audience,
            string client,
            string redirectUri,
            bool refreshSession)
        {
            var authContext = new AuthenticationContext(authority);

            var authResult = authContext.AcquireToken(
                audience,
                client,
                new Uri(redirectUri),
                refreshSession ? PromptBehavior.RefreshSession : PromptBehavior.Auto);

            return authResult.AccessToken;
        }
    }
}