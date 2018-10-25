// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using Linq;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    #pragma warning disable 1998
    internal class GetSelectedPartitionStateAction : FabricTestAction<SelectedPartition>
    {
        public GetSelectedPartitionStateAction(PartitionSelector partitionSelector)
        {
            this.PartitionSelector = partitionSelector;
        }

        public PartitionSelector PartitionSelector { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(GetSelectedPartitionStateActionHandler); }
        }

        private class GetSelectedPartitionStateActionHandler : FabricTestActionHandler<GetSelectedPartitionStateAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, GetSelectedPartitionStateAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "PartitionSelector");

                Guid partitionId;
                Uri serviceName;
                if (!action.PartitionSelector.TryGetPartitionIdIfNotGetServiceName(out partitionId, out serviceName))
                {
                    // TODO: make these actions which store state locally as well. 
                    ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ServicePartitionList>(
                        () => 
                            testContext.FabricClient.QueryManager.GetPartitionListAsync(
                            serviceName, 
                            null, 
                            default(string), 
                            action.RequestTimeout, 
                            cancellationToken),                        
                        action.ActionTimeout,
                        cancellationToken).ConfigureAwait(false);

                    Partition partitionResult = action.PartitionSelector.GetSelectedPartition(partitionsResult.ToArray(), testContext.Random);

                    partitionId = partitionResult.PartitionInformation.Id;
                }
                else
                {
                    // Validate the partition specified is actually from the service specified.
                    // Intentionally do not use FabricClientRetryErrors.GetPartitionListFabricErrors.  We do not want to retry "service not found".                
                    ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ServicePartitionList>(
                        () => testContext.FabricClient.QueryManager.GetPartitionListAsync(
                            serviceName,
                            null,
                            default(string),
                            action.RequestTimeout,
                            cancellationToken),
                        action.ActionTimeout,
                        cancellationToken).ConfigureAwait(false);

                    var guids = partitionsResult.Select(p => p.PartitionId()).ToList();
                    if (!guids.Contains(partitionId))
                    {
                        // The message in the first arg is only for debugging, it is not returned to the user.                        
                        throw new FabricException("Partition not found", FabricErrorCode.PartitionNotFound);
                    }
                }

                action.Result = new SelectedPartition(serviceName, partitionId);

                ResultTraceString = StringHelper.Format("PartitionSelector Selected Partition with ID {0}", action.Result);
            }
        }
    }
}