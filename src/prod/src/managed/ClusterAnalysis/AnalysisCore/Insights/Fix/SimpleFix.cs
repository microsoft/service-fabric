// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Fix
{
    using System.Runtime.Serialization;

    /// <summary>
    /// </summary>
    [DataContract]
    public class SimpleFix : IFix
    {
        [DataMember(IsRequired = true)]
        private string simpleFixMg;

        /// <summary>
        /// </summary>
        /// <param name="msg"></param>
        public SimpleFix(string msg)
        {
            this.simpleFixMg = msg;
        }

        /// <inheritdoc />
        public string GetUserFriendlySummary()
        {
            return this.simpleFixMg;
        }
    }
}