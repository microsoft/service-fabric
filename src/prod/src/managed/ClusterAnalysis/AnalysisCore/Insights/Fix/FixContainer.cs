// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Fix
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Runtime.Serialization;

    /// <summary>
    /// </summary>
    [DataContract]
    [KnownType(typeof(SimpleFix))]
    public class FixContainer
    {
        [DataMember(IsRequired = true)]
        private List<IFix> rootCauseList = new List<IFix>();

        /// <summary>
        /// Get a Read only list of Fixes in this container.
        /// </summary>
        public IReadOnlyList<IFix> Fixes
        {
            get
            {
                return new ReadOnlyCollection<IFix>(this.rootCauseList);
            }
        }

        /// <summary>
        /// Add a Fix to container
        /// </summary>
        /// <param name="fix"></param>
        internal void Add(IFix fix)
        {
            this.rootCauseList.Add(fix);
        }

        /// <summary>
        /// Clear all fixes.
        /// </summary>
        internal void Clear()
        {
            this.rootCauseList.Clear();
        }
    }
}