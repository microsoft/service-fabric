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
    /// Base class for describing different scaling mechanisms. Scaling mechanisms are a method of describing what should be done when a scaling operation is triggered.
    /// See <see cref="System.Fabric.Description.AddRemoveIncrementalNamedPartitionScalingMechanism" /> and <see cref="System.Fabric.Description.PartitionInstanceCountScaleMechanism" /> as examples of scaling mechanisms.
    /// </para>
    /// </summary>
    [KnownType(typeof(AddRemoveIncrementalNamedPartitionScalingMechanism))]
    [KnownType(typeof(PartitionInstanceCountScaleMechanism))]
    public abstract class ScalingMechanismDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingMechanismDescription" /> class.</para>
        /// </summary>
        protected ScalingMechanismDescription()
        {
            Kind = ScalingMechanismKind.Invalid;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ScalingMechanismDescription" /> class of a particular kind.</para>
        /// </summary>
        protected ScalingMechanismDescription(ScalingMechanismKind kind)
        {
            Kind = kind;
        }

        /// <summary>
        /// <para>Gets the kind of the scaling mechanism.</para>
        /// </summary>
        /// <value>
        /// <para>Scaling mechanism kind associated with this scaling policy mechanism.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ScalingMechanismKind Kind
        {
            get;
            private set;
        }

        internal static ScalingMechanismDescription GetCopy(ScalingMechanismDescription other)
        {
            if (other is PartitionInstanceCountScaleMechanism)
            {
                return new PartitionInstanceCountScaleMechanism(other as PartitionInstanceCountScaleMechanism);
            }
            else if (other is AddRemoveIncrementalNamedPartitionScalingMechanism)
            {
                return new AddRemoveIncrementalNamedPartitionScalingMechanism(other as AddRemoveIncrementalNamedPartitionScalingMechanism);
            }
            else
            {
                return null;
            }
        }

        internal abstract void ToNative(PinCollection pinCollection, ref NativeTypes.FABRIC_SCALING_MECHANISM scalingMechanism);

        internal static unsafe ScalingMechanismDescription CreateFromNative(NativeTypes.FABRIC_SCALING_MECHANISM mechanism)
        {
            if (mechanism.ScalingMechanismKind == NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_SCALE_PARTITION_INSTANCE_COUNT)
            {
                return PartitionInstanceCountScaleMechanism.CreateFromNative(mechanism);
            }
            else if (mechanism.ScalingMechanismKind == NativeTypes.FABRIC_SCALING_MECHANISM_KIND.FABRIC_SCALING_MECHANISM_KIND_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION)
            {
                return AddRemoveIncrementalNamedPartitionScalingMechanism.CreateFromNative(mechanism);
            }
            else
            {
                return null;
            }
        }


        // Method used by JsonSerializer to resolve derived type using json property "ScalingMechanismKind".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute("Kind")]
        internal static Type ResolveDerivedClass(ScalingMechanismKind kind)
        {
            switch (kind)
            {
                case ScalingMechanismKind.ScalePartitionInstanceCount:
                    return typeof(PartitionInstanceCountScaleMechanism);
                case ScalingMechanismKind.AddRemoveIncrementalNamedPartition:
                    return typeof(AddRemoveIncrementalNamedPartitionScalingMechanism);
                default:
                    return null;
            }
        }
    }
}
