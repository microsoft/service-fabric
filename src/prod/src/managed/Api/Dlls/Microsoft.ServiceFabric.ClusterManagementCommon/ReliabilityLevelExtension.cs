// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum ReliabilityLevel
    {
        Invalid,
        Iron, // The system services replica set count (1/1) and Seed Nodes = 1, specially for dev cluster scenario
        Bronze, // The system services replica set count (3/3) and Seed Nodes = 3
        Silver, // The system services replica set count (3/5) and Seed Nodes = 5
        Gold, // The system services replica set count (5/7) and Seed Nodes = 7
        Platinum // The system services replica set count (5/7) and Seed Nodes >= 9
    }

    public static class ReliabilityLevelExtension
    {
        public static int GetSeedNodeCount(this ReliabilityLevel reliabilityLevel)
        {
            switch (reliabilityLevel)
            {
                case ReliabilityLevel.Iron:
                    return 1;
                case ReliabilityLevel.Bronze:
                    return 3;
                case ReliabilityLevel.Silver:
                    return 5;
                case ReliabilityLevel.Gold:
                    return 7;
                case ReliabilityLevel.Platinum:
                    return 9;
                default:
                    throw new ClusterManagementException(ClusterManagementErrorCode.BadRequest);
            }
        }

        public static ReplicaSetSize GetReplicaSetSize(this ReliabilityLevel reliabilityLevel)
        {
            switch (reliabilityLevel)
            {
                case ReliabilityLevel.Iron:
                    return new ReplicaSetSize()
                    {
                        MinReplicaSetSize = 1,
                        TargetReplicaSetSize = 1
                    };
                case ReliabilityLevel.Bronze:
                    return new ReplicaSetSize()
                    {
                        MinReplicaSetSize = 3,
                        TargetReplicaSetSize = 3
                    };
                case ReliabilityLevel.Silver:
                    return new ReplicaSetSize()
                    {
                        MinReplicaSetSize = 3,
                        TargetReplicaSetSize = 5
                    };
                case ReliabilityLevel.Gold:
                    return new ReplicaSetSize()
                    {
                        MinReplicaSetSize = 5,
                        TargetReplicaSetSize = 7
                    };
                case ReliabilityLevel.Platinum:
                    return new ReplicaSetSize()
                    {
                        MinReplicaSetSize = 5,
                        TargetReplicaSetSize = 7
                    };
                default:
                    throw new ClusterManagementException(ClusterManagementErrorCode.BadRequest);
            }
        }

        public static ReliabilityLevel GetReliabilityLevel(int primaryNodeTypeNodeCount)
        {
            if (primaryNodeTypeNodeCount >= ReliabilityLevel.Platinum.GetSeedNodeCount())
            {
                return ReliabilityLevel.Platinum;
            }
            else if (primaryNodeTypeNodeCount >= ReliabilityLevel.Gold.GetSeedNodeCount())
            {
                return ReliabilityLevel.Gold;
            }
            else if (primaryNodeTypeNodeCount >= ReliabilityLevel.Silver.GetSeedNodeCount())
            {
                return ReliabilityLevel.Silver;
            }
            else if (primaryNodeTypeNodeCount >= ReliabilityLevel.Bronze.GetSeedNodeCount())
            {
                return ReliabilityLevel.Bronze;
            }
            else if (primaryNodeTypeNodeCount >= ReliabilityLevel.Iron.GetSeedNodeCount())
            {
                return ReliabilityLevel.Iron;
            }
            else
            {
                return ReliabilityLevel.Invalid;
            }
        }
    }
}