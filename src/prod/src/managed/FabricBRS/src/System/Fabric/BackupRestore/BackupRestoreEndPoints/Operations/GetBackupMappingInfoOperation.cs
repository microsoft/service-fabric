// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using BackupMappingModel = System.Fabric.BackupRestore.Common.Model.BackupMapping;
    using System.Fabric.BackupRestore.Common;
    using System.Collections.Generic;
    using System.Threading;
    using System.Text;
    using System.Fabric.BackupRestore.Enums;
    using BRSContinuationToken = System.Fabric.BackupRestore.Common.BRSContinuationToken;
    using System.Linq;

    internal class GetBackupMappingOperation : BackupMappingOperation<BackupConfigurationInfo>
    {
        private readonly string fabricRequestHeader;

        //continuationToken = LastFabricURI + Separator + ListingSuspendedOrNot 
        private readonly string continuationToken;
        private readonly int maxResults;

        // "S" is used only when have completed the listing of override URIs and we have started listing of suspended URIs.
        private const string SuspendedContinuationToken = "S";

        // "O" is used only when we are listing the overridden URIs and we have not started listing of suspended ones.
        private const string OverrideContinuationToken = "O";
        private const char ContinuationTokenSeparatorChar = Constants.ContinuationTokenSeparatorChar;


        internal GetBackupMappingOperation(string fabricRequestHeader, string apiVersion, Microsoft.ServiceFabric.Services.Runtime.StatefulService statefulService, string continuationToken = null, int maxResults = 0) : base(apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
            this.continuationToken = continuationToken;
            this.maxResults = maxResults;
        }

        internal override async Task<BackupConfigurationInfo> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationNameUri;
            string serviceNameUri;
            string partitionId;

            List<BackupEntityConfigurationInfo> backupConfigurationInfos = new List<BackupEntityConfigurationInfo>();
            var suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(this.StatefulService);
            var enablementKey = await UtilityHelper.GetFabricUriFromRequstHeader(this.fabricRequestHeader, timeout, cancellationToken);
            BRSContinuationToken brsContinuationToken = new BRSContinuationToken();
            brsContinuationToken.IncomingContinuationToken = continuationToken;
            var fabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionUri(enablementKey, out applicationNameUri, out serviceNameUri, out partitionId);
            var backupMapping = await this.BackupMappingStore.GetValueAsync(enablementKey, timeout, cancellationToken);

            int maxResultsForOverriddenPolicies = 0;
            if (maxResults != 0)
            {
                maxResultsForOverriddenPolicies = maxResults - 1;
            }
            switch (fabricBackupResourceType)
            {
                case FabricBackupResourceType.PartitionUri:

                    PartitionBackupConfigurationInfo partitionBackupConfigurationInfo = null;
                    BackupSuspensionInfo backupSuspensionInfo = new BackupSuspensionInfo();
                    if (await suspendStore.IsFabrcUriSuspended(enablementKey, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Partition;
                    }
                    else if (await suspendStore.IsFabrcUriSuspended(serviceNameUri, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Service;
                    }
                    else if (await suspendStore.IsFabrcUriSuspended(applicationNameUri, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Application;
                    }
                    if (backupMapping != null)
                    {
                        partitionBackupConfigurationInfo = new PartitionBackupConfigurationInfo(serviceNameUri,
                            partitionId, backupMapping.BackupPolicyName, BackupEntityKind.Partition, backupSuspensionInfo
                            );
                    }
                    else
                    {
                        var backupMappingInherited = await this.BackupMappingStore.GetValueAsync(serviceNameUri, timeout, cancellationToken);
                        if (backupMappingInherited != null)
                        {
                            partitionBackupConfigurationInfo = new PartitionBackupConfigurationInfo(serviceNameUri,
                            partitionId, backupMappingInherited.BackupPolicyName, BackupEntityKind.Service, backupSuspensionInfo
                            );

                        }
                        else
                        {
                            backupMappingInherited = await this.BackupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken);
                            if (backupMappingInherited != null)
                            {
                                partitionBackupConfigurationInfo = new PartitionBackupConfigurationInfo(serviceNameUri,
                            partitionId, backupMappingInherited.BackupPolicyName, BackupEntityKind.Application, backupSuspensionInfo
                            );
                            }
                        }
                    }

                    return partitionBackupConfigurationInfo;

                case FabricBackupResourceType.ServiceUri:

                    ServiceBackupConfigurationInfo serviceBackupConfigurationInfo = null;
                    backupSuspensionInfo = new BackupSuspensionInfo();
                    if (await suspendStore.IsFabrcUriSuspended(serviceNameUri, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Service;
                    }
                    else if (await suspendStore.IsFabrcUriSuspended(applicationNameUri, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Application;
                    }
                    if (backupMapping != null)
                    {
                        serviceBackupConfigurationInfo = new ServiceBackupConfigurationInfo(serviceNameUri,
                            backupMapping.BackupPolicyName, BackupEntityKind.Service, backupSuspensionInfo);
                    }
                    else
                    {
                        backupMapping = await this.BackupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken);
                        if (backupMapping != null)
                        {
                            serviceBackupConfigurationInfo = new ServiceBackupConfigurationInfo(serviceNameUri,
                                    backupMapping.BackupPolicyName, BackupEntityKind.Application, backupSuspensionInfo);
                        }
                    }

                    if (serviceBackupConfigurationInfo != null && String.IsNullOrEmpty(brsContinuationToken.IncomingContinuationToken))
                    {
                        backupConfigurationInfos.Add(serviceBackupConfigurationInfo);
                    }


                    if (maxResults == 1 && String.IsNullOrEmpty(brsContinuationToken.IncomingContinuationToken) && serviceBackupConfigurationInfo != null)
                    {
                        // if maxResults is 1, then, we need to check if more elements are there or not.
                        List<string> suspendedPartitions = await
                            suspendStore.GetAllSuspensionWithFabricUri(serviceNameUri, timeout, cancellationToken, brsContinuationToken.IncomingContinuationToken);
                        var overridesBackupMappings = await this.BackupMappingStore.GetAllProtectionWithFabricUri(serviceNameUri,
                            timeout, cancellationToken, brsContinuationToken.IncomingContinuationToken);
                        int countToCompare = 0;
                        if (backupSuspensionInfo.IsSuspended)
                        {
                            countToCompare = 1;
                        }
                        if (overridesBackupMappings.Count > 1 || suspendedPartitions.Count > countToCompare)
                        {
                            // overridesBackupMappings should be greater than 1 as it contains this mapping also or suspendedPartitions should have its count greater than zero.
                            brsContinuationToken.OutgoingContinuationToken = serviceNameUri + ContinuationTokenSeparatorChar + OverrideContinuationToken;
                        }
                    }
                    else
                    {
                        backupConfigurationInfos.AddRange(
                           await this.GetOverriddenBackupConfigurationinfo(serviceNameUri, backupSuspensionInfo, timeout, cancellationToken, brsContinuationToken, maxResultsForOverriddenPolicies));
                    }
                    break;
                case FabricBackupResourceType.ApplicationUri:
                    ApplicationBackupConfigurationInfo applicationBackupConfigurationInfo = null;
                    backupSuspensionInfo = new BackupSuspensionInfo();

                    if (await suspendStore.IsFabrcUriSuspended(applicationNameUri, timeout, cancellationToken))
                    {
                        backupSuspensionInfo.IsSuspended = true;
                        backupSuspensionInfo.SuspensionInheritedFrom = BackupEntityKind.Application;
                    }
                    if (backupMapping != null)
                    {
                        applicationBackupConfigurationInfo = new ApplicationBackupConfigurationInfo(applicationNameUri,
                            backupMapping.BackupPolicyName, BackupEntityKind.Application, backupSuspensionInfo);
                    }

                    if (applicationBackupConfigurationInfo != null && String.IsNullOrEmpty(brsContinuationToken.IncomingContinuationToken))
                    {
                        backupConfigurationInfos.Add(applicationBackupConfigurationInfo);
                    }

                    if (maxResults == 1 && String.IsNullOrEmpty(brsContinuationToken.IncomingContinuationToken) && applicationBackupConfigurationInfo != null)
                    {
                        // if maxResults is 1, then, we need to check if more elements are there or not.
                        List<string> suspendedPartitions = await
                            suspendStore.GetAllSuspensionWithFabricUri(applicationNameUri, timeout, cancellationToken);
                        var overridesBackupMappings = await this.BackupMappingStore.GetAllProtectionWithFabricUri(applicationNameUri,
                            timeout, cancellationToken, null);
                        int countToCompare = 0;
                        if (backupSuspensionInfo.IsSuspended)
                        {
                            countToCompare = 1;
                        }
                        if (overridesBackupMappings.Count > 1 || suspendedPartitions.Count > countToCompare)
                        {
                            // overridesBackupMappings is compared to 1 as 1st element will be same as this one.
                            brsContinuationToken.OutgoingContinuationToken = applicationNameUri + ContinuationTokenSeparatorChar + OverrideContinuationToken;

                        }
                    }
                    else
                    {
                        backupConfigurationInfos.AddRange(
                           await this.GetOverriddenBackupConfigurationinfo(applicationNameUri, backupSuspensionInfo, timeout, cancellationToken, brsContinuationToken, maxResultsForOverriddenPolicies));
                    }
                    break;
            }

            if (backupConfigurationInfos.Count == 0)
            {
                throw new FabricException(StringResources.BackupMappingNotExisting, FabricErrorCode.BackupNotEnabled);
            }
            return new PagedBackupConfigurationInfo()
            {
                Items = backupConfigurationInfos,
                ContinuationToken = brsContinuationToken.OutgoingContinuationToken
            };
        }

        private async Task<List<BackupEntityConfigurationInfo>> GetOverriddenBackupConfigurationinfo(string applicationServiceNameUri,
            BackupSuspensionInfo inheritedBackupSuspensionInfo, TimeSpan timeout, CancellationToken cancellationToken, BRSContinuationToken brsContinuationToken, int maxResultsLocal = 0)
        {
            List<BackupEntityConfigurationInfo> backupConfigurationInfos = new List<BackupEntityConfigurationInfo>();

            bool listSuspended = false;
            if (!String.IsNullOrEmpty(brsContinuationToken.IncomingContinuationToken))
            {
                string[] array = brsContinuationToken.IncomingContinuationToken.Split(ContinuationTokenSeparatorChar);
                brsContinuationToken.IncomingContinuationToken = array[0];
                if (array[1] == SuspendedContinuationToken)
                {
                    // We need to check if we were listing suspended partitions previously or override.
                    listSuspended = true;
                }
            }
            string continuationTokenForSuspendendPartitions = null;
            if (listSuspended)
            {
                // if we were listing overrides, then this continuationTokenForSuspendendPartitions should be null.
                continuationTokenForSuspendendPartitions = brsContinuationToken.IncomingContinuationToken;
            }

            var suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(this.StatefulService);
            List<string> suspendedEntities = await
                suspendStore.GetAllSuspensionWithFabricUri(applicationServiceNameUri, timeout, cancellationToken, continuationTokenForSuspendendPartitions);
            HashSet<string> suspendedEntitiesHashSet = new HashSet<string>(suspendedEntities);
            var overridesBackupMappings = await this.BackupMappingStore.GetAllProtectionWithFabricUri(applicationServiceNameUri,
                timeout, cancellationToken, brsContinuationToken.IncomingContinuationToken);

            string applicationNameUri = null;
            string serviceNameUri = null;
            string partitionId = null;
            int counter = 0;
            if (maxResultsLocal != 0)
            {
                // if maxResults is equal to zero, then we dont need to sort as we have already filtered the elements.
                overridesBackupMappings = overridesBackupMappings.OrderBy(backupMapping => backupMapping.ApplicationOrServiceUri).ToList();
            }
            foreach (var overridesBackupMapping in overridesBackupMappings)
            {
                if (!applicationServiceNameUri.Equals(overridesBackupMapping.ApplicationOrServiceUri))
                {
                    BackupEntityConfigurationInfo backupConfigurationInfo = null;
                    FabricBackupResourceType fabricBackupResourceType = UtilityHelper
                        .GetApplicationAndServicePartitionUri(
                            overridesBackupMapping.ApplicationOrServiceUri, out applicationNameUri, out serviceNameUri,
                            out partitionId);
                    BackupSuspensionInfo backupSuspensionInfo = inheritedBackupSuspensionInfo;
                    switch (fabricBackupResourceType)
                    {
                        case FabricBackupResourceType.PartitionUri:
                            if (suspendedEntitiesHashSet.Contains(overridesBackupMapping.ApplicationOrServiceUri))
                            {
                                backupSuspensionInfo = new BackupSuspensionInfo(BackupEntityKind.Partition, true);
                                suspendedEntitiesHashSet.Remove(overridesBackupMapping.ApplicationOrServiceUri);
                            }
                            if (listSuspended)
                            {
                                continue;
                            }
                            backupConfigurationInfo = new PartitionBackupConfigurationInfo(serviceNameUri,
                                    partitionId, overridesBackupMapping.BackupPolicyName, BackupEntityKind.Partition, backupSuspensionInfo);

                            break;

                        case FabricBackupResourceType.ServiceUri:
                            if (suspendedEntitiesHashSet.Contains(overridesBackupMapping.ApplicationOrServiceUri))
                            {
                                backupSuspensionInfo = new BackupSuspensionInfo(BackupEntityKind.Service, true);
                                suspendedEntitiesHashSet.Remove(overridesBackupMapping.ApplicationOrServiceUri);
                            }
                            if (listSuspended)
                            {
                                continue;
                            }
                            backupConfigurationInfo = new ServiceBackupConfigurationInfo(serviceNameUri,
                                overridesBackupMapping.BackupPolicyName, BackupEntityKind.Service, backupSuspensionInfo);
                            break;

                        case FabricBackupResourceType.ApplicationUri:
                            if (suspendedEntitiesHashSet.Contains(overridesBackupMapping.ApplicationOrServiceUri))
                            {
                                backupSuspensionInfo = new BackupSuspensionInfo(BackupEntityKind.Application, true);
                                suspendedEntitiesHashSet.Remove(overridesBackupMapping.ApplicationOrServiceUri);
                            }
                            if (listSuspended)
                            {
                                continue;
                            }
                            backupConfigurationInfo = new ApplicationBackupConfigurationInfo(applicationNameUri,
                                overridesBackupMapping.BackupPolicyName, BackupEntityKind.Application, backupSuspensionInfo);
                            break;
                    }
                    if (backupConfigurationInfo != null)
                    {
                        backupConfigurationInfos.Add(backupConfigurationInfo);
                        counter++;
                        if (maxResultsLocal != 0 && counter == maxResultsLocal)
                        {
                            if (counter < overridesBackupMappings.Count || suspendedEntitiesHashSet.Count > 0)
                            {
                                // we need to check if there are more elements to be listed or not.
                                brsContinuationToken.OutgoingContinuationToken = overridesBackupMapping.ApplicationOrServiceUri + ContinuationTokenSeparatorChar + OverrideContinuationToken;
                            }
                            return backupConfigurationInfos;
                        }
                    }
                }
            }

            suspendedEntitiesHashSet.Remove(applicationServiceNameUri);

            foreach (var leftSuspendUri in suspendedEntitiesHashSet)
            {

                if (maxResultsLocal != 0)
                {
                    // if maxResults is equal to zero, then we dont need to sort as we have already filtered the elements.
                    suspendedEntitiesHashSet.ToList().Sort();
                }
                FabricBackupResourceType leftSuspendUriType =
                UtilityHelper
                        .GetApplicationAndServicePartitionUri(
                            leftSuspendUri, out applicationNameUri, out serviceNameUri,
                            out partitionId);
                BackupEntityConfigurationInfo backupConfigurationInfo = null;
                BackupMappingModel backupMapping = null;
                BackupEntityKind backupEntityKind = BackupEntityKind.Invalid;

                switch (leftSuspendUriType)
                {
                    case FabricBackupResourceType.PartitionUri:
                        backupMapping = await this.BackupMappingStore.GetValueAsync(serviceNameUri);
                        backupEntityKind = backupMapping != null ? BackupEntityKind.Service : BackupEntityKind.Invalid;
                        backupMapping = await this.BackupMappingStore.GetValueAsync(applicationNameUri);
                        backupEntityKind = backupMapping != null ? BackupEntityKind.Application : BackupEntityKind.Invalid;

                        backupConfigurationInfo = new PartitionBackupConfigurationInfo(serviceNameUri,
                                partitionId, backupMapping.BackupPolicyName, backupEntityKind,
                                new BackupSuspensionInfo(BackupEntityKind.Partition, true));
                        break;

                    case FabricBackupResourceType.ServiceUri:
                        backupMapping = await this.BackupMappingStore.GetValueAsync(applicationNameUri);
                        backupEntityKind = backupMapping != null ? BackupEntityKind.Application : BackupEntityKind.Invalid;

                        backupConfigurationInfo = new ServiceBackupConfigurationInfo(serviceNameUri,
                            backupMapping.BackupPolicyName, backupEntityKind,
                            new BackupSuspensionInfo(BackupEntityKind.Service, true));
                        break;
                    case FabricBackupResourceType.ApplicationUri:
                        backupMapping = await this.BackupMappingStore.GetValueAsync(applicationNameUri);
                        backupEntityKind = backupMapping != null ? BackupEntityKind.Application : BackupEntityKind.Invalid;

                        backupConfigurationInfo = new ApplicationBackupConfigurationInfo(applicationNameUri,
                            backupMapping.BackupPolicyName, backupEntityKind,
                            new BackupSuspensionInfo(BackupEntityKind.Application, true));
                        break;
                }

                if (backupMapping != null && backupEntityKind != BackupEntityKind.Invalid)
                {
                    backupConfigurationInfos.Add(backupConfigurationInfo);
                    counter++;
                    if (maxResultsLocal != 0 && counter == maxResultsLocal)
                    {
                        if (counter < suspendedEntitiesHashSet.Count)
                        {
                            // we need to check if there are more elements to be listed or not.
                            brsContinuationToken.OutgoingContinuationToken = leftSuspendUri + ContinuationTokenSeparatorChar + SuspendedContinuationToken;
                        }
                        break;
                    }
                }

            }
            return backupConfigurationInfos;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get BackupPolicy Operation, fabricRequestHeader: {0} ", this.fabricRequestHeader);
            return stringBuilder.ToString();
        }
    }
}