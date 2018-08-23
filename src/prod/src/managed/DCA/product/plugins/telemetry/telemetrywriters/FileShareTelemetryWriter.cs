// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using TelemetryAggregation;

    public sealed class FileShareTelemetryWriter : ITelemetryWriter
    {
        private readonly string filePath;
        private TelemetryCollection.LogErrorDelegate logErrorDelegate;

        public FileShareTelemetryWriter(string fileShareDirectory, TelemetryCollection.LogErrorDelegate logErrorDelegate)
        {
            if (!Directory.Exists(fileShareDirectory))
            {
                Directory.CreateDirectory(fileShareDirectory);
            }

            this.filePath = Path.Combine(fileShareDirectory, "tfile.json");
            this.logErrorDelegate = logErrorDelegate;
        }

        public void Dispose()
        {
        }

        void ITelemetryWriter.PushTelemetry(TelemetryCollection telemetryCollection)
        {
            telemetryCollection.SaveToFile(this.filePath, this.logErrorDelegate);
        }
    }
}