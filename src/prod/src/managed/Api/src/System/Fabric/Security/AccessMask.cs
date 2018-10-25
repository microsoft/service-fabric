// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System.Fabric.Interop;

    [Flags]
    internal enum AccessMask
    {
        None = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_ACCESS_MASK_NONE,
        Create = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_ACCESS_MASK_CREATE,
        Write = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_ACCESS_MASK_WRITE,
        Read = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_ACCESS_MASK_READ,
        Delete = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_STANDARD_RIGHTS_DELETE,
        ReadAcl = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_STANDARD_RIGHTS_READ_CONTROL,
        WriteAcl = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_STANDARD_RIGHTS_WRITE_DAC,
        GenericRead = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_GENERIC_READ,
        GenericWrite = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_GENERIC_WRITE,
        GenericAll = NativeTypes.FABRIC_ACCESS_MASK.FABRIC_GENERIC_ALL
    }
}