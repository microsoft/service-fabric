// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    internal class ConsumerInitializationParameters
    {
        // Application instance ID of the application for which we are collecting data
        private readonly string applicationInstanceId;

        // Name of the section containing the consumer's settings in the cluster manifest 
        private readonly string sectionName;

        // Windows Fabric Node ID
        private readonly string fabricNodeId;

        // Windows Fabric node instance name
        private readonly string fabricNodeInstanceName;

        // Full path to the Windows Fabric log directory
        private readonly string logDirectory;

        // Full path to the Windows Fabric work directory
        private readonly string workDirectory;

        private readonly DiskSpaceManager diskSpaceManager;

        public ConsumerInitializationParameters(
            string applicationInstanceId,
            string sectionName,
            string fabricNodeId,
            string fabricNodeInstanceName,
            string logDirectory,
            string workDirectory,
            DiskSpaceManager diskSpaceManager)
        {
            if (string.IsNullOrEmpty(sectionName))
            {
                throw new ArgumentNullException("sectionName");
            }

            if (string.IsNullOrEmpty(logDirectory))
            {
                throw new ArgumentNullException("logDirectory");
            }

            if (diskSpaceManager == null)
            {
                throw new ArgumentNullException("diskSpaceManager");
            }

            this.applicationInstanceId = applicationInstanceId;
            this.sectionName = sectionName;
            this.fabricNodeId = fabricNodeId;
            this.fabricNodeInstanceName = fabricNodeInstanceName;
            this.logDirectory = logDirectory;
            this.workDirectory = workDirectory;
            this.diskSpaceManager = diskSpaceManager;
        }

        public string ApplicationInstanceId
        {
            get { return this.applicationInstanceId; }
        }

        public string SectionName
        {
            get { return this.sectionName; }
        }

        public string FabricNodeId
        {
            get { return this.fabricNodeId; }
        }

        public string FabricNodeInstanceName
        {
            get { return this.fabricNodeInstanceName; }
        }

        public string LogDirectory
        {
            get { return this.logDirectory; }
        }

        public string WorkDirectory
        {
            get { return this.workDirectory; }
        }

        public DiskSpaceManager DiskSpaceManager
        {
            get { return this.diskSpaceManager; }
        }
    }
}