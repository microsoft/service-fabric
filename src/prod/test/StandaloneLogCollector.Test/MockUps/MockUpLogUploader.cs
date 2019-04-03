// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using Microsoft.Tracing.Azure;

    class MockUpLogUploader : LogUploaderBase
    {
        public MockUpLogUploader(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        protected override void InternalUploadAsync(LogPackage logPackage, AzureSasKeyCredential credential)
        {
            for (int i = 0; i < 3; i++)
            {
                Task task = Task.Factory.StartNew(() => { });
                this.ReportTaskStart(new UploadTask(task, "hiahia", "heihei"));
            }

            this.IsUploaded = true;
        }

        public bool IsUploaded { get; private set;}
    }
}