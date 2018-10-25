// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;

    /// <summary>
    /// Note: Please keep updated AllFabricRetyableErrors if you add/update error rules in this class.
    /// </summary>
    internal class FabricClientRetryErrors
    {
        /// <summary>
        /// Gets all fabric client retryable errors.
        /// 
        /// TODO: RDBug 11875738: FabricClientRetryErrors.All: Fill the info using reflection over the all defined retryable error rules per operation.
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> All = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();

            // RetryableFabricErrorCodes
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.FabricHealthEntityNotFound);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PLBNotReady);
            // RDBugs: 6535276, 6737159
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.InvalidReplicaStateForReplicaOperation);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ObjectClosed);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ServiceNotFound);

            // RetrySuccessFabricErrorCodes
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadySecondaryReplica);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadyPrimaryReplica);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricVersionAlreadyExists);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricUpgradeInProgress);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricAlreadyInTargetVersion);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.StopInProgress);

            // RetryableExceptions
            retryErrors.RetryableExceptions.Add(typeof(FabricServiceNotFoundException));

            // InternalRetrySuccessFabricErrorCodes
            // Temporary workaround for http://vstfrd:8080/Azure/RD/_workitems/edit/4806429
            retryErrors.InternalRetrySuccessFabricErrorCodes.Add((uint)2147949808);

            // RetrySuccessExceptions

            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> GetEntityHealthFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.FabricHealthEntityNotFound);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> MoveSecondaryFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadySecondaryReplica);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PLBNotReady);
            // RDBugs: 6535276, 6737159
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.InvalidReplicaStateForReplicaOperation);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> MovePrimaryFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadyPrimaryReplica);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PLBNotReady);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> RemoveReplicaErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ObjectClosed);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> RestartReplicaErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ObjectClosed);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> GetPartitionListFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ServiceNotFound);
            retryErrors.RetryableExceptions.Add(typeof(FabricServiceNotFoundException));
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> GetClusterManifestFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> ProvisionFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricVersionAlreadyExists);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> UpgradeFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricUpgradeInProgress);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricAlreadyInTargetVersion);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> RemoveUnreliableTransportBehaviorErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            // Temporary workaround for http://vstfrd:8080/Azure/RD/_workitems/edit/4806429
            retryErrors.InternalRetrySuccessFabricErrorCodes.Add((uint)2147949808);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> StartNodeErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.StopInProgress);
            return retryErrors;
        });

        public static readonly Lazy<FabricClientRetryErrors> GetDeployedClusterEntityErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.InvalidAddress);
            return retryErrors;
        });

        public FabricClientRetryErrors()
        {
            this.RetryableExceptions = new List<Type>();
            this.RetryableFabricErrorCodes = new List<FabricErrorCode>();
            this.RetrySuccessExceptions = new List<Type>();
            this.RetrySuccessFabricErrorCodes = new List<FabricErrorCode>();

            this.InternalRetrySuccessFabricErrorCodes = new List<uint>();

            this.PopulateDefautlValues();
        }

        public IList<Type> RetryableExceptions { get; private set; }

        public IList<FabricErrorCode> RetryableFabricErrorCodes { get; private set; }

        public IList<Type> RetrySuccessExceptions { get; private set; }

        public IList<FabricErrorCode> RetrySuccessFabricErrorCodes { get; private set; }

        public IList<uint> InternalRetrySuccessFabricErrorCodes { get; private set; }

        private void PopulateDefautlValues()
        {
            this.RetryableExceptions.Add(typeof(TimeoutException));
            this.RetryableExceptions.Add(typeof(OperationCanceledException));
            this.RetryableExceptions.Add(typeof(FabricTransientException));

            this.RetryableFabricErrorCodes.Add(FabricErrorCode.OperationTimedOut);
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.CommunicationError);
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.GatewayNotReachable);
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.ServiceTooBusy);
        }

        public bool IsRetryable(Exception e)
        {         
            Exception currentEx = e;
            for (int i = 0; i < 5 && currentEx != null; i++)
            {
                if (this.RetryableExceptions.Contains(currentEx.GetType()))
                {   
                    return true;
                }

                currentEx = currentEx.InnerException;
            }

            FabricException fabricException = e as FabricException;
            if (fabricException != null && this.RetryableFabricErrorCodes.Contains(fabricException.ErrorCode))
            {
                return true;
            }

            if (this.RetrySuccessExceptions.Contains(e.GetType()))
            {   
                return true;
            }

            if (fabricException != null && this.RetrySuccessFabricErrorCodes.Contains(fabricException.ErrorCode))
            {
                return true;
            }

            if (e.GetType() == typeof(FabricTransientException))
            {   
                return true;
            }

            if (fabricException != null && fabricException.InnerException != null)
            {
                var ex = fabricException.InnerException;

                if (this.InternalRetrySuccessFabricErrorCodes.Contains((uint)ex.HResult))
                {
                    return true;
                }
            }

            return false;
        }

    }
}