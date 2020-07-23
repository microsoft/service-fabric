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
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    class MockUpLogCollector : LogCollectorBase
    {
        public MockUpLogCollector(TraceLogger traceLogger)
            : base(traceLogger)
        {
        }

        public override void CollectLogs()
        {
            foreach (KeyValuePair<string, string> log in TestUtility.ExistingTestLogs)
            {
                this.AddFileToList(Path.Combine(TestUtility.TestDirectory, log.Value), log.Key);
            }

            foreach (KeyValuePair<string, string> log in TestUtility.GeneratedTestLogs)
            {
                string path = Path.Combine(TestUtility.TestDirectory, log.Value);
                File.Open(path, FileMode.OpenOrCreate).Dispose();
                this.AddFileToList(path, log.Key, isGenerated: true);
            }

            this.IsCollected = true;
        }

        public override void SaveLogs(LogPackage dstPackage)
        {
            dstPackage.LogIndex.FabricLogDirectory = "fabricDirectory";
            dstPackage.LogIndex.PerfCounterDirectory = "perfDirectory";
            base.SaveLogs(dstPackage);
        }

        public bool IsCollected { get; private set; }
    }
}