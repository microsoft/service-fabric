// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.ImageStore;
    using System.Fabric.JsonSerializerWrapper;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Testability.Client;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Numerics;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Repair;

    [SuppressMessage("Microsoft.Maintainability", "CA1506:AvoidExcessiveClassCoupling", Justification = "Matches product")]
    public class PowerShellFabricClient : FabricClientObject
    {
        private const string TraceSource = "PowershellFabricClient";

        private readonly PowerShellManager powershellManager;
        private readonly string[] gatewayAddresses;
        private readonly SecurityCredentials securityCredentials;

        private IJsonSerializer jsonSerializer;

        public PowerShellFabricClient(
            IEnumerable<string> gatewayAddresses,
            SecurityCredentials securityCredentials,
            FabricClientSettings clientSettings)
        {
            this.gatewayAddresses = gatewayAddresses == null ? new string[0] : gatewayAddresses.ToArray();
            if (this.gatewayAddresses.Length != 0)
            {
                if (securityCredentials != null)
                {
                    this.securityCredentials = securityCredentials;
                }
            }

            // TODO: handle remaining client settings parameters

            this.powershellManager = new PowerShellManager(this.ConnectToCluster);
        }

        public override async Task<OperationResult<DeployedApplication[]>> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<DeployedApplication> queryResults = new List<DeployedApplication>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationNameFilter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedApplicationCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<DeployedApplication>(queryResults, powershellResult);
                });

            return OperationResult<DeployedApplication>.GetOperationResultArray(errorCode, queryResults);
        }

        public override async Task<OperationResult<DeployedApplicationPagedList>> GetDeployedApplicationPagedListAsync(
            PagedDeployedApplicationQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // DeployedApplicationPagedList is derived from type PagedList, which includes the list of ApplicationType and a continuation token
            List<DeployedApplicationPagedList> queryResultsList = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, queryDescription.NodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, queryDescription.ApplicationNameFilter);
                    parameters.Add(PowerShellConstants.ContinuationTokenParameter, queryDescription.ContinuationToken);
                    parameters.Add(PowerShellConstants.MaxResultsParameter, queryDescription.MaxResults);
                    parameters.Add(PowerShellConstants.IncludeHealthState, queryDescription.IncludeHealthState);
                    parameters.Add(PowerShellConstants.GetSinglePage, true);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedApplicationCommand, parameters);

                    // Get the results as List<DeployedApplicationPagedList>
                    PowerShellFabricClient.SelectValidResultTypes<DeployedApplicationPagedList>(queryResultsList, powershellResult);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplicationPagedList>(errorCode, queryResultsList.FirstOrDefault());
        }

        public override async Task<OperationResult<DeployedCodePackage[]>> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<DeployedCodePackage> queryResults = new List<DeployedCodePackage>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestNameFilter);
                    parameters.Add(PowerShellConstants.CodePackageNameParameter, codePackageNameFilter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedCodePackageCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<DeployedCodePackage>(queryResults, powershellResult);
                });

            return OperationResult<DeployedCodePackage>.GetOperationResultArray(errorCode, queryResults);
        }

        public override async Task<OperationResult<DeployedServiceReplica[]>> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<DeployedServiceReplica> queryResults = new List<DeployedServiceReplica>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestNameFilter);
                    if (partitionIdFilter.HasValue)
                    {
                        parameters.Add(PowerShellConstants.PartitionIdParameter, partitionIdFilter.Value);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedReplicaCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<DeployedServiceReplica>(queryResults, powershellResult);
                });

            return OperationResult<DeployedServiceReplica>.GetOperationResultArray(errorCode, queryResults);
        }

        public override async Task<OperationResult<DeployedServicePackage[]>> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<DeployedServicePackage> queryResults = new List<DeployedServicePackage>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestNameFilter);

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedServiceManifestCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<DeployedServicePackage>(queryResults, powershellResult);
                });

            return OperationResult<DeployedServicePackage>.GetOperationResultArray(errorCode, queryResults);
        }

        public override async Task<OperationResult<DeployedServiceType[]>> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<DeployedServiceType> queryResults = new List<DeployedServiceType>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestNameFilter);
                    parameters.Add(PowerShellConstants.ServiceTypeNameParameter, serviceTypeNameFilter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetDeployedServiceTypeCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<DeployedServiceType>(queryResults, powershellResult);
                });

            return OperationResult<DeployedServiceType>.GetOperationResultArray(errorCode, queryResults);
        }

        public override async Task<OperationResult> StartNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            string ipAddressOrFQDN,
            int clusterConnectionPort,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
            delegate
            {
                CommandParameterCollection parameters = new CommandParameterCollection();
                parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                parameters.Add(PowerShellConstants.NodeInstanceIdParameter, nodeInstance);
                parameters.Add(PowerShellConstants.IPAddressOrFQDNParameter, ipAddressOrFQDN);
                parameters.Add(PowerShellConstants.ClusterConnectionPortParameter, clusterConnectionPort);
                parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));
                parameters.Add(PowerShellConstants.CommandCompletionMode, completionMode.ToString());
                return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartNodeCommand, parameters);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StopNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.NodeInstanceIdParameter, nodeInstance);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));
                    parameters.Add(PowerShellConstants.CommandCompletionMode, completionMode.ToString());

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StopNodeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            string nodeName,
            BigInteger nodeInstance,
            bool createFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");
            RestartNodeResult result = null;
            PSDataCollection<PSObject> powershellResult = null;
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.NodeInstanceIdParameter, nodeInstance);

                    if (createFabricDump)
                    {
                        parameters.Add(PowerShellConstants.CreateFabricDumpSwitch, createFabricDump);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));
                    parameters.Add(PowerShellConstants.CommandCompletionMode, completionMode.ToString());
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartNodeCommand, parameters);
                    var psItem = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<RestartNodeResult>();
                    result = psItem.FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<RestartNodeResult>(errorCode, result);
        }

        public override async Task<OperationResult> RestartDeployedCodePackageAsync(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
            delegate
            {
                CommandParameterCollection parameters = new CommandParameterCollection();
                parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestName);
                parameters.Add(PowerShellConstants.ServicePackageActivationIdParameter, servicePackageActivationId);
                parameters.Add(PowerShellConstants.CodePackageNameParameter, codePackageName);
                parameters.Add(PowerShellConstants.CodePackageInstanceIdParameter, codePackageInstanceId);
                parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));
                parameters.Add(PowerShellConstants.CommandCompletionMode, completionMode.ToString());
                return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartCodePackageCommand, parameters);
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ValidateApplicationAsync(
            Uri applicationName,
            TimeSpan maxStabilizationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(applicationName, "applicationName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                              {
                                                                  CommandParameterCollection parameters = new CommandParameterCollection
                                                                                                              {
                                                                                                                      {
                                                                                                                          PowerShellConstants.ApplicationNameParameter,
                                                                                                                          applicationName
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants.MaxStabilizationTimeoutSec,
                                                                                                                          ConvertToSecondsClamped(maxStabilizationTimeout)
                                                                                                                      }
                                                                                                              };
                                                                  return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.ValidateApplicationCommand, parameters);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ValidateServiceAsync(
            Uri serviceName,
            TimeSpan maxStabilizationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(serviceName, "serviceName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                              {
                                                                  CommandParameterCollection parameters = new CommandParameterCollection
                                                                                                              {
                                                                                                                      {
                                                                                                                          PowerShellConstants.ServiceNameParameter,
                                                                                                                          serviceName
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants.MaxStabilizationTimeoutSec,
                                                                                                                          ConvertToSecondsClamped(maxStabilizationTimeout)
                                                                                                                      }
                                                                                                              };
                                                                  return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.ValidateServiceCommand, parameters);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CleanTestStateAsync(
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                              {
                                                                  CommandParameterCollection parameters = new CommandParameterCollection
                                                                                                              {
                                                                                                                      {
                                                                                                                          PowerShellConstants.TimeoutSec,
                                                                                                                          ConvertToSecondsClamped(operationTimeout)
                                                                                                                      }
                                                                                                              };
                                                                  return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.CleanTestStateCommand, parameters);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        // TODO all replica and partition selector overloads
        public override async Task<OperationResult<RestartNodeResult>> RestartNodeAsync(
            ReplicaSelector replicaSelector,
            bool createFabricDump,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            RestartNodeResult result = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
            async delegate
            {
                CommandParameterCollection parameters = new CommandParameterCollection();

                if (createFabricDump)
                {
                    parameters.Add(PowerShellConstants.CreateFabricDumpSwitch, createFabricDump);
                }

                parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));
                parameters.Add(PowerShellConstants.CommandCompletionMode, completionMode);
                AddReplicaSelectorParameters(replicaSelector, parameters);

                powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartNodeCommand, parameters);
                var psItem = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<RestartNodeResult>();
                result = psItem.FirstOrDefault();
            });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<RestartNodeResult>(errorCode, result);
        }

        public override async Task<OperationResult> RestartDeployedCodePackageAsync(
            Uri applicationName,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(applicationName, "applicationName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                              {
                                                                  CommandParameterCollection parameters = new CommandParameterCollection
                                                                                                              {
                                                                                                                      {
                                                                                                                          PowerShellConstants.TimeoutSec,
                                                                                                                          ConvertToSecondsClamped(operationTimeout)
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants.CommandCompletionMode,
                                                                                                                          completionMode
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants
                                                                                                                              .ApplicationNameParameter,
                                                                                                                          applicationName.OriginalString
                                                                                                                      }
                                                                                                              };
                                                                  this.AddReplicaSelectorParameters(replicaSelector, parameters);

                                                                  return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartCodePackageCommand, parameters);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(replicaSelector, "replicaSelector");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                          {
                                                              CommandParameterCollection parameters =
                                                                  new CommandParameterCollection
                                                                      {
                                                                              {
                                                                                  PowerShellConstants.TimeoutSec,
                                                                                  ConvertToSecondsClamped(operationTimeout)
                                                                              },
                                                                              {
                                                                                  PowerShellConstants
                                                                                      .CommandCompletionMode,
                                                                                  completionMode
                                                                              }
                                                                      };
                                                              this.AddReplicaSelectorParameters(replicaSelector, parameters);

                                                              return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartReplicaCommand, parameters);
                                                          }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                         delegate
                                                         {
                                                             CommandParameterCollection parameters =
                                                                 new CommandParameterCollection
                                                                     {
                                                                             {
                                                                                 PowerShellConstants.NodeNameParameter,
                                                                                 nodeName
                                                                             },
                                                                             {
                                                                                 PowerShellConstants.PartitionIdParameter,
                                                                                 partitionId
                                                                             },
                                                                             {
                                                                                 PowerShellConstants.ReplicaOrInstanceIdParameter,
                                                                                 replicaId
                                                                             },
                                                                             {
                                                                                 PowerShellConstants.CommandCompletionMode,
                                                                                 completionMode
                                                                             },
                                                                             {
                                                                                 PowerShellConstants.TimeoutSec,
                                                                                 ConvertToSecondsClamped(operationTimeout)
                                                                             }
                                                                     };

                                                             return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartReplicaCommand, parameters);
                                                         }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveReplicaAsync(
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(replicaSelector, "replicaSelector");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                delegate
                                                {
                                                    CommandParameterCollection parameters =
                                                        new CommandParameterCollection
                                                            {
                                                                    {
                                                                        PowerShellConstants.TimeoutSec,
                                                                        ConvertToSecondsClamped(operationTimeout)
                                                                    },
                                                                    {
                                                                        PowerShellConstants.CommandCompletionMode,
                                                                        completionMode
                                                                    }
                                                            };

                                                    if (forceRemove)
                                                    {
                                                        parameters.Add(PowerShellConstants.ForceRemoveParameter);
                                                    }

                                                    this.AddReplicaSelectorParameters(replicaSelector, parameters);

                                                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveReplicaCommand, parameters);
                                                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RemoveReplicaAsync(
            string nodeName,
            Guid partitionId,
            long replicaId,
            CompletionMode completionMode,
            bool forceRemove,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                delegate
                                                {
                                                    CommandParameterCollection parameters =
                                                        new CommandParameterCollection
                                                            {
                                                                    {
                                                                        PowerShellConstants.NodeNameParameter,
                                                                        nodeName
                                                                    },
                                                                    {
                                                                        PowerShellConstants.PartitionIdParameter,
                                                                        partitionId
                                                                    },
                                                                    {
                                                                        PowerShellConstants.ReplicaOrInstanceIdParameter,
                                                                        replicaId
                                                                    },
                                                                    {
                                                                        PowerShellConstants.CommandCompletionMode,
                                                                        completionMode
                                                                    },
                                                                    {
                                                                        PowerShellConstants.TimeoutSec,
                                                                        ConvertToSecondsClamped(operationTimeout)
                                                                    }
                                                            };

                                                    if (forceRemove)
                                                    {
                                                        parameters.Add(PowerShellConstants.ForceRemoveParameter);
                                                    }

                                                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveReplicaCommand, parameters);
                                                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> InvokeDataLossAsync(
            PartitionSelector partitionSelector,
            DataLossMode datalossMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(partitionSelector, "partitionSelector");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                    delegate
                                    {
                                        CommandParameterCollection parameters = new CommandParameterCollection
                                                                                    {
                                                                                            {
                                                                                                PowerShellConstants.TimeoutSec,
                                                                                                ConvertToSecondsClamped(operationTimeout)
                                                                                            },
                                                                                            {
                                                                                                PowerShellConstants.InvokeDataLossMode,
                                                                                                datalossMode
                                                                                            }
                                                                                    };
                                        this.AddPartitionSelectorParameters(partitionSelector, parameters);

                                        return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.InvokeDataLossCommand, parameters);
                                    }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> RestartPartitionAsync(
            PartitionSelector partitionSelector,
            RestartPartitionMode restartPartitionMode,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(partitionSelector, "partitionSelector");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                        delegate
                        {
                            CommandParameterCollection parameters = new CommandParameterCollection
                                                                        {
                                                                                {
                                                                                    PowerShellConstants.TimeoutSec,
                                                                                    ConvertToSecondsClamped(operationTimeout)
                                                                                },
                                                                                {
                                                                                    PowerShellConstants.RestartPartitionMode,
                                                                                    restartPartitionMode
                                                                                }
                                                                        };
                            this.AddPartitionSelectorParameters(partitionSelector, parameters);

                            return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RestartPartitionCommand, parameters);
                        }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> MovePrimaryReplicaAsync(
            string nodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                        delegate
                        {
                            CommandParameterCollection parameters = new CommandParameterCollection
                                                                        {
                                                                                {
                                                                                    PowerShellConstants.TimeoutSec,
                                                                                    ConvertToSecondsClamped(operationTimeout)
                                                                                },
                                                                                {
                                                                                    PowerShellConstants.NodeNameParameter,
                                                                                    nodeName
                                                                                },
                                                                                {
                                                                                    PowerShellConstants.IgnoreConstraintsParameter,
                                                                                    ignoreConstraints
                                                                                }
                                                                        };
                            this.AddPartitionSelectorParameters(partitionSelector, parameters);

                            return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.MovePrimaryReplicaCommand, parameters);
                        }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> MoveSecondaryReplicaAsync(
            string currentNodeName,
            string newNodeName,
            PartitionSelector partitionSelector,
            bool ignoreConstraints,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            ThrowIf.Null(currentNodeName, "currentNodeName");
            ThrowIf.Null(newNodeName, "newNodeName");
            ThrowIf.Null(partitionSelector, "partitionSelector");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
            delegate
            {
                CommandParameterCollection parameters = new CommandParameterCollection
                                                            {
                                                                    {
                                                                        PowerShellConstants.TimeoutSec,
                                                                        ConvertToSecondsClamped(operationTimeout)
                                                                    },
                                                                    {
                                                                        PowerShellConstants.CurrentSecondaryNodeName,
                                                                        currentNodeName
                                                                    },
                                                                    {
                                                                        PowerShellConstants.NewSecondaryNodeName,
                                                                        newNodeName
                                                                    },
                                                                    {
                                                                        PowerShellConstants.IgnoreConstraintsParameter,
                                                                        ignoreConstraints
                                                                    }
                                                            };
                this.AddPartitionSelectorParameters(partitionSelector, parameters);
                return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.MoveSecondaryReplicaCommand, parameters);
            }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        private void AddReplicaSelectorParameters(ReplicaSelector replicaSelector, CommandParameterCollection parameters)
        {
            if (replicaSelector == null || parameters == null)
            {
                return;
            }

            if (replicaSelector.PowershellParameters.Count > 0)
            {
                foreach (var keyval in replicaSelector.PowershellParameters)
                {
                    if (keyval.Key == PowerShellConstants.ReplicaKind)
                    {
                        parameters.Add(keyval.Value);
                    }
                    else
                    {
                        parameters.Add(keyval.Key, keyval.Value);
                    }
                }
            }

            this.AddPartitionSelectorParameters(replicaSelector.PartitionSelector, parameters);
        }

        private void AddPartitionSelectorParameters(PartitionSelector partitionSelector, CommandParameterCollection parameters)
        {
            if (partitionSelector == null || parameters == null)
            {
                return;
            }

            if (partitionSelector.PowershellParameters.Count > 0)
            {
                foreach (var keyval in partitionSelector.PowershellParameters)
                {
                    if (keyval.Key == PowerShellConstants.PartitionKind)
                    {
                        parameters.Add(keyval.Value);
                    }
                    else
                    {
                        parameters.Add(keyval.Key, keyval.Value);
                    }
                }
            }
        }

        public override async Task<OperationResult> CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationDescription, "applicationDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationDescription.ApplicationName);
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationDescription.ApplicationTypeName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, applicationDescription.ApplicationTypeVersion);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    if (applicationDescription.ApplicationParameters != null && applicationDescription.ApplicationParameters.Count > 0)
                    {
                        Hashtable parameterList = BuildParameterArugument<ApplicationParameter>(applicationDescription.ApplicationParameters);
                        parameters.Add(PowerShellConstants.ApplicationParameter, parameterList);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.NewApplicationCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceAsync(ServiceDescription winFabricServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(winFabricServiceDescription, "winFabricServiceDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = 0;

            errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    StatefulServiceDescription statefulServiceDescription = winFabricServiceDescription as StatefulServiceDescription;
                    StatelessServiceDescription statelessServiceDescription = winFabricServiceDescription as StatelessServiceDescription;
                    PartitionSchemeDescription partitionDescription = winFabricServiceDescription.PartitionSchemeDescription;
                    NamedPartitionSchemeDescription namedPartitionSchemeDescription = partitionDescription as NamedPartitionSchemeDescription;
                    SingletonPartitionSchemeDescription singletonPartitionSchemeDescription = partitionDescription as SingletonPartitionSchemeDescription;
                    UniformInt64RangePartitionSchemeDescription rangePartitionSchemeDescription = partitionDescription as UniformInt64RangePartitionSchemeDescription;

                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, winFabricServiceDescription.ApplicationName);
                    parameters.Add(PowerShellConstants.ServiceNameParameter, winFabricServiceDescription.ServiceName);
                    parameters.Add(PowerShellConstants.ServiceTypeNameParameter, winFabricServiceDescription.ServiceTypeName);

                    if (winFabricServiceDescription.ServicePackageActivationMode != ServicePackageActivationMode.SharedProcess)
                    {
                        parameters.Add(PowerShellConstants.ServicePackageActivationModeParameter, winFabricServiceDescription.ServicePackageActivationMode.ToString());
                    }

                    if (!string.IsNullOrEmpty(winFabricServiceDescription.ServiceDnsName))
                    {
                        parameters.Add(PowerShellConstants.ServiceDnsNameParameter, winFabricServiceDescription.ServiceDnsName);
                    }

                    if (winFabricServiceDescription.ScalingPolicies != null)
                    {
                        parameters.Add(PowerShellConstants.ScalingPolicies, winFabricServiceDescription.ScalingPolicies);
                    }

                    StringBuilder sb = new StringBuilder();

                    if (!string.IsNullOrEmpty(winFabricServiceDescription.PlacementConstraints))
                    {
                        parameters.Add(PowerShellConstants.PlacementConstraintsParameter, winFabricServiceDescription.PlacementConstraints);
                    }

                    if (winFabricServiceDescription.PlacementPolicies.Count > 0)
                    {
                        // string policyArray = string.Join<ServicePlacementPolicyDescription>(",", winFabricServiceDescription.PlacementPolicies.ToArray());
                        foreach (ServicePlacementPolicyDescription policy in winFabricServiceDescription.PlacementPolicies)
                        {
                            switch (policy.Type)
                            {
                                case ServicePlacementPolicyType.InvalidDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementInvalidDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.PreferPrimaryDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementPreferPrimaryDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.RequireDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementRequiredDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.RequireDomainDistribution:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementRequireDomainDistributionPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.NonPartiallyPlaceService:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementNonPartiallyPlaceServicePolicyDescription).ToString());
                                    break;
                                default:
                                    TestabilityTrace.TraceSource.WriteError(TraceSource, "Invalid Placement policy type");
                                    break;
                            }
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.PlacementPolicy, sb.ToString());
                    }

                    if (winFabricServiceDescription.Correlations.Count > 0)
                    {
                        sb.Clear();
                        foreach (ServiceCorrelationDescription correlation in winFabricServiceDescription.Correlations)
                        {
                            sb.AppendFormat("{0},", correlation.ToString());
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.Correlation, sb.ToString());
                    }

                    bool isStateful;

                    if (statefulServiceDescription != null)
                    {
                        isStateful = true;
                        parameters.Add(PowerShellConstants.Stateful);
                        parameters.Add(PowerShellConstants.TargetReplicaSetSize, statefulServiceDescription.TargetReplicaSetSize);
                        parameters.Add(PowerShellConstants.MinimumReplicaSetSize, statefulServiceDescription.MinReplicaSetSize);
                        if (statefulServiceDescription.HasPersistedState)
                        {
                            parameters.Add(PowerShellConstants.HasPersistedStateParameter);
                        }
                    }
                    else
                    {
                        isStateful = false;
                        parameters.Add(PowerShellConstants.Stateless);
                        parameters.Add(PowerShellConstants.InstanceCount, statelessServiceDescription.InstanceCount);
                    }

                    if (winFabricServiceDescription.Metrics.Count > 0)
                    {
                        sb.Clear();
                        foreach (ServiceLoadMetricDescription metric in winFabricServiceDescription.Metrics)
                        {
                            sb.AppendFormat("{0},", (metric as ServiceLoadMetricDescription).ToString(isStateful));
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.Metric, sb.ToString());
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    if (namedPartitionSchemeDescription != null && namedPartitionSchemeDescription.Scheme == PartitionScheme.Named)
                    {
                        parameters.Add(PowerShellConstants.PartitionSchemeNamedParameter);
                        parameters.Add(PowerShellConstants.PartitionNamesParameter, namedPartitionSchemeDescription.PartitionNames.ToArray());
                    }
                    else if (rangePartitionSchemeDescription != null && rangePartitionSchemeDescription.Scheme == PartitionScheme.UniformInt64Range)
                    {
                        parameters.Add(PowerShellConstants.PartitionSchemeUniformInt64Parameter);
                        parameters.Add(PowerShellConstants.PartitionCountParameter, rangePartitionSchemeDescription.PartitionCount);
                        parameters.Add(PowerShellConstants.LowKey, rangePartitionSchemeDescription.LowKey);
                        parameters.Add(PowerShellConstants.HighKey, rangePartitionSchemeDescription.HighKey);
                    }
                    else
                    {
                        parameters.Add(PowerShellConstants.PartitionSchemeSingletonParameter);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.NewServiceCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CreateServiceFromTemplateAsync(
            Uri applicationName,
            Uri serviceName,
            string serviceDnsName,
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationName, "applicationName");
            ThrowIf.Null(serviceName, "serviceName");
            ThrowIf.NullOrEmpty(serviceTypeName, "serviceTypeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.ServiceNameParameter, serviceName);
                    parameters.Add(PowerShellConstants.ServiceTypeNameParameter, serviceTypeName);

                    if (servicePackageActivationMode != ServicePackageActivationMode.SharedProcess)
                    {
                        parameters.Add(PowerShellConstants.ServicePackageActivationModeParameter, servicePackageActivationMode.ToString());
                    }

                    if (!string.IsNullOrWhiteSpace(serviceDnsName))
                    {
                        parameters.Add(PowerShellConstants.ServiceDnsNameParameter, serviceDnsName);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    // Defect 1237578
                    // parameters.Add(PowerShellConstants.InitializationDataParameter, initializationData);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.NewServiceFromTemplateCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(deleteApplicationDescription, "deleteApplicationDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, deleteApplicationDescription.ApplicationName);
                    parameters.Add(PowerShellConstants.ForceParameter);
                    if (deleteApplicationDescription.ForceDelete)
                    {
                        parameters.Add(PowerShellConstants.ForceRemoveParameter);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveApplicationCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(deleteServiceDescription, "deleteServiceDescription");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceNameParameter, deleteServiceDescription.ServiceName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    if (deleteServiceDescription.ForceDelete)
                    {
                        parameters.Add(PowerShellConstants.ForceRemoveParameter);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveServiceCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateServiceAsync(Uri name, ServiceUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceNameParameter, name);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);

                    StatefulServiceUpdateDescription statefulServiceDescription = description as StatefulServiceUpdateDescription;
                    StatelessServiceUpdateDescription statelessServiceDescription = description as StatelessServiceUpdateDescription;

                    bool isStateful = false;

                    if (statefulServiceDescription != null)
                    {
                        parameters.Add(PowerShellConstants.Stateful);
                        parameters.Add(PowerShellConstants.TargetReplicaSetSize, statefulServiceDescription.TargetReplicaSetSize);
                        parameters.Add(PowerShellConstants.ReplicaRestartWaitDurationParameter, statefulServiceDescription.ReplicaRestartWaitDuration);
                        parameters.Add(PowerShellConstants.QuorumLossWaitDurationParameter, statefulServiceDescription.QuorumLossWaitDuration);
                        isStateful = true;
                    }
                    else
                    {
                        parameters.Add(PowerShellConstants.Stateless);
                        parameters.Add(PowerShellConstants.InstanceCount, statelessServiceDescription.InstanceCount);
                    }

                    if (description.ScalingPolicies != null)
                    {
                        parameters.Add(PowerShellConstants.ScalingPolicies, description.ScalingPolicies);
                    }

                    if (!string.IsNullOrEmpty(description.PlacementConstraints))
                    {
                        parameters.Add(PowerShellConstants.PlacementConstraintsParameter, description.PlacementConstraints);
                    }

                    StringBuilder sb = new StringBuilder();

                    if (description.PlacementPolicies.Count > 0)
                    {
                        // string policyArray = string.Join<ServicePlacementPolicyDescription>(",", winFabricServiceDescription.PlacementPolicies.ToArray());
                        foreach (ServicePlacementPolicyDescription policy in description.PlacementPolicies)
                        {
                            switch (policy.Type)
                            {
                                case ServicePlacementPolicyType.InvalidDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementInvalidDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.PreferPrimaryDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementPreferPrimaryDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.RequireDomain:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementRequiredDomainPolicyDescription).ToString());
                                    break;
                                case ServicePlacementPolicyType.RequireDomainDistribution:
                                    sb.AppendFormat("{0},", (policy as ServicePlacementRequireDomainDistributionPolicyDescription).ToString());
                                    break;
                                default:
                                    TestabilityTrace.TraceSource.WriteError(TraceSource, "Invalid Placement policy type");
                                    break;
                            }
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.PlacementPolicy, sb.ToString());
                    }

                    if (description.Correlations.Count > 0)
                    {
                        sb.Clear();
                        foreach (ServiceCorrelationDescription correlation in description.Correlations)
                        {
                            sb.AppendFormat("{0},", correlation.ToString());
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.Correlation, sb.ToString());
                    }

                    if (description.Metrics.Count > 0)
                    {
                        sb.Clear();
                        foreach (ServiceLoadMetricDescription metric in description.Metrics)
                        {
                            sb.AppendFormat("{0},", metric.ToString(isStateful));
                        }

                        if (sb.Length > 0)
                        {
                            sb.Length--;
                        }

                        parameters.Add(PowerShellConstants.Metric, sb.ToString());
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UpdateServiceCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress realProgress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(realProgress, "realProgress");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, realProgress.ApplicationName);
                    parameters.Add(PowerShellConstants.UpgradeDomainNameParameter, realProgress.NextUpgradeDomain);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.ResumeServiceFabricApplicationUpgradeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.EnableServiceFabricNode, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    if (deactivationIntent == NodeDeactivationIntent.RemoveData)
                    {
                        parameters.Add(PowerShellConstants.Intent, NodeDeactivationIntent.RemoveData);
                    }
                    else if (deactivationIntent == NodeDeactivationIntent.Pause)
                    {
                        parameters.Add(PowerShellConstants.Intent, NodeDeactivationIntent.Pause);
                    }
                    else if (deactivationIntent == NodeDeactivationIntent.Restart)
                    {
                        parameters.Add(PowerShellConstants.Intent, NodeDeactivationIntent.Restart);
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Invalid NodeDeactivation intent supplied: {0}", deactivationIntent));
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.DisableServiceFabricNode, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(string buildPath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await ProvisionApplicationAsync(new ProvisionApplicationTypeDescription(buildPath), timeout, cancellationToken);
        }

        public override async Task<OperationResult> ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(description, "description");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    if (description is ExternalStoreProvisionApplicationTypeDescription)
                    {
                        var desc = (ExternalStoreProvisionApplicationTypeDescription)description;
                        parameters.Add(PowerShellConstants.ApplicationPackageDownloadUriParameter, desc.ApplicationPackageDownloadUri);
                        parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, desc.ApplicationTypeName);
                        parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, desc.ApplicationTypeVersion);
                    }
                    else if (description is ProvisionApplicationTypeDescription)
                    {
                        var desc = (ProvisionApplicationTypeDescription)description;
                        parameters.Add(PowerShellConstants.ApplicationPathParameter, desc.BuildPath);
                        parameters.Add(PowerShellConstants.ApplicationPackageCleanupPolicyParameter, desc.ApplicationPackageCleanupPolicy);
                    }

                    if (description.Async)
                    {
                        parameters.Add(PowerShellConstants.AsyncSwitchParameter);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RegisterApplicationTypeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.UnprovisionApplicationAsync(new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout, cancellationToken);
        }

        public override async Task<OperationResult> UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(description, "description");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, description.ApplicationTypeName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, description.ApplicationTypeVersion);
                    if (description.Async)
                    {
                        parameters.Add(PowerShellConstants.AsyncSwitchParameter);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UnregisterApplicationTypeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> ProvisionFabricAsync(string codeFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    AddCodeAndConfigPathParameters(parameters, codeFilePath, clusterManifestFilePath);

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RegisterClusterPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    AddCodeAndConfigVersionParameters(parameters, codeVersion, configVersion);

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UnregisterServiceFabricClusterPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpgradeFabricAsync(FabricUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    AddCodeAndConfigVersionParameters(parameters, description.TargetCodeVersion, description.TargetConfigVersion);

                    RollingUpgradePolicyDescription rollingUpgradePolicy = description.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                    if (rollingUpgradePolicy != null)
                    {
                        AddTimeParameter(parameters, PowerShellConstants.ReplicaQuorumTimeoutSecParameter, rollingUpgradePolicy.UpgradeReplicaSetCheckTimeout);
                        switch (rollingUpgradePolicy.UpgradeMode)
                        {
                            case RollingUpgradeMode.Monitored:
                                parameters.Add(PowerShellConstants.MonitoredParameter);
                                break;
                            case RollingUpgradeMode.UnmonitoredManual:
                                parameters.Add(PowerShellConstants.UnmonitoredManualParameter);
                                break;
                            default:
                                parameters.Add(PowerShellConstants.UnmonitoredAutoParameter);
                                break;
                        }

                        if (rollingUpgradePolicy.ForceRestart)
                        {
                            parameters.Add(PowerShellConstants.RestartProcessSwitch);
                        }

                        MonitoredRollingFabricUpgradePolicyDescription monitoredUpgradePolicy = rollingUpgradePolicy as MonitoredRollingFabricUpgradePolicyDescription;
                        if (monitoredUpgradePolicy != null)
                        {
                            parameters.Add(PowerShellConstants.FailureActionParameter, monitoredUpgradePolicy.MonitoringPolicy.FailureAction);
                            AddTimeParameter(parameters, PowerShellConstants.HealthCheckRetryTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckRetryTimeout);
                            AddTimeParameter(parameters, PowerShellConstants.HealthCheckWaitDurationSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckWaitDuration);
                            AddTimeParameter(parameters, PowerShellConstants.UpgradeDomainTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.UpgradeDomainTimeout);
                            AddTimeParameter(parameters, PowerShellConstants.UpgradeTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.UpgradeTimeout);
                            AddTimeParameter(parameters, PowerShellConstants.HealthCheckStableDurationSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckStableDuration);

                            ClusterHealthPolicy healthPolicy = monitoredUpgradePolicy.HealthPolicy;
                            if (healthPolicy != null)
                            {
                                parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                                parameters.Add(PowerShellConstants.MaxPercentUnhealthyNodesParameter, healthPolicy.MaxPercentUnhealthyNodes);
                                parameters.Add(PowerShellConstants.MaxPercentUnhealthyApplicationsParameter, healthPolicy.MaxPercentUnhealthyApplications);
                                parameters.Add(PowerShellConstants.ApplicationTypeHealthPolicyMapParameter, healthPolicy.ApplicationTypeHealthPolicyMap);
                            }

                            if (monitoredUpgradePolicy.ApplicationHealthPolicyMap.Count > 0)
                            {
                                parameters.Add(PowerShellConstants.ApplicationHealthPolicyMapParameter, monitoredUpgradePolicy.ApplicationHealthPolicyMap);
                            }
                        }
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartServiceFabricClusterUpgradeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpgradeApplicationAsync(ApplicationUpgradeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(description, "description");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, description.ApplicationName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, description.TargetApplicationTypeVersion);

                    RollingUpgradePolicyDescription upgradePolicyDescription = description.UpgradePolicyDescription as RollingUpgradePolicyDescription;

                    switch (upgradePolicyDescription.UpgradeMode)
                    {
                        case RollingUpgradeMode.Monitored:
                            parameters.Add(PowerShellConstants.MonitoredParameter);
                            break;
                        case RollingUpgradeMode.UnmonitoredManual:
                            parameters.Add(PowerShellConstants.UnmonitoredManualParameter);
                            break;
                        default:
                            parameters.Add(PowerShellConstants.UnmonitoredAutoParameter);
                            break;
                    }

                    BuildHealthPolicyParameters(ref parameters, upgradePolicyDescription);
                    if (description.ApplicationParameters != null && description.ApplicationParameters.Count > 0)
                    {
                        Hashtable parameterList = BuildParameterArugument<ApplicationParameter>(description.ApplicationParameters);
                        parameters.Add(PowerShellConstants.ApplicationParameter, parameterList);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartServiceFabricApplicationUpgradeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, description.ApplicationName);
                    BuildApplicationHealthPolicyParameters(ref parameters, description.HealthPolicy);
                    parameters.Add(PowerShellConstants.UpgradeModeParameter, description.UpgradeMode);
                    if (description.FailureAction != UpgradeFailureAction.Invalid)
                    {
                        parameters.Add(PowerShellConstants.FailureActionParameter, description.FailureAction);
                    }

                    if (description.ForceRestart.HasValue && description.ForceRestart.Value)
                    {
                        parameters.Add(PowerShellConstants.RestartProcessSwitch, description.ForceRestart.Value);
                    }

                    AddTimeParameter(parameters, PowerShellConstants.ReplicaQuorumTimeoutSecParameter, description.UpgradeReplicaSetCheckTimeout);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckRetryTimeoutSecParameter, description.HealthCheckRetryTimeout);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckStableDurationSecParameter, description.HealthCheckStableDuration);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckWaitDurationSecParameter, description.HealthCheckWaitDuration);
                    AddTimeParameter(parameters, PowerShellConstants.UpgradeDomainTimeoutSecParameter, description.UpgradeDomainTimeout);
                    AddTimeParameter(parameters, PowerShellConstants.UpgradeTimeoutSecParameter, description.UpgradeTimeout);

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UpdateServiceFabricApplicationUpgradeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        private static void BuildApplicationHealthPolicyParameters(ref CommandParameterCollection parameters, ApplicationHealthPolicy healthPolicy)
        {
            if (healthPolicy != null)
            {
                parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                parameters.Add(PowerShellConstants.MaxPercentUnhealthyDeployedApplicationsParameter, healthPolicy.MaxPercentUnhealthyDeployedApplications);
                parameters.Add(PowerShellConstants.DefaultServiceTypeHealthPolicyParameter, MakeServiceTypeHealthPolicyString(healthPolicy.DefaultServiceTypeHealthPolicy));

                if (healthPolicy.ServiceTypeHealthPolicyMap.Count > 0)
                {
                    Hashtable policyMap = new Hashtable();
                    foreach (KeyValuePair<string, ServiceTypeHealthPolicy> e in healthPolicy.ServiceTypeHealthPolicyMap)
                    {
                        policyMap.Add(e.Key, MakeServiceTypeHealthPolicyString(e.Value));
                    }

                    parameters.Add(PowerShellConstants.ServiceTypeHealthPolicyMapParameter, policyMap);
                }
            }
        }

        private static void BuildHealthPolicyParameters(ref CommandParameterCollection parameters, RollingUpgradePolicyDescription description)
        {
            if (description != null)
            {
                AddTimeParameter(parameters, PowerShellConstants.ReplicaQuorumTimeoutSecParameter, description.UpgradeReplicaSetCheckTimeout);

                if (description.ForceRestart)
                {
                    parameters.Add(PowerShellConstants.RestartProcessSwitch);
                }

                MonitoredRollingApplicationUpgradePolicyDescription monitoredUpgradePolicy = description as MonitoredRollingApplicationUpgradePolicyDescription;
                if (monitoredUpgradePolicy != null)
                {
                    parameters.Add(PowerShellConstants.FailureActionParameter, monitoredUpgradePolicy.MonitoringPolicy.FailureAction);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckRetryTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckRetryTimeout);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckWaitDurationSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckWaitDuration);
                    AddTimeParameter(parameters, PowerShellConstants.HealthCheckStableDurationSecParameter, monitoredUpgradePolicy.MonitoringPolicy.HealthCheckStableDuration);
                    AddTimeParameter(parameters, PowerShellConstants.UpgradeDomainTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.UpgradeDomainTimeout);
                    AddTimeParameter(parameters, PowerShellConstants.UpgradeTimeoutSecParameter, monitoredUpgradePolicy.MonitoringPolicy.UpgradeTimeout);

                    ApplicationHealthPolicy healthPolicy = monitoredUpgradePolicy.HealthPolicy;
                    BuildApplicationHealthPolicyParameters(ref parameters, healthPolicy);
                }
            }
        }

        public override async Task<OperationResult<NodeList>> GetNodeListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Ignore continuation token
            NodeList nodeList = new NodeList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ContinuationTokenParameter, queryDescription.ContinuationToken);
                    parameters.Add(PowerShellConstants.MaxResultsParameter, queryDescription.MaxResults);
                    // We always add this as true because this will be what REST and managed returns, so it makes testing a lot easier.
                    parameters.Add(PowerShellConstants.GetSinglePage, true);
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetNodeCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<Node>();
                    foreach (Node item in psItems)
                    {
                        nodeList.Add(item);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeList>(errorCode, nodeList);
        }

        public override async Task<OperationResult<ServiceReplicaList>> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var queryResults = new ServiceReplicaList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionIdParameter, partitionId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetReplicaCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<Replica>();
                    foreach (Replica item in psItems)
                    {
                        queryResults.Add(item);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceReplicaList>(errorCode, queryResults);
        }

        public override async Task<OperationResult<ServicePartitionList>> GetPartitionListAsync(Uri serviceName, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(serviceName, "serviceName");
            var queryResults = new ServicePartitionList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceNameParameter, serviceName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<Partition>();
                    foreach (Partition item in psItems)
                    {
                        queryResults.Add(item);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServicePartitionList>(errorCode, queryResults);
        }

        public override async Task<OperationResult<ApplicationType[]>> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<ApplicationType> queryResults = new List<ApplicationType>();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationTypeNameFilter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    var psItems = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetApplicationTypeCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<ApplicationType>(queryResults, psItems);
                });

            return OperationResult<ApplicationType>.GetOperationResultArray(errorCode, queryResults);
        }

        // This is used by QueryCommandsDispatcher's associated test. QueryCommandsDispatcher does not call this directly, but rather,
        // this is one of 3 options it chooses when performing a test. The other options are the REST and managed pathways also in this project.
        public override async Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await GetApplicationTypePagedListAsync(queryDescription, timeout, cancellationToken, false);
        }

        public override async Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken, bool getPagedResults = false)
        {
            return await GetApplicationTypePagedListAsync(queryDescription, timeout, cancellationToken, true, getPagedResults);
        }

        public override async Task<OperationResult<ApplicationList>> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Ignore continuation token, PowerShell gets all results
            var queryResults = new ApplicationList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationQueryDescription.ApplicationNameFilter);
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationQueryDescription.ApplicationTypeNameFilter);
                    parameters.Add(PowerShellConstants.ApplicationDefinitionKindFilterParameter, applicationQueryDescription.ApplicationDefinitionKindFilter);
                    parameters.Add(PowerShellConstants.ExcludeApplicationParametersParameter, applicationQueryDescription.ExcludeApplicationParameters);
                    parameters.Add(PowerShellConstants.ContinuationTokenParameter, applicationQueryDescription.ContinuationToken);
                    parameters.Add(PowerShellConstants.MaxResultsParameter, applicationQueryDescription.MaxResults);
                    // We always add this as true because this will be what REST and managed returns, so it makes testing a lot easier.
                    parameters.Add(PowerShellConstants.GetSinglePage, true);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetApplicationCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<Application>();
                    foreach (Application item in psItems)
                    {
                        queryResults.Add(item);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationList>(errorCode, queryResults);
        }

        public override async Task<OperationResult<ServiceList>> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(serviceQueryDescription.ApplicationName, "applicationName");

            // Ignore continuation token
            var queryResults = new ServiceList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(new CommandParameter(PowerShellConstants.ApplicationNameParameter, serviceQueryDescription.ApplicationName.OriginalString));
                    parameters.Add(new CommandParameter(PowerShellConstants.ServiceNameParameter, serviceQueryDescription.ServiceNameFilter));
                    parameters.Add(new CommandParameter(PowerShellConstants.ServiceTypeNameParameter, serviceQueryDescription.ServiceTypeNameFilter));
                    parameters.Add(PowerShellConstants.ContinuationTokenParameter, serviceQueryDescription.ContinuationToken);
                    if (serviceQueryDescription.MaxResults.HasValue)
                    {
                        parameters.Add(PowerShellConstants.MaxResultsParameter, serviceQueryDescription.MaxResults.Value);
                    }
                    // We add this as true because this will be what REST and managed returns, so it makes testing a lot easier.
                    parameters.Add(PowerShellConstants.GetSinglePage, true);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<Service>();
                    foreach (Service item in psItems)
                    {
                        queryResults.Add(item);
                    }
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceList>(errorCode, queryResults);
        }

        public override async Task<OperationResult> RemoveNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(clusterManifestPath, "clusterManifestPath");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
                    parameters.Add(PowerShellConstants.ComputerNameParameter, computerName);
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveNodeConfigurationCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<string>> GetClusterManifestAsync(ClusterManifestQueryDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string clusterManifest = string.Empty;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<string> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    if (!string.IsNullOrEmpty(description.ClusterManifestVersion))
                    {
                        parameters.Add(PowerShellConstants.ClusterManifestVersionParameter, description.ClusterManifestVersion);
                    }

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetClusterManifestCommand, parameters);
                    clusterManifest = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<string>().FirstOrDefault();
                });

            if (string.IsNullOrEmpty(string.Format(CultureInfo.InvariantCulture, clusterManifest)))
            {
                throw new InvalidOperationException("Empty cluster manifest returned");
            }

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(clusterManifest);
            }

            return operationResult;
        }

        public override async Task<OperationResult<string>> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");
            ThrowIf.NullOrEmpty(applicationTypeVersion, "applicationTypeVersion");
            ThrowIf.NullOrEmpty(serviceManifestName, "serviceManifestName");

            string serviceManifest = string.Empty;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<string> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationTypeName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, applicationTypeVersion);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestName);
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceManifestCommand, parameters);
                    serviceManifest = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<string>().FirstOrDefault();
                });

            if (string.IsNullOrEmpty(string.Format(CultureInfo.InvariantCulture, serviceManifest)))
            {
                throw new InvalidOperationException("Empty service manifest was returned");
            }

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(serviceManifest);
            }

            return operationResult;
        }

        public override async Task<OperationResult<NodeLoadInformation>> GetNodeLoadAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            NodeLoadInformation nodeLoad = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetNodeLoadInformationCommand, parameters);
                    nodeLoad = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<NodeLoadInformation>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeLoadInformation>(errorCode, nodeLoad);
        }

        public override async Task<OperationResult<ClusterLoadInformation>> GetClusterLoadAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            ClusterLoadInformation clusterLoad = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetClusterLoadInformationCommand, parameters);
                    clusterLoad = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ClusterLoadInformation>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterLoadInformation>(errorCode, clusterLoad);
        }

        public override async Task<OperationResult<ApplicationLoadInformation>> GetApplicationLoadAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationName, "applicationName");

            ApplicationLoadInformation applicationLoad = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricApplicationLoadInformationCommand, parameters);
                    applicationLoad = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ApplicationLoadInformation>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationLoadInformation>(errorCode, applicationLoad);
        }

        public override async Task<OperationResult<ReplicaLoadInformation>> GetReplicaLoadAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(partitionId.ToString(), "partitionId");
            ThrowIf.NullOrEmpty(replicaId.ToString(), "replicaId");

            ReplicaLoadInformation replicaLoad = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionIdParameter, partitionId);
                    parameters.Add(PowerShellConstants.ReplicaOrInstanceIdParameter, replicaId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetReplicaLoadInformationCommand, parameters);
                    replicaLoad = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ReplicaLoadInformation>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaLoadInformation>(errorCode, replicaLoad);
        }

        public override async Task<OperationResult<PartitionLoadInformation>> GetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(partitionId.ToString(), "partitionId");

            PartitionLoadInformation partitionLoad = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionIdParameter, partitionId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionLoadInformationCommand, parameters);
                    partitionLoad = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<PartitionLoadInformation>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionLoadInformation>(errorCode, partitionLoad);
        }

        public override async Task<OperationResult<string>> InvokeEncryptSecretAsync(string certificateThumbprint, string certificateStoreLocation, string text, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(certificateThumbprint, "certificateThumbprint");
            ThrowIf.NullOrEmpty(certificateStoreLocation, "certificateStoreLocation");
            ThrowIf.NullOrEmpty(text, "text");

            string invokeResult = string.Empty;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<string> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.CertificateThumbprintParameter, certificateThumbprint);
                    parameters.Add(PowerShellConstants.CertificateStoreLocationParameter, certificateStoreLocation);
                    parameters.Add(PowerShellConstants.TextParameter, text);
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.InvokeEncryptSecretCommand, parameters);
                    invokeResult = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<string>().FirstOrDefault();
                });

            if (string.IsNullOrEmpty(string.Format(CultureInfo.InvariantCulture, invokeResult)))
            {
                throw new InvalidOperationException("Empty invoke result returned");
            }

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(invokeResult);
            }

            return operationResult;
        }

        public override async Task<OperationResult> NewNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(clusterManifestPath, "clusterManifestPath");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
                    parameters.Add(PowerShellConstants.ComputerNameParameter, computerName);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.NewNodeConfigurationCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<bool>> TestClusterConnectionAsync(CancellationToken cancellationToken)
        {
            bool success = false;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<bool> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.TestClusterConnectionCommand, parameters);
                    foreach (PSObject item in powershellResult)
                    {
                        success = (bool)item.BaseObject;
                    }
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(success);
            }

            return operationResult;
        }

        public override async Task<OperationResult<bool>> TestClusterManifestAsync(string clusterManifestPath, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(clusterManifestPath, "clusterManifestPath");

            bool success = false;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<bool> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.TestClusterManifestCommand, parameters);
                    foreach (PSObject item in powershellResult)
                    {
                        success = (bool)item.BaseObject;
                    }
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(success);
            }

            return operationResult;
        }

        public override async Task<OperationResult<Replica>> GetServicePartitionReplicaAsync(Guid partitionId, long id, bool isStateful, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<Replica> queryResults = new List<Replica>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionIdParameter, partitionId);
                    parameters.Add(PowerShellConstants.ReplicaOrInstanceIdParameter, id);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetReplicaCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<Replica>(queryResults, powershellResult);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Replica>(errorCode, queryResults.First());
        }

        public override async Task<OperationResult> UpdateNodeConfigurationAsync(string clusterManifestPath, string computerName, string userName, string password, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(clusterManifestPath, "clusterManifestPath");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
                    parameters.Add(PowerShellConstants.ComputerNameParameter, computerName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UpdateNodeConfigurationCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<Node>> GetNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            List<Node> queryResults = new List<Node>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetNodeCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<Node>(queryResults, powershellResult);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Node>(errorCode, queryResults.FirstOrDefault());
        }

        public override async Task<OperationResult<Application>> GetApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationName, "applicationName");

            List<Application> queryResults = new List<Application>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, applicationName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetApplicationCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<Application>(queryResults, powershellResult);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<Application>(errorCode, queryResults.FirstOrDefault());
        }

        public override async Task<OperationResult<ApplicationType>> GetApplicationTypeAsync(string applicationTypeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");

            List<ApplicationType> queryResults = new List<ApplicationType>();
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationTypeName);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetApplicationTypeCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<ApplicationType>(queryResults, powershellResult);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationType>(errorCode, queryResults.FirstOrDefault());
        }

        public async override Task<OperationResult<ApplicationUpgradeProgress>> GetApplicationUpgradeProgressAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(name, "name");

            ApplicationUpgradeProgress queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ApplicationUpgradeProgress> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, name);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricApplicationUpgradeCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ApplicationUpgradeProgress;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationUpgradeProgress>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationUpgradeProgress>(queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<FabricUpgradeProgress>> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricUpgradeProgress queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<FabricUpgradeProgress> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricClusterUpgradeCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as FabricUpgradeProgress;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeProgress>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<FabricUpgradeProgress>(queryResult);
            }

            return operationResult;
        }

        public override async Task<OperationResult<Partition>> GetServicePartitionAsync(Uri serviceName, Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(serviceName, "serviceName");

            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<Partition> operationResult = null;
            var queryResults = new List<Partition>();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceNameParameter, serviceName);
                    parameters.Add(PowerShellConstants.PartitionIdParameter, partitionId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionCommand, parameters);
                    PowerShellFabricClient.SelectValidResultTypes<Partition>(queryResults, powershellResult);
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<Partition>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<Partition>(queryResults.First());
            }

            return operationResult;
        }

        public override async Task<OperationResult> UpdateServiceGroupAsync(ServiceKind serviceKind, Uri serviceGroupName, int targetReplicaSetSize, TimeSpan replicaRestartWaitDuration, TimeSpan quorumLossWaitDuration, int instanceCount, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(serviceGroupName, "serviceGroupName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceGroupNameParameter, serviceGroupName);

                    if (serviceKind == ServiceKind.Stateful)
                    {
                        parameters.Add(PowerShellConstants.Stateful);
                        parameters.Add(PowerShellConstants.TargetReplicaSetSize, targetReplicaSetSize);
                        parameters.Add(PowerShellConstants.ReplicaRestartWaitDurationParameter, replicaRestartWaitDuration);
                        parameters.Add(PowerShellConstants.QuorumLossWaitDurationParameter, quorumLossWaitDuration);
                    }
                    else if (serviceKind == ServiceKind.Stateless)
                    {
                        parameters.Add(PowerShellConstants.Stateful);
                        parameters.Add(PowerShellConstants.InstanceCount, instanceCount);
                    }
                    else
                    {
                        throw new InvalidProgramException("WinFabricServiceKind parameter is invalid");
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.ForceParameter);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.UpdateServiceFabricServiceGroupCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<string>> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(applicationTypeVersion, "applicationTypeVersion");
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");

            string applicationManifest = string.Empty;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<string> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationTypeName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, applicationTypeVersion);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricApplicationManifestCommand, parameters);
                    applicationManifest = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<string>().FirstOrDefault();
                });

            if (string.IsNullOrEmpty(string.Format(CultureInfo.InvariantCulture, applicationManifest)))
            {
                throw new InvalidOperationException("Empty application manifest was returned");
            }

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<string>(applicationManifest);
            }

            return operationResult;
        }

        public override async Task<OperationResult<ImageStoreContentResult>> ListImageStoreContentAsync(string remoteLocation, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PSDataCollection<PSObject> powershellResult = null;
            ImageStoreContentResult result = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.RemoteRelativePathParameter, remoteLocation);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    parameters.Add(PowerShellConstants.UsePagingParameter, true);
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetImageStoreContentCommand, parameters);
                    result = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ImageStoreContentResult>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<ImageStoreContentResult>(errorCode, result);
        }

        public override async Task<OperationResult<bool>> TestApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");

            bool success = false;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<bool> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationPackagePathParameter, applicationPackagePath);
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.TestServiceFabricApplicationPackageCommand, parameters);
                    foreach (PSObject item in powershellResult)
                    {
                        success = (bool)item.BaseObject;
                    }
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<bool>(success);
            }

            return operationResult;
        }

        public override async Task<OperationResult> RemoveApplicationPackageAsync(string applicationPackagePathInImageStore, string imageStoreConnectionString, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePathInImageStore, "applicationPackagePathInImageStore");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationPackagePathInImageStoreParameter, applicationPackagePathInImageStore);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.RemoveApplicationPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyApplicationPackageAsync(string applicationPackagePath, string imageStoreConnectionString, string applicationPackagePathInImageStore, bool compress, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");
  
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationPackagePathInImageStoreParameter, applicationPackagePathInImageStore);
                    parameters.Add(PowerShellConstants.ApplicationPackagePathParameter, applicationPackagePath);
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    if (compress)
                    {
                        parameters.Add(PowerShellConstants.CompressPackageParameter);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.CopyServiceFabricApplicationPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    AddCodeAndConfigPathParameters(parameters, codePackagePath, clusterManifestPath);
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    AddCodeAndConfigPathInImageStoreParameters(parameters, codePackagePathInImageStore, clusterManifestPathInImageStore);
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.CopyServiceFabricClusterPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }


        public override async Task<OperationResult> CopyClusterPackageAsync(string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore,
            TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    AddCodeAndConfigPathParameters(parameters, codePackagePath, clusterManifestPath);
                    if (!string.IsNullOrEmpty(imageStoreConnectionString))
                    {
                        parameters.Add(PowerShellConstants.ImageStoreConnectionStringParameter, imageStoreConnectionString);
                    }

                    AddCodeAndConfigPathInImageStoreParameters(parameters, codePackagePathInImageStore, clusterManifestPathInImageStore);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.CopyServiceFabricClusterPackageCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }
        public override async Task<OperationResult> CopyWindowsFabricServicePackageToNodeAsync(string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.NullOrEmpty(serviceManifestName, "serviceManifestName");
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");
            ThrowIf.NullOrEmpty(applicationTypeVersion, "applicationTypeVersion");
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, serviceManifestName);
                    parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, applicationTypeName);
                    parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, applicationTypeVersion);
                    parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);
                    parameters.Add(PowerShellConstants.PackageSharingPoliciesParameter, GetServicePackageSharingPolicy(policies));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.CopyServiceFabricServicePackageToNodeCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ClusterHealth>> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ClusterHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ClusterHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.MaxPercentUnhealthyApplications > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyApplicationsParameter, healthPolicy.MaxPercentUnhealthyApplications);
                        }

                        if (healthPolicy.MaxPercentUnhealthyNodes > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyNodesParameter, healthPolicy.MaxPercentUnhealthyNodes);
                        }
                    }

                    if (queryDescription.ApplicationHealthPolicyMap.Count > 0)
                    {
                        parameters.Add(PowerShellConstants.ApplicationHealthPolicyMapParameter, queryDescription.ApplicationHealthPolicyMap);
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.NodesFilter != null)
                    {
                        parameters.Add(PowerShellConstants.NodesHealthStateFilter, queryDescription.NodesFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.ApplicationsFilter != null)
                    {
                        if (queryDescription.ApplicationsFilter.HealthStateFilterValue != HealthStateFilter.Default)
                        {
                            parameters.Add(PowerShellConstants.ApplicationsHealthStateFilter, queryDescription.ApplicationsFilter.HealthStateFilterValue);
                        }
                    }

                    if (queryDescription.HealthStatisticsFilter != null)
                    {
                        if (queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics)
                        {
                            parameters.Add(PowerShellConstants.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics);
                        }
                        else
                        {
                            parameters.Add(PowerShellConstants.IncludeSystemApplicationHealthStatistics, queryDescription.HealthStatisticsFilter.IncludeSystemApplicationHealthStatistics);
                        }
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricClusterHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ClusterHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealth>(errorCode, queryResult);
            }

            return operationResult;
        }

        public override async Task<OperationResult<ClusterHealthChunk>> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ClusterHealthChunk queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ClusterHealthChunk> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    var healthPolicy = queryDescription.ClusterHealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.MaxPercentUnhealthyApplications > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyApplicationsParameter, healthPolicy.MaxPercentUnhealthyApplications);
                        }

                        if (healthPolicy.MaxPercentUnhealthyNodes > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyNodesParameter, healthPolicy.MaxPercentUnhealthyNodes);
                        }

                        if (healthPolicy.ApplicationTypeHealthPolicyMap.Count > 0)
                        {
                            parameters.Add(PowerShellConstants.ApplicationTypeHealthPolicyMapParameter, healthPolicy.ApplicationTypeHealthPolicyMap);
                        }
                    }

                    if (queryDescription.ApplicationHealthPolicies.Count > 0)
                    {
                        parameters.Add(PowerShellConstants.ApplicationHealthPoliciesParameter, queryDescription.ApplicationHealthPolicies);
                    }

                    if (queryDescription.NodeFilters.Count > 0)
                    {
                        parameters.Add(PowerShellConstants.NodeFilters, queryDescription.NodeFilters);
                    }

                    if (queryDescription.ApplicationFilters.Count > 0)
                    {
                        parameters.Add(PowerShellConstants.ApplicationFilters, queryDescription.ApplicationFilters);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricClusterHealthChunkCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ClusterHealthChunk;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealthChunk>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ClusterHealthChunk>(errorCode, queryResult);
            }

            return operationResult;
        }

        public override async Task<OperationResult<ApplicationHealth>> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ApplicationHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ApplicationHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, queryDescription.ApplicationName);

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.MaxPercentUnhealthyDeployedApplications > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyDeployedApplicationsParameter, healthPolicy.MaxPercentUnhealthyDeployedApplications);
                        }

                        if (healthPolicy.DefaultServiceTypeHealthPolicy != null && healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyServicesParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices);
                        }

                        if (healthPolicy.DefaultServiceTypeHealthPolicy != null && healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyPartitionsPerServiceParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService);
                        }

                        if (healthPolicy.DefaultServiceTypeHealthPolicy != null && healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyServicesParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition);
                        }

                        if (queryDescription.HealthStatisticsFilter != null)
                        {
                            parameters.Add(PowerShellConstants.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics);
                        }
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.ServicesFilter != null)
                    {
                        if (queryDescription.ServicesFilter.HealthStateFilterValue != HealthStateFilter.Default)
                        {
                            parameters.Add(PowerShellConstants.ServicesHealthStateFilter, queryDescription.ServicesFilter.HealthStateFilterValue);
                        }
                    }

                    if (queryDescription.DeployedApplicationsFilter != null)
                    {
                        if (queryDescription.DeployedApplicationsFilter.HealthStateFilterValue != HealthStateFilter.Default)
                        {
                            parameters.Add(PowerShellConstants.DeployedApplicationsHealthStateFilter, queryDescription.DeployedApplicationsFilter.HealthStateFilterValue);
                        }
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricApplicationHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ApplicationHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationHealth>(errorCode, queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<ServiceHealth>> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ServiceHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ServiceHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.ServiceNameParameter, queryDescription.ServiceName);

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.DefaultServiceTypeHealthPolicy != null)
                        {
                            // TODO: this is not correct; if the application contains map with specific items for this service type, that policy should be used.
                            // For that, we need to know the service type name.
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyPartitionsPerServiceParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService);
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyReplicasPerPartitionParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition);
                        }
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.PartitionsFilter != null)
                    {
                        if (queryDescription.PartitionsFilter.HealthStateFilterValue != HealthStateFilter.Default)
                        {
                            parameters.Add(PowerShellConstants.PartitionsHealthStateFilter, queryDescription.PartitionsFilter.HealthStateFilterValue);
                        }
                    }

                    if (queryDescription.HealthStatisticsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricServiceHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ServiceHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ServiceHealth>(errorCode, queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<PartitionHealth>> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            PartitionHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<PartitionHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionId, queryDescription.PartitionId);

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.DefaultServiceTypeHealthPolicy != null)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyReplicasPerPartitionParameter, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition);
                        }
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.ReplicasFilter != null)
                    {
                        if (queryDescription.ReplicasFilter.HealthStateFilterValue != HealthStateFilter.Default)
                        {
                            parameters.Add(PowerShellConstants.ReplicasHealthStateFilter, queryDescription.ReplicasFilter.HealthStateFilterValue);
                        }
                    }

                    if (queryDescription.HealthStatisticsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricPartitionHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as PartitionHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionHealth>(errorCode, queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<ReplicaHealth>> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            ReplicaHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<ReplicaHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.PartitionId, queryDescription.PartitionId);
                    parameters.Add(PowerShellConstants.ReplicaOrInstanceIdParameter, queryDescription.ReplicaOrInstanceId);

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricReplicaHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as ReplicaHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<ReplicaHealth>(errorCode, queryResult);
            }

            return operationResult;
        }

        public override OperationResult ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            ThrowIf.Null(healthReport, "healthReport");

            CommandParameterCollection parameters = new CommandParameterCollection();
            parameters.Add(PowerShellConstants.SourceIdParameter, healthReport.HealthInformation.SourceId);
            parameters.Add(PowerShellConstants.HealthPropertyParameter, healthReport.HealthInformation.Property);
            if (!string.IsNullOrEmpty(healthReport.HealthInformation.Description))
            {
                parameters.Add(PowerShellConstants.DescriptionParameter, healthReport.HealthInformation.Description);
            }

            parameters.Add(PowerShellConstants.HealthStateParameter, healthReport.HealthInformation.HealthState);
            parameters.Add(PowerShellConstants.SequenceNumberParameter, healthReport.HealthInformation.SequenceNumber);
            AddTimeParameter(parameters, PowerShellConstants.TimeToLiveSecParameter, healthReport.HealthInformation.TimeToLive);

            if (healthReport.HealthInformation.RemoveWhenExpired)
            {
                parameters.Add(PowerShellConstants.RemoveWhenExpiredParameter);
            }

            if (sendOptions != null && sendOptions.Immediate)
            {
                parameters.Add(PowerShellConstants.ImmediateParameter);
            }

            switch (healthReport.Kind)
            {
                case HealthReportKind.Application:
                    {
                        var report = healthReport as ApplicationHealthReport;
                        parameters.Add(PowerShellConstants.ApplicationNameParameter, report.ApplicationName);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendApplicationHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.Node:
                    {
                        var report = healthReport as NodeHealthReport;
                        parameters.Add(PowerShellConstants.NodeNameParameter, report.NodeName);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendNodeHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.Cluster:
                    {
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendClusterHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.Service:
                    {
                        var report = healthReport as ServiceHealthReport;
                        parameters.Add(PowerShellConstants.ServiceNameParameter, report.ServiceName);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendServiceHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.Partition:
                    {
                        var report = healthReport as PartitionHealthReport;
                        parameters.Add(PowerShellConstants.PartitionIdParameter, report.PartitionId);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendPartitionHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.StatefulServiceReplica:
                    {
                        var report = healthReport as StatefulServiceReplicaHealthReport;
                        parameters.Add(PowerShellConstants.PartitionIdParameter, report.PartitionId);
                        parameters.Add(PowerShellConstants.ReplicaIdParameter, report.ReplicaId);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendReplicaHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.StatelessServiceInstance:
                    {
                        var report = healthReport as StatelessServiceInstanceHealthReport;
                        parameters.Add(PowerShellConstants.PartitionIdParameter, report.PartitionId);
                        parameters.Add(PowerShellConstants.InstanceIdParameter, report.InstanceId);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendReplicaHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.DeployedApplication:
                    {
                        var report = healthReport as DeployedApplicationHealthReport;
                        parameters.Add(PowerShellConstants.ApplicationNameParameter, report.ApplicationName);
                        parameters.Add(PowerShellConstants.NodeNameParameter, report.NodeName);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendDeployedApplicationHealthReportCommand, parameters).Wait();
                        break;
                    }
                case HealthReportKind.DeployedServicePackage:
                    {
                        var report = healthReport as DeployedServicePackageHealthReport;
                        parameters.Add(PowerShellConstants.ApplicationNameParameter, report.ApplicationName);
                        parameters.Add(PowerShellConstants.NodeNameParameter, report.NodeName);
                        parameters.Add(PowerShellConstants.ServiceManifestNameParameter, report.ServiceManifestName);
                        this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.SendDeployedServicePackageHealthReportCommand, parameters).Wait();
                        break;
                    }
                default:
                    throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, "Unsupported kind {0}", healthReport.Kind));
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode(0);
        }

        public async override Task<OperationResult<NodeHealth>> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            NodeHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<NodeHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, queryDescription.NodeName);

                    var healthPolicy = queryDescription.HealthPolicy;
                    if (healthPolicy != null)
                    {
                        parameters.Add(PowerShellConstants.ConsiderWarningAsErrorParameter, healthPolicy.ConsiderWarningAsError);
                        if (healthPolicy.MaxPercentUnhealthyNodes > 0)
                        {
                            parameters.Add(PowerShellConstants.MaxPercentUnhealthyNodesParameter, healthPolicy.MaxPercentUnhealthyNodes);
                        }
                    }

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricNodeHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as NodeHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<NodeHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<NodeHealth>(queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<DeployedServicePackageHealth>> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            DeployedServicePackageHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<DeployedServicePackageHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, queryDescription.NodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, queryDescription.ApplicationName);
                    parameters.Add(PowerShellConstants.ServiceManifestNameParameter, queryDescription.ServiceManifestName);

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricDeployedServicePackageHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as DeployedServicePackageHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedServicePackageHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedServicePackageHealth>(queryResult);
            }

            return operationResult;
        }

        public async override Task<OperationResult<DeployedApplicationHealth>> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            ThrowIf.Null(queryDescription, "queryDescription");

            DeployedApplicationHealth queryResult = null;
            PSDataCollection<PSObject> powershellResult = null;
            OperationResult<DeployedApplicationHealth> operationResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.NodeNameParameter, queryDescription.NodeName);
                    parameters.Add(PowerShellConstants.ApplicationNameParameter, queryDescription.ApplicationName);

                    if (queryDescription.EventsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.EventsHealthStateFilter, queryDescription.EventsFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.DeployedServicePackagesFilter != null)
                    {
                        parameters.Add(PowerShellConstants.DeployedServicePackagesHealthStateFilter, queryDescription.DeployedServicePackagesFilter.HealthStateFilterValue);
                    }

                    if (queryDescription.HealthStatisticsFilter != null)
                    {
                        parameters.Add(PowerShellConstants.ExcludeHealthStatistics, queryDescription.HealthStatisticsFilter.ExcludeHealthStatistics);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetServiceFabricDeployedApplicationHealthCommand, parameters);
                    queryResult = powershellResult.FirstOrDefault().ImmediateBaseObject as DeployedApplicationHealth;
                });

            if (errorCode != 0)
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplicationHealth>(errorCode);
            }
            else
            {
                operationResult = FabricClientState.CreateOperationResultFromNativeErrorCode<DeployedApplicationHealth>(queryResult);
            }

            return operationResult;
        }

        public override async Task<OperationResult> InvokeQuorumLossAsync(
            PartitionSelector partitionSelector,
            QuorumLossMode quorumlossMode,
            TimeSpan quorumlossDuration,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                                                          delegate
                                                              {
                                                                  CommandParameterCollection parameters = new CommandParameterCollection
                                                                                                              {
                                                                                                                      {
                                                                                                                          PowerShellConstants.TimeoutSec,
                                                                                                                          ConvertToSecondsClamped(operationTimeout)
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants.InvokeQuorumLossMode,
                                                                                                                          quorumlossMode
                                                                                                                      },
                                                                                                                      {
                                                                                                                          PowerShellConstants.InvokeQuorumLossDuration,
                                                                                                                          ConvertToSecondsClamped(quorumlossDuration)
                                                                                                                      }
                                                                                                              };
                                                                  this.AddPartitionSelectorParameters(partitionSelector, parameters);

                                                                  return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.InvokeQuorumLossCommand, parameters);
                                                              }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartPartitionDataLossAsync(Guid operationId, PartitionSelector partitionSelector, DataLossMode dataLossMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    AddPartitionSelectorParameters(partitionSelector, parameters);
                    parameters.Add(PowerShellConstants.InvokeDataLossMode, dataLossMode);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartPartitionDataLossCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionDataLossProgress>> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionDataLossProgress progress = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionDataLossProgressCommand, parameters);
                    progress = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<PartitionDataLossProgress>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionDataLossProgress>(errorCode, progress);
        }

        public override async Task<OperationResult> StartPartitionQuorumLossAsync(Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumlossMode, TimeSpan quorumlossDuration, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    AddPartitionSelectorParameters(partitionSelector, parameters);
                    parameters.Add(PowerShellConstants.InvokeQuorumLossMode, quorumlossMode);
                    parameters.Add(PowerShellConstants.QuorumLossDurationInSeconds, quorumlossDuration.TotalSeconds);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartPartitionQuorumLossCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionQuorumLossProgress>> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionQuorumLossProgress progress = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionQuorumLossProgressCommand, parameters);
                    progress = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<PartitionQuorumLossProgress>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionQuorumLossProgress>(errorCode, progress);
        }

        public override async Task<OperationResult> StartPartitionRestartAsync(Guid operationId, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan operationTimeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    AddPartitionSelectorParameters(partitionSelector, parameters);
                    parameters.Add(PowerShellConstants.RestartPartitionMode, restartPartitionMode);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(operationTimeout));

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartPartitionRestartCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<PartitionRestartProgress>> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PartitionRestartProgress progress = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetPartitionRestartProgressCommand, parameters);
                    progress = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<PartitionRestartProgress>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<PartitionRestartProgress>(errorCode, progress);
        }

        public override async Task<OperationResult> CancelTestCommandAsync(Guid operationId, bool force, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    if (force)
                    {
                        parameters.Add(PowerShellConstants.ForceCancel, force);
                    }

                    // always specify -Force so there is no prompt.  Not to be confused with "-ForceCancel" which indicates whether or not to gracefully cancel the command
                    parameters.Add(PowerShellConstants.ForceParameter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StopTestCommandCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<TestCommandStatusList>> GetTestCommandStatusListAsync(TestCommandStateFilter stateFilter, TestCommandTypeFilter typeFilter, TimeSpan timeout, CancellationToken cancellationToken)
        {
            PSDataCollection<PSObject> powershellResult = null;
            List<TestCommandStatus> list = new List<TestCommandStatus>();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.TestCommandStateFilter, stateFilter);
                    parameters.Add(PowerShellConstants.TestCommandTypeFilter, typeFilter);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetTestCommandStatusListCommand, parameters);
                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<TestCommandStatus>();
                    foreach (TestCommandStatus item in psItems)
                    {
                        list.Add(item);
                    }
                });

            TestCommandStatusList testCommandStatusList = new TestCommandStatusList(list);
            return FabricClientState.CreateOperationResultFromNativeErrorCode<TestCommandStatusList>(errorCode, testCommandStatusList);
        }

        public override async Task<OperationResult> StartNodeTransitionAsync(
            NodeTransitionDescription description,
            TimeSpan timeout,
            CancellationToken token)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = NativeTypes.FABRIC_ERROR_CODE.E_UNEXPECTED;

            if (description.NodeTransitionType == NodeTransitionType.Start)
            {
                errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.StartSwitch);
                    parameters.Add(PowerShellConstants.OperationId, description.OperationId);
                    parameters.Add(PowerShellConstants.NodeNameParameter, description.NodeName);
                    parameters.Add(PowerShellConstants.NodeInstanceIdParameter, description.NodeInstanceId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartNodeTransitionCommand, parameters);
                });
            }
            else if (description.NodeTransitionType == NodeTransitionType.Stop)
            {
                NodeStopDescription nodeStopDescription = (NodeStopDescription)description;
                errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.StopSwitch);
                    parameters.Add(PowerShellConstants.OperationId, description.OperationId);
                    parameters.Add(PowerShellConstants.NodeNameParameter, description.NodeName);
                    parameters.Add(PowerShellConstants.NodeInstanceIdParameter, description.NodeInstanceId);
                    parameters.Add(PowerShellConstants.StopDurationInSecondsParameter, nodeStopDescription.StopDurationInSeconds);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));
                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartNodeTransitionCommand, parameters);
                });
            }
            else
            {
                throw new NotImplementedException();
            }

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<NodeTransitionProgress>> GetNodeTransitionProgressAsync(Guid operationId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NodeTransitionProgress progress = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();
                    parameters.Add(PowerShellConstants.OperationId, operationId);
                    parameters.Add(PowerShellConstants.TimeoutSec, ConvertToSecondsClamped(timeout));

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetNodeTransitionProgressCommand, parameters);
                    progress = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<NodeTransitionProgress>().FirstOrDefault();
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode<NodeTransitionProgress>(errorCode, progress);
        }

        public override async Task<OperationResult<ChaosDescription>> GetChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosDescription description = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async () => {
                    var parameters = new CommandParameterCollection();

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetChaosCommand, parameters);

                    description = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ChaosDescription>().FirstOrDefault();
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, description);
        }

        public override async Task<OperationResult<ChaosScheduleDescription>> GetChaosScheduleAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosScheduleDescription description = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async () => {
                    var parameters = new CommandParameterCollection();

                    powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetChaosScheduleCommand, parameters);

                    description = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ChaosScheduleDescription>().FirstOrDefault();
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, description);
        }

        public override async Task<OperationResult> SetChaosScheduleAsync(
            string scheduleJson,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var schedule = this.GetDeserializedObject<ChaosScheduleDescription>(scheduleJson);

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                () => {
                    var parameters = new CommandParameterCollection();
                    parameters.Add(
                        PowerShellConstants.ChaosScheduleDescriptionParameter,
                        schedule);

                    return this.powershellManager.ExecutePowerShellCommandAsync(
                        PowerShellConstants.SetChaosScheduleCommand,
                        parameters);
                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartChaosAsync(
            ChaosParameters chaosParameters,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                            () =>
                                {
                                    CommandParameterCollection parameters = new CommandParameterCollection();
                                    parameters.Add(
                                        PowerShellConstants.MaxClusterStabilizationTimeoutSecParameter,
                                        chaosParameters.MaxClusterStabilizationTimeout.TotalSeconds);
                                    parameters.Add(
                                        PowerShellConstants.MaxConcurrentFaultsParameter,
                                        chaosParameters.MaxConcurrentFaults);
                                    if (chaosParameters.EnableMoveReplicaFaults)
                                    {
                                        parameters.Add(
                                            PowerShellConstants.EnableMoveReplicaFaultsSwitchParameter);
                                    }

                                    parameters.Add(
                                        PowerShellConstants.TimeToRunMinuteParameter,
                                        chaosParameters.TimeToRun.TotalMinutes);

                                    Hashtable contextTable = null;
                                    if (chaosParameters.Context.Any())
                                    {
                                        contextTable = new Hashtable();
                                        foreach (var kvp in chaosParameters.Context)
                                        {
                                            contextTable.Add(kvp.Key, kvp.Value);
                                        }
                                    }

                                    parameters.Add(PowerShellConstants.ContextParameter, contextTable);

                                    parameters.Add(PowerShellConstants.ClusterHealthPolicyParameter, chaosParameters.ClusterHealthPolicy);

                                    return
                                        this.powershellManager.ExecutePowerShellCommandAsync(
                                            PowerShellConstants.StartChaosCommand,
                                            parameters);
                                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult<ChaosEventsSegment>> GetChaosEventsAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            long maxResults,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosEventsSegment events = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                            async () =>
                                    {
                                        CommandParameterCollection parameters = new CommandParameterCollection();
                                        parameters.Add(
                                            PowerShellConstants.StartTimeUtcParameter,
                                            startTime);
                                        parameters.Add(
                                            PowerShellConstants.EndTimeUtcParameter,
                                            endTime);
                                        parameters.Add(
                                            PowerShellConstants.ContinuationTokenParameter,
                                            continuationToken);
                                        parameters.Add(
                                            PowerShellConstants.MaxResultsParameter,
                                            maxResults);

                                        powershellResult =
                                            await this.powershellManager.ExecutePowerShellCommandAsync(
                                                PowerShellConstants.GetChaosEventsCommand,
                                                parameters);

                                        events = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ChaosEventsSegment>().FirstOrDefault();
                                    }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, events);
        }

        public override async Task<OperationResult<ChaosReport>> GetChaosReportAsync(
            DateTime startTime,
            DateTime endTime,
            string continuationToken,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            ChaosReport report = null;
            PSDataCollection<PSObject> powershellResult = null;

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                            async () =>
                                    {
                                        CommandParameterCollection parameters = new CommandParameterCollection();
                                        parameters.Add(
                                            PowerShellConstants.StartTimeUtcParameter,
                                            startTime);
                                        parameters.Add(
                                            PowerShellConstants.EndTimeUtcParameter,
                                            endTime);
                                        parameters.Add(
                                            PowerShellConstants.ContinuationTokenParameter,
                                            continuationToken);

                                        powershellResult =
                                            await this.powershellManager.ExecutePowerShellCommandAsync(
                                                PowerShellConstants.GetChaosReportCommand,
                                                parameters);

                                        report = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ChaosReport>().FirstOrDefault();
                                    }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode, report);
        }

        public override async Task<OperationResult> StopChaosAsync(
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                            () =>
                                {
                                    CommandParameterCollection parameters = new CommandParameterCollection();

                                    return
                                        this.powershellManager.ExecutePowerShellCommandAsync(
                                            PowerShellConstants.StopChaosCommand,
                                            parameters);
                                }).ConfigureAwait(false);

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        public override async Task<OperationResult> StartRepairTaskAsync(string nodeName, SystemNodeRepairAction nodeAction, string[] nodeNames, string customAction, NodeImpactLevel nodeImpact, string taskId, string description, TimeSpan timeout, CancellationToken cancellationToken)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                delegate
                {
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    if (!string.IsNullOrEmpty(nodeName))
                    {
                        parameters.Add(PowerShellConstants.NodeNameParameter, nodeName);

                        //Node Action
                        if (nodeAction == SystemNodeRepairAction.FullReimage)
                        {
                            parameters.Add(PowerShellConstants.NodeActionParameter, SystemNodeRepairAction.FullReimage);
                        }
                        else if (nodeAction == SystemNodeRepairAction.Reboot)
                        {
                            parameters.Add(PowerShellConstants.NodeActionParameter, SystemNodeRepairAction.Reboot);
                        }
                        else if (nodeAction == SystemNodeRepairAction.ReimageOS)
                        {
                            parameters.Add(PowerShellConstants.NodeActionParameter, SystemNodeRepairAction.ReimageOS);
                        }
                        else
                        {
                            throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Invalid Node Action: {0}", nodeAction));
                        }
                    }

                    else if (nodeNames.Length > 0)
                    {
                        parameters.Add(PowerShellConstants.NodeNamesParameter, nodeNames);

                        if(!string.IsNullOrEmpty(customAction))
                        {
                            parameters.Add(PowerShellConstants.CustomActionParameter, customAction);
                        }
                        else
                        {
                            //Node Impact
                            if (nodeImpact == NodeImpactLevel.Invalid)
                            {
                                parameters.Add(PowerShellConstants.NodeImpactParameter, NodeImpactLevel.Invalid);
                            }
                            else if (nodeImpact == NodeImpactLevel.None)
                            {
                                parameters.Add(PowerShellConstants.NodeImpactParameter, NodeImpactLevel.None);
                            }
                            else if (nodeImpact == NodeImpactLevel.RemoveData)
                            {
                                parameters.Add(PowerShellConstants.NodeImpactParameter, NodeImpactLevel.RemoveData);
                            }
                            else if (nodeImpact == NodeImpactLevel.RemoveNode)
                            {
                                parameters.Add(PowerShellConstants.NodeImpactParameter, NodeImpactLevel.RemoveNode);
                            }
                            else if (nodeImpact == NodeImpactLevel.Restart)
                            {
                                parameters.Add(PowerShellConstants.NodeImpactParameter, NodeImpactLevel.Restart);
                            }
                            else
                            {
                                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Invalid Node Impact: {0}", nodeImpact));
                            }
                        }
                    }

                    if (!string.IsNullOrEmpty(taskId))
                    {
                        parameters.Add(PowerShellConstants.TaskIdParameter, taskId);
                    }

                    if (!string.IsNullOrEmpty(description))
                    {
                        parameters.Add(PowerShellConstants.DescriptionParameter, description);
                    }

                    return this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.StartServiceFabricRepairTaskCommand, parameters);
                });

            return FabricClientState.CreateOperationResultFromNativeErrorCode(errorCode);
        }

        ////==========================================================
        private static void SelectValidResultTypes<T>(List<T> queryResults, PSDataCollection<PSObject> powershellResult)
        {
            queryResults.AddRange(powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<T>());
        }

        private static Hashtable BuildParameterArugument<T>(System.Collections.Specialized.NameValueCollection descriptionParameters)
        {
            Hashtable parameterTable = new Hashtable();
            foreach (string name in descriptionParameters.Keys)
            {
                parameterTable.Add(name, descriptionParameters[name]);
            }

            return parameterTable;
        }

        private static void AddCodeAndConfigVersionParameters(CommandParameterCollection parameters, string codeVersion, string configVersion)
        {
            if (string.IsNullOrEmpty(codeVersion) && string.IsNullOrEmpty(configVersion))
            {
                throw new InvalidOperationException("CodePackageVersionParameter and ClusterManifestVersionParameter both cannot be empty at the same time");
            }

            if (!string.IsNullOrEmpty(codeVersion) && !string.IsNullOrEmpty(configVersion))
            {
                parameters.Add(PowerShellConstants.CodePackageVersionParameter, codeVersion);
                parameters.Add(PowerShellConstants.ClusterManifestVersionParameter, configVersion);
            }
            else if (string.IsNullOrEmpty(configVersion) && !string.IsNullOrEmpty(codeVersion))
            {
                parameters.Add(PowerShellConstants.CodeParameter);
                parameters.Add(PowerShellConstants.CodePackageVersionParameter, codeVersion);
            }
            else
            {
                parameters.Add(PowerShellConstants.ConfigParameter);
                parameters.Add(PowerShellConstants.ClusterManifestVersionParameter, configVersion);
            }
        }

        private static void AddCodeAndConfigPathParameters(CommandParameterCollection parameters, string codePackagePath, string clusterManifestPath)
        {
            if (string.IsNullOrEmpty(codePackagePath) && string.IsNullOrEmpty(clusterManifestPath))
            {
                throw new InvalidOperationException("CodePackagePathParameter and ClusterManifestPathParameter both cannot be empty at the same time");
            }

            if (!string.IsNullOrEmpty(codePackagePath) && !string.IsNullOrEmpty(clusterManifestPath))
            {
                parameters.Add(PowerShellConstants.CodePackagePathParameter, codePackagePath);
                parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
            }
            else if (!string.IsNullOrEmpty(codePackagePath))
            {
                parameters.Add(PowerShellConstants.CodeParameter);
                parameters.Add(PowerShellConstants.CodePackagePathParameter, codePackagePath);
            }
            else
            {
                parameters.Add(PowerShellConstants.ConfigParameter);
                parameters.Add(PowerShellConstants.ClusterManifestPathParameter, clusterManifestPath);
            }
        }

        private static void AddCodeAndConfigPathInImageStoreParameters(CommandParameterCollection parameters, string codePackagePathInImageStore, string clusterManifestPathInImageStore)
        {
            if (!string.IsNullOrEmpty(codePackagePathInImageStore))
            {
                parameters.Add(PowerShellConstants.CodePackagePathInImageStoreParameter, codePackagePathInImageStore);
            }

            if (!string.IsNullOrEmpty(clusterManifestPathInImageStore))
            {
                parameters.Add(PowerShellConstants.ClusterManifestPathInImageStoreParameter, clusterManifestPathInImageStore);
            }
        }

        private static void AddTimeParameter(CommandParameterCollection parameters, string parameterName, TimeSpan? duration)
        {
            if (duration.HasValue)
            {
                AddTimeParameter(parameters, parameterName, duration.Value);
            }
        }

        private static void AddTimeParameter(CommandParameterCollection parameters, string parameterName, TimeSpan duration)
        {
            if (duration.TotalSeconds >= 0)
            {
                parameters.Add(parameterName, ConvertToSecondsClamped(duration));
            }
        }

        private static string MakeServiceTypeHealthPolicyString(ServiceTypeHealthPolicy policy)
        {
            return string.Join(",", policy.MaxPercentUnhealthyPartitionsPerService, policy.MaxPercentUnhealthyReplicasPerPartition, policy.MaxPercentUnhealthyServices);
        }

        private void ConnectToCluster(Runspace runspace)
        {
            Command connectCommand = new Command(PowerShellConstants.ConnectClusterCommand);
            if (this.gatewayAddresses.Length > 0)
            {
                connectCommand.Parameters.Add(PowerShellConstants.ConnectionEndpointParameter, this.gatewayAddresses);
                if (this.securityCredentials != null)
                {
                    if (securityCredentials.GetType() == typeof(X509Credentials))
                    {
                        X509Credentials certificateCredentials = (X509Credentials)securityCredentials;
                        connectCommand.Parameters.Add(PowerShellConstants.X509CredentialParameter);
                        if (certificateCredentials.RemoteCommonNames != null && certificateCredentials.RemoteCommonNames.Any())
                        {
                            connectCommand.Parameters.Add(PowerShellConstants.ServerCommonNameParameter, certificateCredentials.RemoteCommonNames);
                        }

                        if (certificateCredentials.RemoteCertThumbprints != null && certificateCredentials.RemoteCertThumbprints.Any())
                        {
                            connectCommand.Parameters.Add(PowerShellConstants.ServerCertThumbprintParameter, certificateCredentials.RemoteCertThumbprints.First());
                        }

                        connectCommand.Parameters.Add(PowerShellConstants.FindTypeParameter, certificateCredentials.FindType);
                        connectCommand.Parameters.Add(PowerShellConstants.FindValueParameter, certificateCredentials.FindValue);
                        connectCommand.Parameters.Add(PowerShellConstants.StoreLocationParameter, certificateCredentials.StoreLocation);
                        connectCommand.Parameters.Add(PowerShellConstants.StoreNameParameter, certificateCredentials.StoreName);
                    }
                    else if (securityCredentials.GetType() == typeof(ClaimsCredentials))
                    {
                        //ClaimsCredentials can technically be used for both DSTS and AAD, but ScriptTest is only supporting AAD at this point
                        ClaimsCredentials AADCredentials = (ClaimsCredentials)securityCredentials;
                        connectCommand.Parameters.Add(PowerShellConstants.AADCredentialParameter);
                        connectCommand.Parameters.Add(PowerShellConstants.SecurityToken, AADCredentials.LocalClaims);
                        connectCommand.Parameters.Add(PowerShellConstants.ServerCertThumbprintParameter, AADCredentials.ServerThumbprints);
                    }
                    else
                    {
                        WindowsCredentials kerbSettings = this.securityCredentials as WindowsCredentials;
                        if (kerbSettings != null)
                        {
                            connectCommand.Parameters.Add(PowerShellConstants.WindowsCredentialParameter);
                        }
                    }
                }
            }

            using (PowerShell shell = PowerShell.Create())
            {
                shell.Runspace = runspace;
                shell.Commands.AddCommand(connectCommand);
                shell.Invoke();
            }
        }

        private PackageSharingPolicy[] GetServicePackageSharingPolicy(PackageSharingPolicyDescription[] policies)
        {
            Dictionary<WinFabricServicePackageSharingScope, PackageSharingPolicyScope> mapper = new Dictionary<WinFabricServicePackageSharingScope, PackageSharingPolicyScope>()
            {
                { WinFabricServicePackageSharingScope.None, PackageSharingPolicyScope.None },
                { WinFabricServicePackageSharingScope.All, PackageSharingPolicyScope.All },
                { WinFabricServicePackageSharingScope.Code, PackageSharingPolicyScope.Code },
                { WinFabricServicePackageSharingScope.Config, PackageSharingPolicyScope.Config },
                { WinFabricServicePackageSharingScope.Data, PackageSharingPolicyScope.Data }
            };

            PackageSharingPolicy[] targetPolicy = new PackageSharingPolicy[policies.Length];
            for (int index = 0; index < policies.Length; index++)
            {
                targetPolicy[index] = new PackageSharingPolicy(policies[index].PackageName, mapper[policies[index].Scope]);
            }

            return targetPolicy;
        }

        private static int ConvertToSecondsClamped(TimeSpan timeSpan)
        {
            return (int)Math.Min(timeSpan.TotalSeconds, int.MaxValue);
        }

        // This is used by QueryCommandsDispatcher's associated test. QueryCommandsDispatcher does not call this directly, but rather,
        // this is one of 3 options it chooses when performing a test. The other options are the REST and managed pathways.
        private async Task<OperationResult<ApplicationTypePagedList>> GetApplicationTypePagedListAsync(
            PagedApplicationTypeQueryDescription queryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            bool isForPowerShell,
            bool getPagedResults = false)
        {
            // Is derived from type PagedList, which includes the list of ApplicationType and a continuation token
            var queryResults = new ApplicationTypePagedList();

            NativeTypes.FABRIC_ERROR_CODE errorCode = await this.powershellManager.HandleExceptionAsync(
                async delegate
                {
                    // Pass only the parameters that the test (user) has passed in.
                    // PowerShell does not take continuationToken
                    CommandParameterCollection parameters = new CommandParameterCollection();

                    if (!string.IsNullOrEmpty(queryDescription.ApplicationTypeNameFilter))
                    {
                        parameters.Add(PowerShellConstants.ApplicationTypeNameParameter, queryDescription.ApplicationTypeNameFilter);
                    }

                    if (!string.IsNullOrEmpty(queryDescription.ApplicationTypeVersionFilter))
                    {
                        parameters.Add(PowerShellConstants.ApplicationTypeVersionParameter, queryDescription.ApplicationTypeVersionFilter);
                    }

                    parameters.Add(PowerShellConstants.ApplicationTypeDefinitionKindFilterParameter, queryDescription.ApplicationTypeDefinitionKindFilter);

                    // This is a switch parameter, so it can only be passed in if it's true.
                    if (queryDescription.ExcludeApplicationParameters)
                    {
                        parameters.Add(PowerShellConstants.ExcludeApplicationParametersParameter, queryDescription.ExcludeApplicationParameters);
                    }

                    parameters.Add(PowerShellConstants.TimeoutSec, (int)timeout.TotalSeconds);
                    parameters.Add(PowerShellConstants.UsePagingParameter, true);

                    // This command returns all the PowerShell objects printed to the output of PowerShell
                    // Since the output to PowerShell is each ApplicationType without the continuation token, that is what we'll get here
                    // That is, the first some objects will be of type ApplicationType, while the last object will be a string (the continuationToken)
                    // IF a continuation token is chosen to be returned.
                    var powershellResult = await this.powershellManager.ExecutePowerShellCommandAsync(PowerShellConstants.GetApplicationTypeCommand, parameters);

                    var psItems = powershellResult.Select<PSObject, object>(i => i.BaseObject).OfType<ApplicationType>();
                    if (psItems != null)
                    {
                        foreach (ApplicationType item in psItems)
                        {
                            queryResults.Add(item);
                        }
                    }

                    // Add this there to let the test verifier know what kind of results to expect.
                    queryResults.ContinuationToken = "IsPowerShellTest";
                });
            return FabricClientState.CreateOperationResultFromNativeErrorCode<ApplicationTypePagedList>(errorCode, queryResults);
        }

        private T GetDeserializedObject<T>(string json)
        {
            this.jsonSerializer = JsonSerializerImplDllLoader.TryGetJsonSerializerImpl();

            if (this.jsonSerializer == null)
            {
                // Fail if serializer was not loaded successfully.
                throw new DllNotFoundException("SystemFabricClient..ctor(): Call to JsonSerializerImplDllLoader.TryGetJsonSerializerImpl() failed.");
            }

            var obj = this.jsonSerializer.Deserialize(json, typeof(T));
            return (T)obj;
        }
    }
}

#pragma warning restore 1591