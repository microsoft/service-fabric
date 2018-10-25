// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is an enum used during byte serialization/deserialization to encode type of a Chaos event
    /// </summary>
    internal enum ChaosEventType
    {
        Invalid                 = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_INVALID,
        Started                 = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_STARTED,
        ExecutingFaults         = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS,
        Waiting                 = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_WAITING,
        ValidationFailed        = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED,
        TestError               = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_TEST_ERROR,
        Stopped                 = NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_STOPPED
    }
}