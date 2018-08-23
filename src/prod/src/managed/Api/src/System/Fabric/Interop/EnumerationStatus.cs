// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;

    [Flags]
    internal enum EnumerationStatus
    {
        Invalid             = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_INVALID,
        BestEffortMoreData  = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_BEST_EFFORT_MORE_DATA,
        ConsistentMoreData  = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_CONSISTENT_MORE_DATA,
        BestEffortFinished  = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_BEST_EFFORT_FINISHED,
        ConsistentFinished  = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_CONSISTENT_FINISHED,

        ValidMask           = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_VALID_MASK,
        BestEffortMask      = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_BEST_EFFORT_MASK,
        ConsistentMask      = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_CONSISTENT_MASK,
        MoreDataMask        = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_MORE_DATA_MASK,
        FinishedMask        = NativeTypes.FABRIC_ENUMERATION_STATUS.FABRIC_ENUMERATION_FINISHED_MASK
    }
}