// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Fabric.Test;
using System.IO;

namespace System.Fabric.Setup
{
    internal class ValidationUtility
    {
        public static HashSet<SectionSetting> GetDeletedKeys(HashSet<SectionSetting> source, HashSet<SectionSetting> target)
        {
            LogHelper.Log("Check if any keys are deleted :");

            if (source == null || source.Count == 0)
            {
                return source;
            }

            if (target == null || target.Count == 0)
            {
                return source;
            }                       

            foreach (SectionSetting setting in target)
            {
                if (source.Contains(setting))
                {
                    source.Remove(setting);
                }
            }

            return source;
        }

        public static HashSet<SectionSetting> GetConfigsFromCSV(string path, bool hasDeprecated)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException(string.Format("Path {0} doesn't exist.", path));
            }

            LogHelper.Log("Load csv file from path {0}:", path);
            HashSet<SectionSetting> settings = new HashSet<SectionSetting>();

            string line = string.Empty;
            using (StreamReader sr = new StreamReader(path))
            {
                sr.ReadLine();
                sr.ReadLine();
                while (sr.Peek() > 0)
                {
                    line = sr.ReadLine();
                    string[] configs = line.Split(',');

                    // Skip FabricTest, FabricRandomTest and DEPRECATED settings
                    if (configs.Length >= 2 && configs[0] != "FabricTest" && configs[0] != "FabricRandomTest")
                    {
                        if (hasDeprecated || (configs.Length >= 6 ? configs[5] != "DEPRECATED" : true))
                        {
                            settings.Add(new SectionSetting(configs[0], configs[1]));
                        }
                    }
                }
            }

            return settings;
        }

        public static Dictionary<string, string> GetUpgradeMatrix(string dataRoot)
        {
            Dictionary<string, string> matrix = new Dictionary<string, string>();
            matrix.Add("v2.2_RTO", Path.Combine(dataRoot, "v2.2_RTO_Configurations.csv"));
            matrix.Add("v2.2_CU2", Path.Combine(dataRoot, "v2.2_CU2_Configurations.csv"));
            matrix.Add("v2.0_CU3", Path.Combine(dataRoot, "v2.0_CU3_Configurations.csv"));

            return matrix;
        }

        /// <summary>
        /// Get the exception list for configuration
        /// If any setting is deleted from existing configurations (v2.2 rto, v2.2 cu2, v2.0 cu3, etc. according to upgrade matrix),
        /// and the setting is not deprecated nor in section FabricTest/FabricRandomTest
        /// It should be added in the exception list if it is expected.
        /// </summary>
        /// <returns>The exception list which contains section name and setting name</returns>
        public static List<SectionSetting> GetExceptionList()
        {            
            List<SectionSetting> exceptionList = new List<SectionSetting>();

            // Add the setting to be skipped
            // Section: Security Setting :ForceSessionRefreshAtExpiration is skipped
            exceptionList.Add(new SectionSetting("Security", "ForceSessionRefreshAtExpiration"));
            exceptionList.Add(new SectionSetting("Security", "ClaimsTokenErrorCloseDelay"));
            exceptionList.Add(new SectionSetting("Transport", "MaxIpcMessageSize"));

            // Repair Manager internal settings that were in 2.2 but the whole feature was not released. They were renamed in 3.0.
            exceptionList.Add(new SectionSetting("RepairManager", "AllowNewRequests"));
            exceptionList.Add(new SectionSetting("RepairManager", "MaxActiveRequests"));
            exceptionList.Add(new SectionSetting("RepairManager", "MaxTotalRequests"));

            // unreliable transport changes enables and disables it based on the existence of behaviors
            exceptionList.Add(new SectionSetting("Federation", "UnreliableChannelEnabled"));
            exceptionList.Add(new SectionSetting("FabricHost", "ActivatorUseUnreliableTransport"));
            exceptionList.Add(new SectionSetting("Hosting", "UnreliableChannelForIPCEnabled"));
            exceptionList.Add(new SectionSetting("Replication", "UnreliableChannelEnabled"));
            exceptionList.Add(new SectionSetting("ClusterManager/Replication", "UnreliableChannelEnabled"));
            exceptionList.Add(new SectionSetting("Naming/Replication", "UnreliableChannelEnabled"));
            exceptionList.Add(new SectionSetting("Failover/Replication", "UnreliableChannelEnabled"));            
            exceptionList.Add(new SectionSetting("FileStoreService/Replication", "UnreliableChannelEnabled"));
            exceptionList.Add(new SectionSetting("RepairManager/Replication", "UnreliableChannelEnabled"));

            // Internal config used by tests being removed
            exceptionList.Add(new SectionSetting("Common", "UseInstanceNameAsNodeId"));
            exceptionList.Add(new SectionSetting("Federation", "InMemoryTransportEnabled"));

            // Store - replaced with "ReplicatedStore", "TombstoneCleanupLimit"
            exceptionList.Add(new SectionSetting("TombstoneCleanupLimit", "TombstoneCleanupLimit"));

            return exceptionList;
        }
    }
}