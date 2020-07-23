// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.RootCause
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Runtime.Serialization;

    /// <summary>
    /// </summary>
    [DataContract]
    [KnownType(typeof(NodeDown))]
    [KnownType(typeof(SimpleRootCause))]
    public class RootCauseContainer
    {
        [DataMember(IsRequired = true)]
        private List<IRootCause> rootCauseList = new List<IRootCause>();

        /// <summary>
        /// Get a read only list of all Root causes
        /// </summary>
        public IReadOnlyList<IRootCause> RootCauses
        {
            get
            {
                return new ReadOnlyCollection<IRootCause>(this.rootCauseList);
            }
        }

        /// <summary>
        /// Add a root cause to container
        /// </summary>
        /// <param name="rootCause"></param>
        internal void Add(IRootCause rootCause)
        {
            this.rootCauseList.Add(rootCause);
        }

        /// <summary>
        /// Clear all root causes in container
        /// </summary>
        internal void Clear()
        {
            this.rootCauseList.Clear();
        }
    }
}