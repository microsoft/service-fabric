// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric;
    using System;

    class ParitionSelectorTestHelper
    {
        public static Guid PartitionID
        {
            get;
            private set;
        }

        public static Uri ServiceName
        {
            get;
            private set;
        }

        public static string PartitionKey
        {
            get;
            private set;
        }

        public static long PartitionKeyLong
        {
            get;
            private set;
        }

        static ParitionSelectorTestHelper()
        {
            PartitionID = Guid.NewGuid();
            ServiceName = new Uri("http://foo");
            PartitionKey = "PartitionTestKey";
            PartitionKeyLong = 1234L;
        }

        public enum PartitionCase
        {
            PartitionId,
            ServiceNameRandomPartition,
            ServiceNamePartitionSingleton,
            ServiceNamePartitionNamed,
            ServiceNamePartitionUniformedInt
        }

        public static string GetScript(PartitionCase partitionCase)
        {
            string script = "";
            switch (partitionCase)
            {
            case PartitionCase.PartitionId:
            {
                script = string.Format("-ServiceName {0} -PartitionId {1}", ServiceName, PartitionID);
                break;
            }

            case PartitionCase.ServiceNameRandomPartition:
            {
                script = string.Format("-ServiceName {0}", ServiceName);
                break;
            }

            case PartitionCase.ServiceNamePartitionSingleton:
            {
                script = string.Format("-ServiceName {0} -PartitionKindSingleton ", ServiceName);
                break;
            }

            case PartitionCase.ServiceNamePartitionNamed:
            {
                script = string.Format("-ServiceName {0} -PartitionKindNamed -PartitionKey {1}", ServiceName, PartitionKey);
                break;
            }

            case PartitionCase.ServiceNamePartitionUniformedInt:
            {
                script = string.Format("-ServiceName {0} -PartitionKindUniformInt64 -PartitionKey {1}", ServiceName, PartitionKeyLong);
                break;
            }
            }

            return script;
        }

        public static string FormatOutput(string output, PartitionSelector partitionSelector)
        {
            return output.Replace("###PartitionID###", PartitionID.ToString()).Replace("###PartitionSelector###", partitionSelector.ToString());
        }

        public static PartitionSelector GetExpectedPartitionSelector(PartitionCase partitionCase)
        {
            PartitionSelector result = null;
            switch (partitionCase)
            {
            case PartitionCase.PartitionId:
            {
                result = PartitionSelector.PartitionIdOf(ServiceName, PartitionID);
                break;
            }

            case PartitionCase.ServiceNameRandomPartition:
            {
                result = PartitionSelector.RandomOf(ServiceName);
                break;
            }

            case PartitionCase.ServiceNamePartitionSingleton:
            {
                result = PartitionSelector.SingletonOf(ServiceName);
                break;
            }

            case PartitionCase.ServiceNamePartitionNamed:
            {
                result = PartitionSelector.PartitionKeyOf(ServiceName, PartitionKey);
                break;
            }

            case PartitionCase.ServiceNamePartitionUniformedInt:
            {
                result = PartitionSelector.PartitionKeyOf(ServiceName, PartitionKeyLong);
                break;
            }
            }

            return result;
        }
    }
}