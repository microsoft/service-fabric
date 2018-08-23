// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    internal interface IProtectedAccountKeyPlatform
    {
        byte[] DecryptProtectedAccountKey(string protectedAccountKey);

        bool TryGetProtectedAccountKey(string protectedAccountKeyName, out string protectedAccountKey);
    }
}