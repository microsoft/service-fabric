// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;

    /// <summary>
    /// Class to define retry-able fabric client errors
    /// </summary>
    public class FabricClientRetryErrors
    {
        /// <summary>
        /// Fabric errors that are retry-able for fabric client GetEntityHealth commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> GetEntityHealthFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.FabricHealthEntityNotFound);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client MoveSecondary commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> MoveSecondaryFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadySecondaryReplica);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PLBNotReady);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client MovePrimary commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> MovePrimaryFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.AlreadyPrimaryReplica);
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.PLBNotReady);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client RemoveReplica commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> RemoveReplicaErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ObjectClosed);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client RestartReplica commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> RestartReplicaErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ObjectClosed);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client GetPartitionList commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> GetPartitionListFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetryableFabricErrorCodes.Add(FabricErrorCode.ServiceNotFound);
            retryErrors.RetryableExceptions.Add(typeof(FabricServiceNotFoundException));
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client GetClusterManifest commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> GetClusterManifestFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client Provision commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> ProvisionFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricVersionAlreadyExists);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client Upgrade commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> UpgradeFabricErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricUpgradeInProgress);
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.FabricAlreadyInTargetVersion);
            return retryErrors;
        });

        /// <summary>
        /// Fabric errors that are retry-able for fabric client RemoveUnreliableTransportBehavior commands
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> RemoveUnreliableTransportBehaviorErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.InternalRetrySuccessFabricErrorCodes.Add(2147949808);
            return retryErrors;
        });

         /// <summary>
        /// Setting SuccessFabricErrorCodes while performing CreateApp
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> CreateAppErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.ApplicationAlreadyExists);
            return retryErrors;
        });

        /// <summary>
        /// Setting SuccessFabricErrorCodes while performing DeleteApp
        /// </summary>
        public static readonly Lazy<FabricClientRetryErrors> DeleteAppErrors = new Lazy<FabricClientRetryErrors>(() =>
        {
            var retryErrors = new FabricClientRetryErrors();
            retryErrors.RetrySuccessFabricErrorCodes.Add(FabricErrorCode.ApplicationNotFound);
            return retryErrors;
        });

        /// <summary>
        /// Constructor that populates default retry-able errors
        /// </summary>
        public FabricClientRetryErrors()
        {
            this.RetryableExceptions = new List<Type>();
            this.RetryableFabricErrorCodes = new List<FabricErrorCode>();
            this.RetrySuccessExceptions = new List<Type>();
            this.RetrySuccessFabricErrorCodes = new List<FabricErrorCode>();

            this.InternalRetrySuccessFabricErrorCodes = new List<uint>();

            this.PopulateDefaultValues();
        }

        /// <summary>
        /// List of exceptions that are retry-able
        /// </summary>
        public IList<Type> RetryableExceptions { get; private set; }

        /// <summary>
        /// List of Fabric error codes that are retry-able
        /// </summary>
        public IList<FabricErrorCode> RetryableFabricErrorCodes { get; private set; }

        /// <summary>
        /// List of success exceptions that are retry-able
        /// </summary>
        public IList<Type> RetrySuccessExceptions { get; private set; }

        /// <summary>
        /// List of success error codes that are retry-able
        /// </summary>
        public IList<FabricErrorCode> RetrySuccessFabricErrorCodes { get; private set; }

        /// <summary>
        /// List of internal success error codes that are retry-able
        /// </summary>
        internal IList<uint> InternalRetrySuccessFabricErrorCodes { get; private set; }

        private void PopulateDefaultValues()
        {
            this.RetryableExceptions.Add(typeof(TimeoutException));
            this.RetryableExceptions.Add(typeof(OperationCanceledException));
            this.RetryableExceptions.Add(typeof(FabricNotReadableException));
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.OperationTimedOut);
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.CommunicationError);

            // TODO: Enable after updating ServiceFabricClientPackage in SF-AppStore Repo
            // this.RetryableFabricErrorCodes.Add(FabricErrorCode.GatewayNotReachable);
            this.RetryableFabricErrorCodes.Add(FabricErrorCode.ServiceTooBusy);
        }
    }
}