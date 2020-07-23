// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.RootCause
{
    using System.Runtime.Serialization;

    /// <summary>
    /// </summary>
    [DataContract]
    public class SimpleRootCause : IRootCause
    {
        /// <summary>
        /// Create an instance of <see cref="SimpleRootCause"/>
        /// </summary>
        /// <param name="msg"></param>
        public SimpleRootCause(string msg)
        {
            this.RootCauseMg = msg;
        }

        /// <inheritdoc />
        public string Impact
        {
            get { throw new System.NotImplementedException(); }
        }

        /// <summary>
        /// </summary>
        public string RootCauseMg { get; private set; }

        /// <inheritdoc />
        public string GetUserFriendlySummary()
        {
            return this.RootCauseMg;
        }
    }
}