//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Fabric.Chaos.DataStructures;
    using System.Globalization;

    internal sealed class SetChaosScheduleDescription
    {
        public SetChaosScheduleDescription(
            ChaosScheduleDescription chaosScheduleDescription)
        {
            Requires.Argument<ChaosScheduleDescription>("chaosScheduleDescription", chaosScheduleDescription).NotNull();
            this.ChaosScheduleDescription = chaosScheduleDescription;
        }

        public ChaosScheduleDescription ChaosScheduleDescription
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "ChaosScheduleDescription: {0}",
                this.ChaosScheduleDescription);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeSetChaosScheduleDescription = new NativeTypes.FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION();

            if (this.ChaosScheduleDescription != null)
            {
                nativeSetChaosScheduleDescription.ChaosScheduleDescription = this.ChaosScheduleDescription.ToNative(pinCollection);
            }

            return pinCollection.AddBlittable(nativeSetChaosScheduleDescription);
        }

        internal static unsafe SetChaosScheduleDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION native = *(NativeTypes.FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION*)nativeRaw;

            ChaosScheduleDescription chaosScheduleDescription = ChaosScheduleDescription.FromNative(native.ChaosScheduleDescription);

            return new SetChaosScheduleDescription(chaosScheduleDescription);
        }
    }
}

