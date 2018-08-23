// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal enum ContainerEventType
    {
        None = NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_NONE,

        Stop = NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_STOP,

        Die = NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_DIE,

        Health = NativeTypes.FABRIC_CONTAINER_EVENT_TYPE.FABRIC_CONTAINER_EVENT_TYPE_HEALTH
    }

    internal sealed class ContainerEventDescription
    {
        internal ContainerEventDescription()
        {
        }

        internal ContainerEventType EventType { get; set; }

        internal string ContainerId { get; set; }

        internal string ContainerName { get; set; }

        internal UInt64 TimeStampInSeconds { get; set; }

        internal bool IsHealthy { get; set; }

        internal UInt32 ExitCode { get; set; }

        internal static IntPtr ToNativeList(PinCollection pin, List<ContainerEventDescription> eventDescList)
        {
            ReleaseAssert.AssertIfNot(
                eventDescList != null, "ContainerEventDescription.ToNativeList() has null eventDescList.");

            var nativeArray = new NativeTypes.FABRIC_CONTAINER_EVENT_DESCRIPTION[eventDescList.Count];

            for (int i = 0; i < eventDescList.Count; ++i)
            {
                eventDescList[i].ToNative(pin, ref nativeArray[i]);
            }

            var nativeList = new NativeTypes.FABRIC_CONTAINER_EVENT_DESCRIPTION_LIST
            {
                Count = (uint)eventDescList.Count,
                Items = pin.AddBlittable(nativeArray)
            };

            return pin.AddBlittable(nativeList);
        }

        internal void ToNative(PinCollection pin, ref NativeTypes.FABRIC_CONTAINER_EVENT_DESCRIPTION nativeDesc)
        {
            nativeDesc.EventType = InteropHelpers.ToNativeContainerEventType(this.EventType);
            nativeDesc.ContainerId = pin.AddBlittable(this.ContainerId);
            nativeDesc.ContainerName = pin.AddBlittable(this.ContainerName);
            nativeDesc.TimeStampInSeconds = this.TimeStampInSeconds;
            nativeDesc.IsHealthy = NativeTypes.ToBOOLEAN(this.IsHealthy);
            nativeDesc.ExitCode = this.ExitCode;
        }
    }
}