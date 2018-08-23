// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric.WRP.Model;
    using System.Globalization;
    using System.Threading.Tasks;

    public interface IFabricClientApplicationWrapper
    {
        TraceType TraceType { get; }
        Task<IFabricOperationResult> GetAsync(IOperationDescription description, IOperationContext context);
        Task CreateAsync(IOperationDescription description, IOperationContext context);
        Task UpdateAsync(IOperationDescription description, IFabricOperationResult result, IOperationContext context);
        Task DeleteAsync(IOperationDescription description, IOperationContext context);
    }

    public abstract class FabricClientApplicationWrapper : IFabricClientApplicationWrapper
    {
        public abstract TraceType TraceType { get; protected set; }

        public abstract Task<IFabricOperationResult> GetAsync(IOperationDescription description, IOperationContext context);

        public abstract Task CreateAsync(IOperationDescription description, IOperationContext context);

        public virtual Task UpdateAsync(IOperationDescription description, IFabricOperationResult result, IOperationContext context)
        {
            return Task.FromResult(0);
        }

        public abstract Task DeleteAsync(IOperationDescription description, IOperationContext context);

        protected bool ValidObjectType<T>(object obj, out string errorMessage)
        {
            errorMessage = string.Empty;
            if (!(obj is T))
            {
                errorMessage =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Unexpected type found. Expected: {0}. Actual: {1}.",
                        typeof(T),
                        obj.GetType());
                Trace.WriteWarning(
                    this.TraceType,
                    errorMessage);
                return false;
            }

            return true;
        }
    }

    public interface IFabricOperationResult
    {
        IOperationStatus OperationStatus { get; }
        IFabricQueryResult QueryResult { get; }
    }

    public interface IFabricQueryResult
    {
    }

    public class FabricOperationResult : IFabricOperationResult
    {
        public IOperationStatus OperationStatus { get; set; }
        public IFabricQueryResult QueryResult { get; set; }
    }

    public static class FabricOperationExtensions
    {
        public static string ToFormattedString(this IOperationStatus status)
        {
            var formattedString = string.Empty;
            if (status != null)
            {
                formattedString = $@"
OperationSequenceNumber: {status.OperationSequenceNumber}
OperationType: {status.OperationType}
ResourceId: {status.ResourceId}
Status: {status.Status}
Progress: {status.Progress}
ErrorDetails: {status.ErrorDetails}
";}
            return formattedString;
        }
    }
}