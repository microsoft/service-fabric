// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    // Settings related to ETL file based producers
    internal class EtlProducerSettings
    {
        private readonly bool enabled;
        private readonly TimeSpan etlReadInterval;
        private readonly TimeSpan etlDeletionAgeMinutes;
        private readonly WinFabricEtlType serviceFabricEtlType;
        private readonly string etlPath;
        private readonly string etlFilePatterns;
        private readonly List<string> customManifestPaths;
        private readonly Dictionary<string, List<ServiceEtwManifestInfo>> serviceEtwManifests;
        private readonly bool processingWinFabEtlFiles;
        private readonly IEnumerable<Guid> appEtwGuids;
        private readonly string applicationType;

        public EtlProducerSettings(
            bool enabled,
            TimeSpan etlReadInterval,
            TimeSpan etlDeletionAgeMinutes,
            WinFabricEtlType serviceFabricEtlType,
            string etlPath,
            string etlFilePatterns,
            List<string> customManifestPaths,
            Dictionary<string, List<ServiceEtwManifestInfo>> serviceEtwManifests,
            bool processingWinFabEtlFiles,
            IEnumerable<Guid> appEtwGuids,
            string applicationType)
        {
            this.enabled = enabled;
            this.etlReadInterval = etlReadInterval;
            this.etlDeletionAgeMinutes = etlDeletionAgeMinutes;
            this.serviceFabricEtlType = serviceFabricEtlType;
            this.etlPath = etlPath;
            this.etlFilePatterns = etlFilePatterns;
            this.customManifestPaths = customManifestPaths;
            this.serviceEtwManifests = serviceEtwManifests;
            this.processingWinFabEtlFiles = processingWinFabEtlFiles;
            this.appEtwGuids = appEtwGuids;
            this.applicationType = applicationType;
        }

        internal bool Enabled
        {
            get { return this.enabled; }
        }

        internal TimeSpan EtlReadInterval
        {
            get { return this.etlReadInterval; }
        }

        internal TimeSpan EtlDeletionAgeMinutes
        {
            get { return this.etlDeletionAgeMinutes; }
        }

        internal WinFabricEtlType ServiceFabricEtlType
        {
            get { return this.serviceFabricEtlType; }
        }

        internal string EtlPath
        {
            get { return this.etlPath; }
        }

        internal string EtlFilePatterns
        {
            get { return this.etlFilePatterns; }
        }

        internal List<string> CustomManifestPaths
        {
            get { return this.customManifestPaths; }
        }

        internal Dictionary<string, List<ServiceEtwManifestInfo>> ServiceEtwManifests
        {
            get
            {
                return this.serviceEtwManifests;
            }
        }

        internal bool ProcessingWinFabEtlFiles
        {
            get { return this.processingWinFabEtlFiles; }
        }

        internal IEnumerable<Guid> AppEtwGuids
        {
            get { return this.appEtwGuids; }
        }

        internal string ApplicationType
        {
            get { return this.applicationType; }
        }
    }
}