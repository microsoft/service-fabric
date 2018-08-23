// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.WRP.Model;
    using System.Linq;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;

    internal class ApplicationCommandProcessor : ResourceCommandProcessor
    {
        private readonly IFabricClientApplicationWrapper fabricClient;

        public ApplicationCommandProcessor(IFabricClientApplicationWrapper fabricClient)
        {
            fabricClient.ThrowIfNull(nameof(fabricClient));
            this.fabricClient = fabricClient;
        }

        public override ResourceType Type { get; } = ResourceType.Application;
        public override TraceType TraceType { get; } = new TraceType("ApplicationCommandProcessor");

        public override async Task<IList<IOperationStatus>> ProcessAsync(
            IList<IOperationDescription> descriptions,
            IOperationContext context)
        {
            descriptions.ThrowIfNull(nameof(descriptions));
            context.ThrowIfNull(nameof(context));

            // TODO: ContinuationToken support
            var operationContext =
                new OperationContext(
                    context.CancellationToken,
                    context.GetRemainingTimeOrThrow(),
                    string.Empty);
            return await base.ProcessAsync(descriptions, operationContext);
        }

        public override async Task<IOperationStatus> CreateOperationStatusAsync(
            IOperationDescription description,
            IOperationContext context)
        {
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            Trace.WriteInfo(TraceType, "CreateOperationStatusAsync called");
            IOperationStatus status = null;
            try
            {
                IFabricOperationResult result = null;
                if (!this.Operations.OperationExists(description))
                {
                    result = await this.fabricClient.GetAsync(description, context);
                    status = result?.OperationStatus;
                    var operationContext =
                        new OperationContext(
                            context.CancellationToken,
                            Constants.MaxOperationTimeout,
                            context.ContinuationToken);
                    switch (description.OperationType)
                    {
                        case OperationType.CreateOrUpdate:
                            if (null == status)
                            {
                                try
                                {
                                    await this.fabricClient.CreateAsync(
                                        description,
                                        operationContext);
                                }
                                catch (Exception e)
                                {
                                    return this.HandleOperationFailedStatus(
                                        description,
                                        nameof(this.fabricClient.CreateAsync),
                                        e);
                                }

                                break;
                            }

                            try
                            {
                                await this.fabricClient.UpdateAsync(
                                    description,
                                    result,
                                    operationContext);
                            }
                            catch (Exception e)
                            {
                                return this.HandleOperationFailedStatus(
                                    description,
                                    nameof(this.fabricClient.UpdateAsync),
                                    e);
                            }

                            break;
                        case OperationType.Delete:
                            try
                            {
                                await this.fabricClient.DeleteAsync(
                                    description,
                                    operationContext);
                            }
                            catch (Exception e)
                            {
                                return this.HandleOperationFailedStatus(
                                    description,
                                    nameof(this.fabricClient.DeleteAsync),
                                    e);
                            }

                            break;
                        default:
                            Trace.WriteError(this.TraceType, "Unhandled operation type: {0}", description.OperationType);
                            break;
                    }

                    Trace.WriteInfo(
                        this.TraceType,
                        this.Operations.TryAdd(description)
                            ? "CreateOperationStatusAsync: Operation started for resource. OperationId: {0}. ResourceId: {1}."
                            : "CreateOperationStatusAsync: Failed to track operation for resource. OperationId: {0}. ResourceId: {1}.",
                        description.OperationSequenceNumber,
                        description.ResourceId);
                }

                result = await this.fabricClient.GetAsync(description, context);
                status = result?.OperationStatus;
            }
            catch (Exception e)
            {
                ResourceCommandProcessor.HandleException("CreateOperationStatusAsync", e, this.TraceType);
            }

            Trace.WriteInfo(
                TraceType,
                "CreateOperationStatusAsync: completed {0}",
                status.ToFormattedString());
            return status;
        }

        private IOperationStatus HandleOperationFailedStatus(
            IOperationDescription description,
            string operationName,
            Exception e)
        {
            if (IsTransientException(e))
            {
                Trace.WriteError(this.TraceType, "{0}: transient exception encountered: {1}{2}", operationName, Environment.NewLine, e);
                return null;
            }

            Trace.WriteError(this.TraceType, "{0}: Operation failed{1}{2}", operationName, Environment.NewLine, e);
            return new ApplicationOperationStatus((ApplicationOperationDescription) description)
            {
                Status = ResultStatus.Failed,
                ErrorDetails = JObject.FromObject(new {Details = e})
            };
        }
    }
}