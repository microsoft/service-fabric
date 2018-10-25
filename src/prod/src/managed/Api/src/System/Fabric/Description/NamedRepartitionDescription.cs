// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes partitioning changes for an existing service of partition scheme type <see cref="System.Fabric.Description.PartitionScheme.Named" />.</para>
    /// </summary>
    public sealed class NamedRepartitionDescription : RepartitionDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of this class.</para>
        /// </summary>
        /// <param name="namesToAdd">
        /// <para>
        /// A list of partition names to add to the service.
        /// </para>
        /// </param>
        /// <param name="namesToRemove">
        /// <para>
        /// A list of partition names to remove from the service.
        /// </para>
        /// </param>
        /// <remarks>
        /// <para>
        /// Either <paramref name="namesToAdd"/> or <paramref name="namesToRemove" /> must be non-empty.
        /// </para>
        /// </remarks>
        public NamedRepartitionDescription(IEnumerable<string> namesToAdd, IEnumerable<string> namesToRemove) : base(PartitionScheme.Named)
        {
            this.NamesToAdd = new ItemList<string>();
            this.NamesToRemove = new ItemList<string>();

            if (namesToAdd != null)
            {
                foreach (var name in namesToAdd)
                {
                    this.NamesToAdd.Add(name);
                }
            }

            if (namesToRemove != null)
            {
                foreach (var name in namesToRemove)
                {
                    this.NamesToRemove.Add(name);
                }
            }

            if (this.NamesToAdd.Count == 0 && this.NamesToRemove.Count == 0)
            {
                throw new ArgumentException(StringResources.Error_NamedRepartitionNoOp);
            }
        }

        /// <summary>
        /// <para>Gets the list of partition names to add to the service.</para>
        /// </summary>
        /// <value>
        /// <para>The list of partition names to add.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// Note that no data migration is performed when adding a new partition name to stateful services.
        /// The newly added partition will have no user data.
        /// </para>
        /// </remarks>
        public IList<string> NamesToAdd
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the list of partition names to remove from the service.</para>
        /// </summary>
        /// <value>
        /// <para>The list of partition names to remove.</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// Note that removing a partition name from a stateful service results in the permanent 
        /// loss of all data stored in that partition. No migration of data is performed during
        /// the removal and the operation is not reversible. Re-adding a previously removed
        /// partition name results in a new partition containing no user data.
        /// </para>
        /// </remarks>
        public IList<string> NamesToRemove
        {
            get;
            private set;
        }

        internal override unsafe IntPtr ToNative(PinCollection pin, out NativeTypes.FABRIC_SERVICE_PARTITION_KIND kind)
        {
            var nativeDescription = new NativeTypes.FABRIC_NAMED_REPARTITION_DESCRIPTION[1];

            var toAddArray = new IntPtr[this.NamesToAdd.Count];
            for (var ix=0; ix<toAddArray.Length; ++ix)
            {
                toAddArray[ix] = pin.AddObject(this.NamesToAdd[ix]);
            }
            nativeDescription[0].NamesToAddCount = (uint)toAddArray.Length;
            nativeDescription[0].NamesToAdd = pin.AddBlittable(toAddArray);

            var toRemoveArray = new IntPtr[this.NamesToRemove.Count];
            for (var ix=0; ix<toRemoveArray.Length; ++ix)
            {
                toRemoveArray[ix] = pin.AddObject(this.NamesToRemove[ix]);
            }
            nativeDescription[0].NamesToRemoveCount = (uint)toRemoveArray.Length;
            nativeDescription[0].NamesToRemove = pin.AddBlittable(toRemoveArray);

            kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_NAMED;

            return pin.AddBlittable(nativeDescription);
        }
    }
}