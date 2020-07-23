// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System.Fabric.Common;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;

    internal sealed class StoreManager
    {
        private const string TraceType = "FabricUOSStoreManager";
        private const string Version5 = "5.0.0.0";
        private static bool isVersionChangedFrom5To6 = false;
        
        private readonly IReliableStateManager stateManager;       

        internal StoreManager(IReliableStateManager stateManager)
        {
            this.stateManager = stateManager;
        }

        public async Task<string> GetStorageObjectAsync(string key, CancellationToken cancellationToken)
        {
            string result = null;
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Start transaction inside GetStorageObjectAsync.");

                    ConditionalValue<string> data = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        result = data.Value;
                    }
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside GetStorageObjectAsync.");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "GetStorageObjectAsync: {0}", e.ToString());
                throw;
            }

            return result;
        }

        public async Task SetStorageObjectAsync(string key, string data, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter SetStorageObjectAsync.");
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    ConditionalValue<string> existingData = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (existingData.HasValue)
                    {
                        bool success = await clusterStateTable.TryUpdateAsync(
                                           tx,
                                           key,
                                           data,
                                           existingData.Value).ConfigureAwait(false);

                        if (!success)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteWarning(
                                TraceType,
                                "SetStorageObjectAsync: StandAloneCluster data is not updated. The reason is {0}",
                                string.Equals(data, existingData.Value, StringComparison.OrdinalIgnoreCase) ? "the value is not changed." : "to be investigated.");
                            return;
                        }
                    }
                    else
                    {
                        await clusterStateTable.AddAsync(
                            tx,
                            key,
                            data,
                            Constants.DefaultDictionaryTimeout,
                            cancellationToken).ConfigureAwait(false);
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "PersistClusterResourceAsync: {0}", e.ToString());
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit SetStorageObjectAsync.");
        }

        public async Task<StandAloneCluster> GetClusterResourceAsync(string key, CancellationToken cancellationToken)
        {
            StandAloneCluster cluster = null;
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Start transaction inside GetClusterResourceAsync.");

                    var data = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Found persisted cluster resource. Deserializing it.");
                        cluster = DeserializeClusterResource(data.Value);
                    }
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside GetClusterResourceAsync.");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "GetClusterResourceAsync: {0}", e.ToString());
                throw;
            }

            return cluster;
        }

        public async Task PersistClusterResourceAsync(string key, StandAloneCluster cluster, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter PersistClusterResourceAsync.");
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Serialize cluster resource into string format.");
                string clusterString = JsonConvert.SerializeObject(
                    cluster,
                    StandaloneUtility.GetStandAloneClusterSerializerSettings());

                using (var tx = this.stateManager.CreateTransaction())
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter transaction inside PersistClusterResourceAsync");

                    if (string.IsNullOrEmpty(cluster.PersistedClusterString))
                    {
                        await clusterStateTable.AddAsync(
                            tx,
                            key,
                            clusterString,
                            Constants.DefaultDictionaryTimeout,
                            cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        /* This is a workaround for scenarios where clusters were upgraded from 5.x->6.x. During deserealization, we change the version in UOSState from 5.0.0.0 to 6.0.0.0. This is needed for 
                         * newtonsoft deserealization to work with the new assembly versions. Here we revert 6.0.0.0 back to 5.0.0.0 before persisting it so that users can downgrade succesfully from 6.x -> 5.x 
                         * versions. Once 5.x is deprecated, we can remove this workaround and persist 6.0.0.0 in UOS state */
                        if (isVersionChangedFrom5To6)
                        {
                            clusterString = NormalizeClusterResource(clusterString, Version5);
                        }

                        bool success = await clusterStateTable.TryUpdateAsync(
                                           tx,
                                           key,
                                           clusterString,
                                           cluster.PersistedClusterString).ConfigureAwait(false);

                        if (!success)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteWarning(
                                TraceType,
                                "PersistClusterResourceAsync: StandAloneCluster data is not updated. The reason is {0}",
                                string.Equals(clusterString, cluster.PersistedClusterString, StringComparison.OrdinalIgnoreCase) ? "the value is not changed." : "to be investigated.");
                            return;
                        }
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside PersistClusterResourceAsync");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "PersistClusterResourceAsync: {0}", e.ToString());
                throw;
            }
        }

        public async Task PersistCodeUpgradePackageDetailsAsync(string key, CodeUpgradeDetail packageDetails, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter PersistCodeUpgradePackageDetailsAsync.");
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Serialize packageDetails into string format.");
                string packageDetailsString = JsonConvert.SerializeObject(
                    packageDetails,
                    new JsonSerializerSettings
                    {
                        ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                        NullValueHandling = NullValueHandling.Ignore,
                        TypeNameHandling = TypeNameHandling.Objects,
                        TypeNameAssemblyFormat = System.Runtime.Serialization.Formatters.FormatterAssemblyStyle.Full
                    });

                using (var tx = this.stateManager.CreateTransaction())
                {
                    ConditionalValue<string> existingData = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (existingData.HasValue)
                    {
                        bool success = await clusterStateTable.TryUpdateAsync(
                                           tx,
                                           key,
                                           packageDetailsString,
                                           existingData.Value).ConfigureAwait(false);

                        if (!success)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteWarning(
                                TraceType,
                                "SetStorageObjectAsync: PackageDetails data is not updated. The reason is {0}",
                                string.Equals(packageDetailsString, existingData.Value, StringComparison.OrdinalIgnoreCase) ? "the value is not changed." : "to be investigated.");
                            return;
                        }
                    }
                    else
                    {
                        await clusterStateTable.AddAsync(
                            tx,
                            key,
                            packageDetailsString,
                            Constants.DefaultDictionaryTimeout,
                            cancellationToken).ConfigureAwait(false);
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "PersistCodeUpgradePackageDetailsAsync: {0}", e.ToString());
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit PersistCodeUpgradePackageDetailsAsync.");
        }

        public async Task<CodeUpgradeDetail> GetCodeUpgradePackageDetailsAsync(string key, CancellationToken cancellationToken)
        {
            CodeUpgradeDetail codeUpgradePackageDetails = null;
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Start transaction inside GetCodeUpgradePackageDetailsAsync.");

                    var data = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Found persisted codeUpgradePackageDetails. Deserializing it.");
                        codeUpgradePackageDetails = JsonConvert.DeserializeObject<CodeUpgradeDetail>(
                            data.Value,
                            new JsonSerializerSettings
                            {
                                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                                NullValueHandling = NullValueHandling.Ignore,
                                TypeNameHandling = TypeNameHandling.Objects
                            });
                    }
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside GetCodeUpgradePackageDetailsAsync.");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "GetCodeUpgradePackageDetailsAsync: {0}", e.ToString());
                throw;
            }

            return codeUpgradePackageDetails;
        }

        public async Task PersistConfigUpgradeErrorDetailsAsync(string key, ConfigUpgradeErrorDetail errorDetails, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter PersistConfigUpgradeErrorDetailsAsync.");
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Serialize errorDetails into string format.");
                string errorDetailsString = JsonConvert.SerializeObject(
                    errorDetails,
                    new JsonSerializerSettings
                    {
                        ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                        NullValueHandling = NullValueHandling.Ignore,
                        TypeNameHandling = TypeNameHandling.Objects,
                        TypeNameAssemblyFormat = System.Runtime.Serialization.Formatters.FormatterAssemblyStyle.Full
                    });

                using (var tx = this.stateManager.CreateTransaction())
                {
                    ConditionalValue<string> existingData = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (existingData.HasValue)
                    {
                        bool success = await clusterStateTable.TryUpdateAsync(
                                           tx,
                                           key,
                                           errorDetailsString,
                                           existingData.Value).ConfigureAwait(false);

                        if (!success)
                        {
                            UpgradeOrchestrationTrace.TraceSource.WriteWarning(
                                TraceType,
                                "SetStorageObjectAsync: UpgradeErrorDetails data is not updated. The reason is {0}",
                                string.Equals(errorDetailsString, existingData.Value, StringComparison.OrdinalIgnoreCase) ? "the value is not changed." : "to be investigated.");
                            return;
                        }
                    }
                    else
                    {
                        await clusterStateTable.AddAsync(
                            tx,
                            key,
                            errorDetailsString,
                            Constants.DefaultDictionaryTimeout,
                            cancellationToken).ConfigureAwait(false);
                    }

                    await tx.CommitAsync().ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "PersistConfigUpgradeErrorDetailsAsync: {0}", e.ToString());
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit PersistConfigUpgradeErrorDetailsAsync.");
        }

        public async Task<ConfigUpgradeErrorDetail> GetConfigUpgradeErrorDetailsAsync(string key, CancellationToken cancellationToken)
        {
            ConfigUpgradeErrorDetail configUpgradeErrorDetails = null;
            var clusterStateTable = await this.stateManager.GetOrAddAsync<IReliableDictionary<string, string>>(Constants.ClusterReliableDictionaryName);

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Start transaction inside GetConfigUpgradeErrorDetailsAsync.");

                    var data = await clusterStateTable.TryGetValueAsync(tx, key, Constants.DefaultDictionaryTimeout, cancellationToken).ConfigureAwait(false);
                    if (data.HasValue)
                    {
                        UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Found persisted configUpgradeErrorDetails. Deserializing it.");
                        configUpgradeErrorDetails = JsonConvert.DeserializeObject<ConfigUpgradeErrorDetail>(
                            data.Value,
                            new JsonSerializerSettings
                            {
                                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                                NullValueHandling = NullValueHandling.Ignore,
                                TypeNameHandling = TypeNameHandling.Objects
                            });
                    }
                }

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit transaction inside GetConfigUpgradeErrorDetailsAsync.");
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "GetConfigUpgradeErrorDetailsAsync: {0}", e.ToString());
                throw;
            }

            return configUpgradeErrorDetails;
        }

        internal static StandAloneCluster DeserializeClusterResource(string str)
        {
            StandAloneCluster result;
            JsonSerializerSettings settings = StandaloneUtility.GetStandAloneClusterDeserializerSettings();

            try
            {
                result = JsonConvert.DeserializeObject<StandAloneCluster>(str, settings);                
                isVersionChangedFrom5To6 = false;
            }
            catch (JsonSerializationException ex)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "GetClusterResourceAsync: Deserialization error. Error is {0}. Will begin best-effort attempt.", ex.ToString());

                var normalizedString = NormalizeClusterResource(str, Assembly.GetExecutingAssembly().GetName().Version.ToString());
                result = JsonConvert.DeserializeObject<StandAloneCluster>(normalizedString, settings);
                isVersionChangedFrom5To6 = true;

                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "GetClusterResourceAsync: Deserialization succeeds after best-effort normalization.");
            }

            result.PersistedClusterString = str;
            return result;
        }

        internal static string NormalizeClusterResource(string originalValue, string version)
        {
            //// Replace 'Version=X.0.0.0' with 'Version=Y.0.0.0', where Y equals the major ver of the current build, and X != Y.
            //// Build 6.1.3 has introduced a regression that cluster json of a 5.7 cluster fails to be deserialized by 6.x code, so that 5.x->6.x upgrade fails for Standalone.
            //// Commit 79e79f1c0f6f45ae309feab47696cc2834e0afc0 and 44bee71ba37a2f9fc72a61fdad33bb4db94e5c82 are the most suspicious.
            //// Due to tight schedule vs limited dev resource, work-around the issue by runtime data migration.

            var assembliesToNormalize = Constants.AssemblyResolveTable.Keys;
            StringBuilder result = new StringBuilder();
            string typePrefix = "\"$type\"";
            string typeSuffix = "\"";
            string normalizedVersion = "Version=" + version;
            string stringToProcess = originalValue;

            int startIdx = 0;
            do
            {
                startIdx = stringToProcess.IndexOf(typePrefix, StringComparison.OrdinalIgnoreCase);

                if (startIdx == -1)
                {
                    result.Append(stringToProcess);
                }
                else
                {
                    result.Append(stringToProcess.Substring(0, startIdx + typePrefix.Length));

                    int endIdx = stringToProcess.IndexOf(typeSuffix, startIdx + typePrefix.Length, StringComparison.OrdinalIgnoreCase);
                    endIdx = stringToProcess.IndexOf(typeSuffix, endIdx + 1, StringComparison.OrdinalIgnoreCase) + typeSuffix.Length;
                    
                    int typeStringLength = endIdx - startIdx - typePrefix.Length;
                    string typeString = stringToProcess.Substring(startIdx + typePrefix.Length, typeStringLength);
                    string[] typeStringSections = typeString.Split(new char[] { ',' }, StringSplitOptions.None);
                    for (int i = 0; i < typeStringSections.Length; i++)
                    {
                        if (assembliesToNormalize.Any(p => p.Equals(typeStringSections[i].Trim(), StringComparison.OrdinalIgnoreCase)))
                        {
                            typeStringSections[++i] = normalizedVersion;
                        }
                    }

                    result.Append(string.Join(",", typeStringSections));
                    stringToProcess = stringToProcess.Substring(endIdx);
                }
            }
            while (startIdx != -1);

            return result.ToString();
        }
    }
}