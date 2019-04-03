// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Globalization;

    internal class JobImpactData : IJobImpactData
    {
        public JobImpactData()
        {
            AssessedNodes = new Dictionary<string, INode>(StringComparer.OrdinalIgnoreCase);
        }

        public DateTimeOffset Timestamp { get; set; }

        public string JobId { get; set; }

        public int UD { get; set; }

        public JobType JobType { get; set; }

        public JobImpactTranslationMode AssessedImpact { get; set; }

        public IDictionary<string, INode> AssessedNodes { get; set; }
        
        public override string ToString()
        {
            string nodes = string.Join(Environment.NewLine, AssessedNodes.Values);
            string text = string.Format(
                CultureInfo.InvariantCulture,
                "JobId: {0}, UD: {1}, JobType: {2}, Timestamp: {3:o}, AssessedImpact: {4}, Nodes:{5}{6}",
                JobId,
                UD,
                JobType,
                Timestamp,
                AssessedImpact,
                Environment.NewLine,
                nodes);

            return text;
        }
    }
}