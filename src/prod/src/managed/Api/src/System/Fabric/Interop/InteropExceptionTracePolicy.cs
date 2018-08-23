// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;

    internal enum InteropExceptionTracePolicy
    {
        None,
        Info,
        Warning,
        WarningExceptInfoForTransient,
        Default = Warning
    }
}