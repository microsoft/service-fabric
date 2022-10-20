// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using IO;
    using Linq;

    /// <summary>
    /// Helper class for loading objects from json test files.
    /// </summary>
    internal static class JsonTestFileLoader
    {
        public static IPolicyAgentDocumentForTenant LoadPolicyAgentDoc(string file)
        {
            file.Validate("file");

            if (!File.Exists(file))
            {
                return new MockPolicyAgentDocumentForTenant();
            }

            var contents = File.ReadAllText(file);

            var doc = contents.FromJson<MockPolicyAgentDocumentForTenant>();
            return doc;
        }

        public static IList<IRepairTask> LoadRepairTasks(string file)
        {
            file.Validate("file");

            if (!File.Exists(file))
            {
                return new List<IRepairTask>();
            }

            var contents = File.ReadAllText(file);

            var repairTasks = contents.FromJson<JsonInputRepairTasks>().RepairTasks.Cast<IRepairTask>().ToList();
            return repairTasks;
        }

        public static IList<JsonExpectedActions> LoadExpectedActions(string file)
        {
            file.Validate("file");

            var contents = File.ReadAllText(file);

            var idsAndExpectedActions = contents.FromJson<JsonExpectedTestOutput>().Items;

            return idsAndExpectedActions ?? new List<JsonExpectedActions>();
        }
    }
}