// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.IO;
using System.Linq;
using System.Threading;

namespace System.Fabric.UpgradeService
{
    using System.Threading.Tasks;

    internal sealed class TestPackageRetriever : IPackageRetriever
    {
        private static readonly TraceType TraceType = new TraceType("Test");

        public Task<ClusterUpgradeCommandParameter> GetUpgradeCommandParamterAsync(
            FabricUpgradeProgress currentUpgradeProgress,
            TimeSpan timeout,
            CancellationToken token)
        {
            Trace.ConsoleWriteLine(TraceType, "In Test");
            ClusterUpgradeCommandParameter parameter = null;
            var tcs = new TaskCompletionSource<ClusterUpgradeCommandParameter>();
            if (Directory.Exists(@"c:\TempTest"))
            {
                parameter = new ClusterUpgradeCommandParameter();
                if (File.Exists(@"c:\TempTest\ClusterManifest.xml"))
                {
                    parameter.ConfigFilePath = @"c:\TempTest\ClusterManifest.xml";
                }

                var files = Directory.EnumerateFiles(@"c:\TempTest", "WindowsFabric.*.msi").ToArray();
                if (files.Count() == 1)
                {
                    var msi = Path.GetFileName(files[0]);
                    var name = Path.GetFileNameWithoutExtension(msi);
                    var version = name.Substring(14);
                    parameter.CodeFilePath = files[0];
                    parameter.CodeVersion = version;
                }

                if (currentUpgradeProgress.TargetCodeVersion == parameter.CodeVersion || 
                    (string.IsNullOrWhiteSpace(parameter.CodeFilePath) && string.IsNullOrWhiteSpace(parameter.ConfigFilePath)))
                {
                    parameter = null;
                }
            }

            if (parameter != null)
            {
                Trace.ConsoleWriteLine(TraceType, "codepath:{0} configPath:{1}", parameter.CodeFilePath, parameter.ConfigFilePath);
            }

            tcs.SetResult(parameter);
            return tcs.Task;
        }
    }
}