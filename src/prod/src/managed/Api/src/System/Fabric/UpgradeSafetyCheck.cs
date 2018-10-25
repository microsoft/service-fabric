// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the safety check that is currently being performed for a node during upgrade.</para>
    /// </summary>
    public abstract class UpgradeSafetyCheck
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.UpgradeSafetyCheck" /> class.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>The kind of the safety check.</para>
        /// </param>
        internal protected UpgradeSafetyCheck(UpgradeSafetyCheckKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Gets the type of the safety check that is being performed.</para>
        /// </summary>
        /// <value>
        /// <para>The type of the safety check that is being performed.</para>
        /// </value>
        public UpgradeSafetyCheckKind Kind
        {
            get;
            private set;
        }

        internal static unsafe IList<UpgradeSafetyCheck> FromNativeList(
           NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_LIST* nativeList)
        {
            var retval = new List<UpgradeSafetyCheck>();

            var nativeItemArray = (NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(FromNative(nativeItem));
            }

            return retval;
        }

        internal static unsafe UpgradeSafetyCheck FromNative(NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK nativeItem)
        {
            var kind = (UpgradeSafetyCheckKind)nativeItem.Kind;
            switch (kind)
            {
                case UpgradeSafetyCheckKind.EnsureSeedNodeQuorum:
                    {
                        return SeedNodeUpgradeSafetyCheck.FromNative(
                            kind,
                            (NativeTypes.FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK*)nativeItem.Value);
                    }

                case UpgradeSafetyCheckKind.EnsurePartitionQuorum:
                case UpgradeSafetyCheckKind.WaitForInbuildReplica:
                case UpgradeSafetyCheckKind.WaitForPrimaryPlacement:
                case UpgradeSafetyCheckKind.WaitForPrimarySwap:
                case UpgradeSafetyCheckKind.WaitForReconfiguration:
                case UpgradeSafetyCheckKind.EnsureAvailability:
                case UpgradeSafetyCheckKind.WaitForResourceAvailability:
                    {
                        return PartitionUpgradeSafetyCheck.FromNative(
                            kind,
                            (NativeTypes.FABRIC_UPGRADE_PARTITION_SAFETY_CHECK*)nativeItem.Value);
                    }

                default:
                    {
                        AppTrace.TraceSource.WriteError(
                                   "UpgradeSafetyCheck.FromNative",
                                   "Unknown FABRIC_UPGRADE_SAFETY_CHECK : {0}",
                                   nativeItem.Kind);

                        return new UnknownUpgradeSafetyCheck(kind);
                    }
            }
        }
    }
}