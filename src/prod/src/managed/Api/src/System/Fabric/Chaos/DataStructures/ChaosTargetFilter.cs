// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;

    /// <summary>
    /// <para> Defines all filters for targeted Chaos faults, for example, faulting only certain node types or faulting only certain applications.
    /// If ChaosTargetFilter is not used, Chaos faults all cluster entities.If ChaosTargetFilter is used, Chaos faults only the entities that meet the ChaosTargetFilter
    /// specification. NodeTypeInclusionList and ApplicationInclusionList allow a union semantics only. It is not possible to specify an intersection
    /// of NodeTypeInclusionList and ApplicationInclusionList. For example, it is not possible to specify "fault this application only when it is on that node type."
    /// Once an entity is included in either NodeTypeInclusionList or ApplicationInclusionList, that entity cannot be excluded using ChaosTargetFilter. Even if
    /// applicationX does not appear in ApplicationInclusionList, in some Chaos iteration applicationX can be faulted because it happens to be on a node of nodeTypeY that is included
    /// in NodeTypeInclusionList.If both NodeTypeInclusionList and ApplicationInclusionList are null or empty, an ArgumentException is thrown.</para>
    /// </summary>
    public sealed class ChaosTargetFilter : ByteSerializable
    {
        private const string EntitySeparator = ",";

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Chaos.DataStructures.ChaosTargetFilter" /> class.</para>
        /// </summary>
        public ChaosTargetFilter()
        {
        }

        /// <summary>
        /// A list of node types to include in Chaos faults.
        /// All types of faults (restart node, restart codepackage, remove replica, restart replica, move primary, and move secondary) are enabled for the nodes of these node types.
        /// If a nodetype (say NodeTypeX) does not appear in the NodeTypeInclusionList, then node level faults (like NodeRestart) will never be enabled for the nodes of
        /// NodeTypeX, but code package and replica faults can still be enabled for NodeTypeX if an application in the ApplicationInclusionList happens to reside on a node of NodeTypeX. 
        /// At most 100 node type names can be included in this list, to increase this number, a config upgrade is required for MaxNumberOfNodeTypesInChaosEntityFilter configuration.
        /// </summary>
        public IList<string> NodeTypeInclusionList { get; set; }

        /// <summary>
        /// A list of application URI's to include in Chaos faults. 
        /// All replicas belonging to services of these applications are amenable to replica faults(restart replica, remove replica, move primary, and move secondary) by Chaos.
        /// Chaos may restart a code package only if the code package hosts replicas of these applications only.
        /// If an application does not appear in this list, it can still be faulted in some Chaos iteration if the application ends up on a node of a node type that is incuded in NodeTypeInclusionList.
        /// However if applicationX is tied to nodeTypeY through placement constraints and applicationX is absent from ApplicationInclusionList and nodeTypeY is absent from NodeTypeInclusionList, then applicationX will never be faulted.
        /// At most 1000 application names can be included in this list, to increase this number, a config upgrade is required for MaxNumberOfApplicationsInChaosEntityFilter configuration.
        /// </summary>
        public IList<string> ApplicationInclusionList { get; set; }

        /// <summary>
        /// Gets a string representation of the ChaosTargetFilter object.
        /// </summary>
        /// <returns>A string representation of the ChaosTargetFilter object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            if (this.NodeTypeInclusionList != null && this.NodeTypeInclusionList.Any())
            {
                sb.AppendFormat(
                    CultureInfo.InvariantCulture,
                    "NodeTypeInclusionList: {0}",
                    string.Join(EntitySeparator, this.NodeTypeInclusionList));
                sb.AppendLine();
            }

            if (this.ApplicationInclusionList != null && this.ApplicationInclusionList.Any())
            {
                sb.AppendFormat(
                    CultureInfo.InvariantCulture,
                    "ApplicationInclusionFilter: {0}",
                    string.Join(EntitySeparator, this.ApplicationInclusionList));
                sb.AppendLine();
            }

            return sb.ToString();
        }

        #region Interop Helpers
        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeEntityFilter = new NativeTypes.FABRIC_CHAOS_CLUSTER_ENTITY_FILTER
            {
                NodeTypeInclusionList = NativeTypes.ToNativeStringList(
                    pinCollection,
                    this.NodeTypeInclusionList),
                ApplicationInclusionList = NativeTypes.ToNativeStringList(
                    pinCollection,
                    this.ApplicationInclusionList),
            };

            return pinCollection.AddBlittable(nativeEntityFilter);
        }

        internal static unsafe ChaosTargetFilter FromNative(IntPtr pointer)
        {
            if (pointer == IntPtr.Zero)
            {
                return null;
            }

            NativeTypes.FABRIC_CHAOS_CLUSTER_ENTITY_FILTER nativeEntityFilter = *(NativeTypes.FABRIC_CHAOS_CLUSTER_ENTITY_FILTER*)pointer;

            IList<string> nodetypeInclusionList = null;

            if (nativeEntityFilter.NodeTypeInclusionList != IntPtr.Zero)
            {
                nodetypeInclusionList =
                    NativeTypes.FromNativeStringList(
                        *(NativeTypes.FABRIC_STRING_LIST*)nativeEntityFilter.NodeTypeInclusionList);
            }

            IList<string> applicationInclusionList = null;

            if (nativeEntityFilter.ApplicationInclusionList != IntPtr.Zero)
            {
                applicationInclusionList =
                    NativeTypes.FromNativeStringList(
                        *(NativeTypes.FABRIC_STRING_LIST*)nativeEntityFilter.ApplicationInclusionList);
            }

            var clusterEntityChaosFilter = new ChaosTargetFilter
                                               {
                                                   NodeTypeInclusionList = 
                                                       nodetypeInclusionList,
                                                   ApplicationInclusionList = 
                                                       applicationInclusionList
                                               };

            return clusterEntityChaosFilter;
        }

        #endregion

        /// <summary>
        /// Reads the state of this object from byte array.
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public override void Read(BinaryReader br)
        {
            var nodetypeCount = br.ReadInt32();
            if (nodetypeCount > 0)
            {
                this.NodeTypeInclusionList = new List<string>();
                for (int i = 0; i < nodetypeCount; i++)
                {
                    this.NodeTypeInclusionList.Add(br.ReadString());
                }
            }

            var appCount = br.ReadInt32();
            if (appCount > 0)
            {
                this.ApplicationInclusionList = new List<string>();
                for (int i = 0; i < appCount; i++)
                {
                    this.ApplicationInclusionList.Add(br.ReadString());
                }
            }
        }

        /// <summary>
        /// Writes the state of this object into a byte array.
        /// </summary>
        /// <param name="bw">A BinaryWriter object.</param>
        public override void Write(BinaryWriter bw)
        {
            bw.Write(this.NodeTypeInclusionList?.Count ?? 0);

            if (this.NodeTypeInclusionList != null)
            {
                foreach (var nodetype in this.NodeTypeInclusionList)
                {
                    bw.Write(nodetype);
                }
            }

            bw.Write(this.ApplicationInclusionList?.Count ?? 0);

            if (this.ApplicationInclusionList != null)
            {
                foreach (var application in this.ApplicationInclusionList)
                {
                    bw.Write(application);
                }
            }
        }
    }
}