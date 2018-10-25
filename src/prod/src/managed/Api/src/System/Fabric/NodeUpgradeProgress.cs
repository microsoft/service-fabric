// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using Common.Serialization;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides the outlines for the upgrade progress details of a node.</para>
    /// </summary>
    public sealed class NodeUpgradeProgress
    {
        internal NodeUpgradeProgress()
        {
        }

        /// <summary>
        /// <para>Gets the name of the node having upgrade progress details.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the node having upgrade progress details.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the upgrade phase of the node.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade phase of the node.</para>
        /// </value>
        public NodeUpgradePhase UpgradePhase
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the list of safety checks that Service Fabric is currently performing on the corresponding node.</para>
        /// </summary>
        /// <value>
        /// <para>The list of safety checks that is Service Fabric currently performing on the corresponding node.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public IList<UpgradeSafetyCheck> PendingSafetyChecks
        {
            get;
            internal set;
        }

        internal static unsafe IList<NodeUpgradeProgress> FromNativeList(NativeTypes.FABRIC_NODE_UPGRADE_PROGRESS_LIST* nativeList)
        {
            var retval = new List<NodeUpgradeProgress>();

            var nativeItemArray = (NativeTypes.FABRIC_NODE_UPGRADE_PROGRESS*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(FromNative(nativeItem));
            }

            return retval;
        }

        internal static unsafe NodeUpgradeProgress FromNative(NativeTypes.FABRIC_NODE_UPGRADE_PROGRESS* nativePtr)
        {
            return FromNative(*nativePtr);
        }

        internal static unsafe NodeUpgradeProgress FromNative(NativeTypes.FABRIC_NODE_UPGRADE_PROGRESS nativeItem)
        {
            IList<UpgradeSafetyCheck> pendingSafetyChecks;
            if (nativeItem.PendingSafetyChecks != IntPtr.Zero)
            {
                pendingSafetyChecks = UpgradeSafetyCheck.FromNativeList(
                    (NativeTypes.FABRIC_UPGRADE_SAFETY_CHECK_LIST*)nativeItem.PendingSafetyChecks);
            }
            else
            {
                pendingSafetyChecks = new List<UpgradeSafetyCheck>();
            }

            return new NodeUpgradeProgress()
            {
                NodeName = NativeTypes.FromNativeString(nativeItem.NodeName),
                UpgradePhase = (NodeUpgradePhase)nativeItem.UpgradePhase,
                PendingSafetyChecks = pendingSafetyChecks
            };
        }
    }
}