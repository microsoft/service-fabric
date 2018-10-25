// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;

    // Data structure representing a single record in the table
    internal class ServicePackageTableRecord
    {
        // Node name
        // This is important for one-box because the same application
        // instance ID can be present on multiple nodes on the machine. 
        // The node name helps distinguish them from each other.
        private readonly string nodeName;

        // Application instance ID
        private readonly string applicationInstanceId;

        // Application rollout version
        private readonly string applicationRolloutVersion;

        // Service package name
        private readonly string servicePackageName;

        // Service package rollout version
        private readonly string serviceRolloutVersion;

        // Run layout root
        private readonly string runLayoutRoot;

        public ServicePackageTableRecord(
            string nodeName,
            string applicationInstanceId,
            string applicationRolloutVersion,
            string servicePackageName,
            string serviceRolloutVersion,
            string runLayoutRoot,
            DateTime latestNotificationTimestamp)
        {
            this.nodeName = nodeName;
            this.applicationInstanceId = applicationInstanceId;
            this.applicationRolloutVersion = applicationRolloutVersion;
            this.servicePackageName = servicePackageName;
            this.serviceRolloutVersion = serviceRolloutVersion;
            this.runLayoutRoot = runLayoutRoot;
            this.LatestNotificationTimestamp = latestNotificationTimestamp;
        }

        public string NodeName
        {
            get { return this.nodeName; }
        }

        public string ApplicationInstanceId
        {
            get { return this.applicationInstanceId; }
        }

        public string ApplicationRolloutVersion
        {
            get { return this.applicationRolloutVersion; }
        }

        public string ServicePackageName
        {
            get { return this.servicePackageName; }
        }

        public string ServiceRolloutVersion
        {
            get { return this.serviceRolloutVersion; }
        }

        public string RunLayoutRoot
        {
            get { return this.runLayoutRoot; }
        }

        /// <summary>
        /// Gets or sets timestamp of the latest notification about this service package.
        /// </summary>
        public DateTime LatestNotificationTimestamp { get; set; }
    }
}