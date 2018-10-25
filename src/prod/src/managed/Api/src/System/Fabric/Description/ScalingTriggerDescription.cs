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
    /// Base class for describing different scaling triggers. Scaling triggers are used to describe under which conditions a scaling operation should happen.
    /// See <see cref="System.Fabric.Description.AveragePartitionLoadScalingTrigger" /> and <see cref="System.Fabric.Description.AverageServiceLoadScalingTrigger" /> as examples of scaling triggers.
    /// </para>
    /// </summary>
    [KnownType(typeof(AveragePartitionLoadScalingTrigger))]
    [KnownType(typeof(AverageServiceLoadScalingTrigger))]
    public abstract class ScalingTriggerDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingTriggerDescription" /> class.</para>
        /// </summary>
        protected ScalingTriggerDescription()
        {
            Kind = ScalingTriggerKind.Invalid;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingTriggerDescription" /> class of a particular kind.</para>
        /// </summary>
        protected ScalingTriggerDescription(ScalingTriggerKind kind)
        {
            Kind = kind;
        }

        /// <summary>
        /// <para>Gets the kind of the scaling trigger.</para>
        /// </summary>
        /// <value>
        /// <para>Scaling trigger kind associated with this scaling trigger description.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ScalingTriggerKind Kind
        {
            get;
            private set;
        }

        internal abstract void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_TRIGGER scalingTrigger);

        internal static unsafe ScalingTriggerDescription CreateFromNative(NativeTypes.FABRIC_SCALING_TRIGGER trigger)
        {
            if (trigger.ScalingTriggerKind == NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_PARTITION_LOAD)
            {
                return AveragePartitionLoadScalingTrigger.CreateFromNative(trigger);
            }
            else if (trigger.ScalingTriggerKind == NativeTypes.FABRIC_SCALING_TRIGGER_KIND.FABRIC_SCALING_TRIGGER_KIND_AVERAGE_SERVICE_LOAD)
            {
                return AverageServiceLoadScalingTrigger.CreateFromNative(trigger);
            }
            else
            {
                return null;
            }

        }
  
        internal static ScalingTriggerDescription GetCopy(ScalingTriggerDescription other)
        {
            if (other is AveragePartitionLoadScalingTrigger)
            {
                return new AveragePartitionLoadScalingTrigger(other as AveragePartitionLoadScalingTrigger);
            }
            else if (other is AverageServiceLoadScalingTrigger)
            {
                return new AverageServiceLoadScalingTrigger(other as AverageServiceLoadScalingTrigger);
            }
            else
            {
                return null;
            }
        }

        // Method used by JsonSerializer to resolve derived type using json property "ScalingTriggerKind".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute("Kind")]
        internal static Type ResolveDerivedClass(ScalingTriggerKind kind)
        {
            switch (kind)
            {
                case ScalingTriggerKind.AveragePartitionLoadTrigger:
                    return typeof(AveragePartitionLoadScalingTrigger);
                case ScalingTriggerKind.AverageServiceLoadTrigger:
                    return typeof(AverageServiceLoadScalingTrigger);
                default:
                    return null;
            }
        }
    }
}
