// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using Runtime.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Class for describing a scaling policy. 
    /// Every scaling policy consists of a <see cref="System.Fabric.Description.ScalingTriggerDescription" /> which describes when scaling should occur and a <see cref="System.Fabric.Description.ScalingMechanismDescription" /> which describes how is scaling performed.
    /// </para>
    /// </summary>
    public class ScalingPolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingPolicyDescription" /> class.</para>
        /// </summary>
        public ScalingPolicyDescription()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingPolicyDescription" /> class with the specified mechanism and trigger. </para>
        /// </summary>
        public ScalingPolicyDescription(ScalingMechanismDescription mechanism, ScalingTriggerDescription trigger)
        {
            ScalingMechanism = mechanism;
            ScalingTrigger = trigger;
        }

        /// <summary>
        /// <para>Gets the scaling mechanism.</para>
        /// </summary>
        /// <value>
        /// <para>Scaling mechanism associated with this scaling policy description.</para>
        /// </value>
        public ScalingMechanismDescription ScalingMechanism
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets the scaling trigger.</para>
        /// </summary>
        /// <value>
        /// <para>Scaling trigger associated with this scaling policy description.</para>
        /// </value>
        public ScalingTriggerDescription ScalingTrigger
        {
            get;
            set;
        }

        internal static ScalingPolicyDescription GetCopy(ScalingPolicyDescription other)
        {
            return new ScalingPolicyDescription(ScalingMechanismDescription.GetCopy(other.ScalingMechanism), ScalingTriggerDescription.GetCopy(other.ScalingTrigger));
        }

        internal IntPtr ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_POLICY scalingPolicy)
        {
            if (ScalingMechanism != null && ScalingTrigger != null)
            {
                ScalingMechanism.ToNative(pinCollection, ref scalingPolicy.ScalingPolicyMechanism);
                ScalingTrigger.ToNative(pinCollection, ref scalingPolicy.ScalingPolicyTrigger);

                return pinCollection.AddBlittable(scalingPolicy);
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        internal static unsafe ScalingPolicyDescription CreateFromNative(IntPtr intPtr)
        {
            NativeTypes.FABRIC_SCALING_POLICY* casted = (NativeTypes.FABRIC_SCALING_POLICY*)intPtr;
            var scalingMechanism = ScalingMechanismDescription.CreateFromNative(casted->ScalingPolicyMechanism);
            var scalingTrigger = ScalingTriggerDescription.CreateFromNative(casted->ScalingPolicyTrigger);

            return new ScalingPolicyDescription(scalingMechanism, scalingTrigger);
        }

        /// <summary>
        /// <para> 
        /// Returns a string of the ScalingPolicyDescription.
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>A string representing the AddRemoveIncrementalNamedPartitionScalingMechanism object.</para>
        /// </returns>
        public override string ToString()
        {
            Text.StringBuilder sb = new Text.StringBuilder();

            sb.AppendFormat("Trigger:{0} Mechanism:{1}",
                ScalingTrigger, ScalingMechanism);

            return sb.ToString();
        }
    }
}
