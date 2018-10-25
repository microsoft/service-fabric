// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerEventNotification
    {
        internal ContainerEventNotification()
        {
        }

        internal UInt64 SinceTime { get; set; }

        internal UInt64 UntilTime { get; set; }

        internal List<ContainerEventDescription> EventList { get; set; }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeNotification = new NativeTypes.FABRIC_CONTAINER_EVENT_NOTIFICATION
            {
                SinceTime = this.SinceTime,
                UntilTime = this.UntilTime,
                EventList = ContainerEventDescription.ToNativeList(pin, this.EventList)
            };

            return pin.AddBlittable(nativeNotification);
        }
    }
}