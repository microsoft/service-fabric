// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

enum Enum : unsigned char
{
    None = 0,
    Ssl = 1, 
    Kerberos = 2, 
    Negotiate = 3,
    Claims = 4 
};

bool IsWindowsProvider(Enum securityProvider);

