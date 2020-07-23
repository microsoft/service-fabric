// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric;

    internal class ReplicaSelectorTestHelper
    {
        static long? replicaInstance = 214253L;

        public static string FormatOutput(string output, PartitionSelector partitionSelector)
        {
            return output.Replace("###PartitionID###", ParitionSelectorTestHelper.PartitionID.ToString()).Replace("###ReplicaSelector###", partitionSelector.ToString());
        }


        public enum ReplicaCase
        {
            ReplicaPrimary,
            ReplicaRandomSecondary,
            ReplicaId,
            ReplicaId_NoValue,
            ReplicaRandom
        }


        public static string GetScript(ParitionSelectorTestHelper.PartitionCase partitionCase, ReplicaCase replicaCase)
        {
            string script = ParitionSelectorTestHelper.GetScript(partitionCase);
            switch (replicaCase)
            {
            case ReplicaCase.ReplicaPrimary:
            {
                script += " -ReplicaKindPrimary ";
                break;
            }

            case ReplicaCase.ReplicaRandomSecondary:
            {
                script += " -ReplicaKindRandomSecondary ";
                break;
            }

            case ReplicaCase.ReplicaId:
            {
                script += string.Format(" -ReplicaOrInstanceId {0}", replicaInstance);
                break;
            }

            case ReplicaCase.ReplicaId_NoValue:
            {
                script += string.Format(" -ReplicaOrInstanceId ''");
                break;
            }
            }

            return script;

        }

        public static string FormatOutput(string output, ReplicaSelector replicaSelector)
        {
            return output.Replace("###ReplicaID###", replicaInstance.ToString()).Replace("###ReplicaSelector###", replicaSelector.ToString());
        }

        public static ReplicaSelector GetExpectedReplicaSelector(ParitionSelectorTestHelper.PartitionCase partitionCase, ReplicaCase replicaCase)
        {
            ReplicaSelector result = null;
            PartitionSelector partitionSelector = ParitionSelectorTestHelper.GetExpectedPartitionSelector(partitionCase);

            switch (replicaCase)
            {
            case ReplicaCase.ReplicaPrimary:
            {
                result = ReplicaSelector.PrimaryOf(partitionSelector);
                break;
            }

            case ReplicaCase.ReplicaRandomSecondary:
            {
                result = ReplicaSelector.RandomSecondaryOf(partitionSelector);
                break;
            }

            case ReplicaCase.ReplicaId:
            {
                result = ReplicaSelector.ReplicaIdOf(partitionSelector, replicaInstance.Value);
                break;
            }

            case ReplicaCase.ReplicaId_NoValue:
            {
                result = ReplicaSelector.ReplicaIdOf(partitionSelector, 0);
                break;
            }

            case ReplicaCase.ReplicaRandom:
            {
                result = ReplicaSelector.RandomOf(partitionSelector);
                break;
            }
            }

            return result;
        }
    }
}