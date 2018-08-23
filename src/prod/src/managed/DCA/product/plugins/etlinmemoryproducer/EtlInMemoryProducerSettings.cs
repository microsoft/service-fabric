// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;

    internal class EtlInMemoryProducerSettings
    {
        private readonly bool enabled;
        private readonly TimeSpan etlReadInterval;
        private readonly TimeSpan etlDeletionAgeMinutes;
        private readonly WinFabricEtlType serviceFabricEtlType;
        private readonly string etlPath;
        private readonly string etlFilePatterns;
        private readonly bool processingWinFabEtlFiles;

        public EtlInMemoryProducerSettings(
            bool enabled,
            TimeSpan etlReadInterval,
            TimeSpan etlDeletionAgeMinutes,
            WinFabricEtlType serviceFabricEtlType,
            string etlPath,
            string etlFilePatterns,
            bool processingWinFabEtlFiles)
        {
            this.enabled = enabled;
            this.etlReadInterval = etlReadInterval;
            this.etlDeletionAgeMinutes = etlDeletionAgeMinutes;
            this.serviceFabricEtlType = serviceFabricEtlType;
            this.etlPath = etlPath;
            this.etlFilePatterns = etlFilePatterns;
            this.processingWinFabEtlFiles = processingWinFabEtlFiles;
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

        internal bool ProcessingWinFabEtlFiles
        {
            get { return this.processingWinFabEtlFiles; }
        }
    }
}