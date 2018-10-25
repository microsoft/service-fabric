// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System.Runtime.InteropServices;
    using System.Security;

    [SuppressUnmanagedCodeSecurityAttribute]
    internal delegate uint EventRecordCallback([In] ref EventRecord eventRecord);
}