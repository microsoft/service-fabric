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
    /// <para>
    /// Represents a safety check that is currently being performed for a node.
    /// </para>
    /// </summary>
    public abstract class SafetyCheck
    {
        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.SafetyCheck" /> object with the specified kind. 
        /// Can only be invoked from derived classes.
        /// </para>
        /// </summary>
        /// <param name="kind">
        /// <para>The safety check kind.</para>
        /// </param>
        internal protected SafetyCheck(SafetyCheckKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>
        /// Gets the type of the safety check that is being performed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The type of safety check that is being performed.</para>
        /// </value>
        public SafetyCheckKind Kind
        {
            get;
            private set;
        }

        internal static unsafe IList<SafetyCheck> FromNativeList(
           NativeTypes.FABRIC_SAFETY_CHECK_LIST* nativeList)
        {
            var retval = new List<SafetyCheck>();

            var nativeItemArray = (NativeTypes.FABRIC_SAFETY_CHECK*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(FromNative(nativeItem));
            }

            return retval;
        }

        internal static unsafe SafetyCheck FromNative(NativeTypes.FABRIC_SAFETY_CHECK nativeItem)
        {
            var kind = (SafetyCheckKind)nativeItem.Kind;
            switch (kind)
            {
                case SafetyCheckKind.EnsureSeedNodeQuorum:
                    {
                        return SeedNodeSafetyCheck.FromNative(
                            kind,
                            (NativeTypes.FABRIC_SEED_NODE_SAFETY_CHECK*)nativeItem.Value);
                    }

                case SafetyCheckKind.EnsurePartitionQuorum:
                case SafetyCheckKind.WaitForInBuildReplica:
                case SafetyCheckKind.WaitForPrimaryPlacement:
                case SafetyCheckKind.WaitForPrimarySwap:
                case SafetyCheckKind.WaitForReconfiguration:
                case SafetyCheckKind.EnsureAvailability:
                    {
                        return PartitionSafetyCheck.FromNative(
                            kind,
                            (NativeTypes.FABRIC_PARTITION_SAFETY_CHECK*)nativeItem.Value);
                    }

                default:
                    {
                        AppTrace.TraceSource.WriteError(
                                   "SafetyCheck.FromNative",
                                   "Unknown FABRIC_SAFETY_CHECK : {0}",
                                   nativeItem.Kind);

                        return new UnknownSafetyCheck(kind);
                    }
            }
        }
    }
}