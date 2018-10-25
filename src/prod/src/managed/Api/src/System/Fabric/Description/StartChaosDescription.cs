// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Fabric.Chaos.DataStructures;
    using System.Globalization;

    internal sealed class StartChaosDescription
    {
        public StartChaosDescription(
            ChaosParameters chaosTestScenarioParameters)
        {
            Requires.Argument<ChaosParameters>("chaosTestScenarioParameters", chaosTestScenarioParameters).NotNull();
            this.ChaosParameters = chaosTestScenarioParameters;
        }

        public ChaosParameters ChaosParameters
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "ChaosParameters: {0}",
                this.ChaosParameters);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStartChaosDescription = new NativeTypes.FABRIC_START_CHAOS_DESCRIPTION();

            if (this.ChaosParameters != null)
            {
                nativeStartChaosDescription.ChaosParameters = this.ChaosParameters.ToNative(pinCollection);
            }

            return pinCollection.AddBlittable(nativeStartChaosDescription);
        }

        internal static unsafe StartChaosDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_CHAOS_DESCRIPTION native = *(NativeTypes.FABRIC_START_CHAOS_DESCRIPTION*)nativeRaw;

            ChaosParameters chaosTestScenarioParameters = ChaosParameters.CreateFromNative(native.ChaosParameters);

            return new StartChaosDescription(chaosTestScenarioParameters);
        }
    }
}
