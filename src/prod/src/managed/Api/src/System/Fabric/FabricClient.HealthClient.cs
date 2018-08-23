// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides functionality to perform health related operations,
        /// like report and query health.</para>
        /// </summary>
        public sealed class HealthClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricHealthClient4 nativeHealthClient;

            #endregion

            #region Constructor

            internal HealthClient(FabricClient fabricClient, NativeClient.IFabricHealthClient4 nativeHealthClient)
            {
                this.fabricClient = fabricClient;
                this.nativeHealthClient = nativeHealthClient;
            }

            #endregion

            #region API

            ///<summary>
            /// <para>Reports health on a Service Fabric entity.</para>
            ///</summary>
            ///<param name="healthReport">
            /// <para>The <see cref="System.Fabric.Health.HealthReport"/> to submit.</para>
            ///</param>
            ///<exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            ///</exception>
            ///<exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            ///</exception>
            ///<exception cref="System.Fabric.FabricException">
            ///     <para>Caused by one of the following:</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            ///</exception>
            ///<exception cref="System.UnauthorizedAccessException">
            ///     <para>Caused by one of the following:</para>
            ///     <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            ///</exception>
            ///<remarks>
            ///<para>
            /// When a cluster is secured, the health client needs administrator permission to be able to send the reports.
            /// Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-connect-to-secure-cluster">connecting to a cluster using the FabricClient APIs</see>.
            ///</para>
            ///<para>
            /// For more information about health reporting, see <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-health-introduction">Service Fabric health monitoring</see>.
            ///</para>
            ///</remarks>
            public void ReportHealth(HealthReport healthReport)
            {
                Requires.Argument<HealthReport>("healthReport", healthReport).NotNull();
                Utility.WrapNativeSyncInvokeInMTA(() => this.ReportHealthHelper(healthReport, null), "HealthClient.ReportHealth");
            }

            ///<summary>
            /// <para>Reports health on a Service Fabric entity and passes send options to control how the report is sent.</para>
            ///</summary>
            ///<param name="healthReport">
            /// <para>The <see cref="System.Fabric.Health.HealthReport"/> to submit.</para>
            ///</param>
            ///<param name="sendOptions">
            ///<para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
            ///</param>
            ///<remarks>
            ///<para>
            /// When a cluster is secured, the health client needs administrator permission to be able to send the reports.
            /// Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-connect-to-secure-cluster">connecting to a cluster using the FabricClient APIs.</see>
            ///</para>
            ///<para>
            /// For more information about health reporting, see <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-health-introduction">Service Fabric health monitoring</see>.
            ///</para>
            ///</remarks>
            ///<exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            ///</exception>
            ///<exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            ///</exception>
            ///<exception cref="System.Fabric.FabricException">
            ///     <para>Caused by one of the following:</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            ///</exception>
            ///<exception cref="System.UnauthorizedAccessException">
            ///     <para>Caused by one of the following:</para>
            ///     <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            ///</exception>
            public void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
            {
                Requires.Argument<HealthReport>("healthReport", healthReport).NotNull();
                Utility.WrapNativeSyncInvokeInMTA(() => this.ReportHealthHelper(healthReport, sendOptions), "HealthClient.ReportHealth");
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster.</para>
            /// </summary>
            /// <returns>
            /// <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. 
            /// Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of the cluster.</para>
            ///   <code language="c#">public static bool GetClusterHealth(string clusterConnection)
            /// {
            ///     ClusterHealth clusterHealth;
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the cluster health.
            ///     try
            ///     {
            ///         clusterHealth = fabricClient.HealthManager.GetClusterHealthAsync().Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     // Display the cluster health state.
            ///     Console.WriteLine("Cluster Health:");
            ///     Console.WriteLine("  Aggregated Health State: " + clusterHealth.AggregatedHealthState);
            ///     Console.WriteLine();
            /// 
            ///     // Display the application health states on the cluster.
            ///     Console.WriteLine("  Application Health States:");
            ///     IList&lt;ApplicationHealthState&gt; applicationHealthStateList = clusterHealth.ApplicationHealthStates;
            ///     foreach(ApplicationHealthState applicationHealthState in applicationHealthStateList)
            ///     {
            ///         Console.WriteLine("    Application {0}: {1}", applicationHealthState.ApplicationName, applicationHealthState.AggregatedHealthState);
            ///     }
            /// 
            ///     // Display Node Health States on the cluster.
            ///     Console.WriteLine("  Node Health States:");
            ///     IList&lt;NodeHealthState&gt; nodeHealthStateList = clusterHealth.NodeHealthStates;
            ///     foreach (NodeHealthState nodeHealthState in nodeHealthStateList)
            ///     {
            ///         Console.WriteLine("    Node Name: " + nodeHealthState.NodeName);
            ///         Console.WriteLine("      Aggregated Health State: " + nodeHealthState.AggregatedHealthState);
            ///     }
            /// 
            ///     // Display Health Events.
            ///     Console.WriteLine("  Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEventList = clusterHealth.HealthEvents;
            ///     foreach(HealthEvent healthEvent in healthEventList)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     Console.WriteLine("  Unhealthy Evaluations:");
            ///     IList&lt;HealthEvaluation&gt; healthEvaluationList = clusterHealth.UnhealthyEvaluations;
            ///     foreach(HealthEvaluation healthEvaluation in healthEvaluationList)
            ///     {
            ///         Console.WriteLine("    Kind: " + healthEvaluation.Kind);
            ///         Console.WriteLine("      Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///         Console.WriteLine("      Description: " + healthEvaluation.Description);
            ///     }
            /// 
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<ClusterHealth> GetClusterHealthAsync()
            {
                var queryDescription = new ClusterHealthQueryDescription();
                return this.GetClusterHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster, evaluating it using the specified policy.</para>
            /// </summary>
            /// <param name="healthPolicy">The cluster health policy used to evaluate cluster health.</param>
            /// <returns>
            ///     <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            ///     <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. 
            ///     Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            ///     <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            ///     <para>Caused by one of the following:</para>
            ///     <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            ///     <para>Caused by one of the following:</para>
            ///     <para><see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealth> GetClusterHealthAsync(ClusterHealthPolicy healthPolicy)
            {
                var queryDescription = new ClusterHealthQueryDescription() { HealthPolicy = healthPolicy, };
                return this.GetClusterHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealth> GetClusterHealthAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ClusterHealthQueryDescription();
                return this.GetClusterHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster by using the specified health policy, timeout and cancellation token.</para>
            /// </summary>
            /// <param name="healthPolicy">The cluster health policy used to evaluate cluster health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealth> GetClusterHealthAsync(ClusterHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ClusterHealthQueryDescription() { HealthPolicy = healthPolicy, };
                return this.GetClusterHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster by using a query description.</para>
            /// </summary>
            /// <param name="queryDescription">The query description that defines 
            /// query parameters like health policies, filters etc.</param>
            /// <returns>
            /// <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealth> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription)
            {
                Requires.Argument<ClusterHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetClusterHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric cluster by using a query description, a timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description that defined query parameters like health policies, filters etc.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealth> GetClusterHealthAsync(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ClusterHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetClusterHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Service Fabric node name.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a node.</para>
            ///   <code language="c#">public static bool GetNodeHealth(string clusterConnection)
            /// {
            ///     NodeHealth nodeHealth;
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the node health.
            ///     try
            ///     {
            ///         nodeHealth = fabricClient.HealthManager.GetNodeHealthAsync("Node1").Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     // Display the node health information.
            ///     Console.WriteLine("Node Health:");
            ///     Console.WriteLine("  Node {0}: {1}", nodeHealth.NodeName, nodeHealth.AggregatedHealthState);
            /// 
            ///     // List the health events.
            ///     Console.WriteLine("    Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEvents = nodeHealth.HealthEvents;
            ///     foreach (HealthEvent healthEvent in healthEvents)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     // List the unhealthy evaluations.
            ///     Console.WriteLine("    Unhealthy Evaluations:");
            ///     IList&lt;HealthEvaluation&gt; healthEvaluationList = nodeHealth.UnhealthyEvaluations;
            ///     foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            ///     {
            ///         Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///         Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///         Console.WriteLine("        Description: " + healthEvaluation.Description);
            ///     }
            /// 
            ///     Console.WriteLine(); 
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<NodeHealth> GetNodeHealthAsync(string nodeName)
            {
                var queryDescription = new NodeHealthQueryDescription(nodeName);
                return this.GetNodeHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Service Fabric node name.</para>
            /// </param>
            /// <param name="healthPolicy">The cluster health policy used to evaluate the node health.</param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<NodeHealth> GetNodeHealthAsync(string nodeName, ClusterHealthPolicy healthPolicy)
            {
                var queryDescription = new NodeHealthQueryDescription(nodeName) { HealthPolicy = healthPolicy, };
                return this.GetNodeHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Service Fabric node.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the 
            /// operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<NodeHealth> GetNodeHealthAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new NodeHealthQueryDescription(nodeName);
                return this.GetNodeHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Service Fabric node.</para>
            /// </param>
            /// <param name="healthPolicy">The cluster health policy used to evaluate the node health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<NodeHealth> GetNodeHealthAsync(string nodeName, ClusterHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new NodeHealthQueryDescription(nodeName) { HealthPolicy = healthPolicy, };
                return this.GetNodeHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description that defines parameters like health policy, filters etc.</param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<NodeHealth> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription)
            {
                Requires.Argument<NodeHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetNodeHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description that defines parameters like health policy, filters etc.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric node.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<NodeHealth> GetNodeHealthAsync(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<NodeHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetNodeHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <returns>
            /// <para>The health of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of an application.</para>
            ///   <code language="c#">public static bool GetApplicationHealth(string clusterConnection)
            /// {
            ///     ApplicationHealth applicationHealth;
            ///     Uri applicationName = new Uri("fabric:/myapp/todo");
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the application health.
            ///     try
            ///     {
            ///         applicationHealth = fabricClient.HealthManager.GetApplicationHealthAsync(applicationName).Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     // Display the application health information.
            ///     Console.WriteLine("Get Application Health:");
            ///     Console.WriteLine("  Application {0}: {1}", applicationHealth.ApplicationName, applicationHealth.AggregatedHealthState);
            /// 
            ///     // List the deployed application health states.
            ///     Console.WriteLine("    Deployed applications:");
            ///     IList&lt;DeployedApplicationHealthState&gt; deployedAppHealthStateList = applicationHealth.DeployedApplicationHealthStates;
            ///     foreach (DeployedApplicationHealthState deployedAppHealthState in deployedAppHealthStateList)
            ///     {
            ///         Console.WriteLine("      Application: " + deployedAppHealthState.ApplicationName);
            ///         Console.WriteLine("        Aggregated Health State: " + deployedAppHealthState.AggregatedHealthState);
            ///         Console.WriteLine("        Node Name: " + deployedAppHealthState.NodeName);
            ///     }
            /// 
            ///     // List the deployed services health states.
            ///     Console.WriteLine("    Service Health States:");
            ///     IList&lt;ServiceHealthState&gt; deployedSvcsHealthStateList = applicationHealth.ServiceHealthStates;
            ///     foreach (ServiceHealthState serviceHealthState in deployedSvcsHealthStateList)
            ///     {
            ///         Console.WriteLine("      Service {0}: {1}", serviceHealthState.ServiceName, serviceHealthState.AggregatedHealthState);
            ///     }
            /// 
            ///     // List the health events.
            ///     Console.WriteLine("    Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEvents = applicationHealth.HealthEvents;
            ///     foreach (HealthEvent healthEvent in healthEvents)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     Console.WriteLine();
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName)
            {
                var queryDescription = new ApplicationHealthQueryDescription(applicationName);
                return this.GetApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application using the application URI and the health policy.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="healthPolicy">The <see cref="System.Fabric.Health.ApplicationHealthPolicy"/> instance used to evaluate the application.</param>
            /// <returns>
            /// <para>The health reports of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new ApplicationHealthQueryDescription(applicationName) { HealthPolicy = healthPolicy, };
                return this.GetApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application using the specified application URI, timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health reports of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ApplicationHealthQueryDescription(applicationName);
                return this.GetApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the application health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health reports of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ApplicationHealth> GetApplicationHealthAsync(Uri applicationName, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ApplicationHealthQueryDescription(applicationName) { HealthPolicy = healthPolicy, };
                return this.GetApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application 
            /// by using the specified query description.</para>
            /// </summary>
            /// <param name="queryDescription">The <see cref="System.Fabric.Description.ApplicationHealthQueryDescription"/> instance to describe the query to get application health.</param>
            /// <returns>
            /// <para>The health of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.ApplicationHealthQueryDescription.ApplicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ApplicationHealth> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription)
            {
                Requires.Argument<ApplicationHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of the specified Service Fabric application using the specified query input, timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The <see cref="System.Fabric.Description.ApplicationHealthQueryDescription"/> instance to describe the query to get application health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health reports of the specified Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.ApplicationHealthQueryDescription.ApplicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ApplicationHealth> GetApplicationHealthAsync(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ApplicationHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the Service Fabric service.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="serviceName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a service.</para>
            ///   <code language="c#">public static bool GetServiceHealth(string clusterConnection)
            /// {
            ///     ServiceHealth serviceHealth;
            ///     Uri serviceName = new Uri("fabric:/myapp/todo/svc1");
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the service health.
            ///     try
            ///     {
            ///         serviceHealth = fabricClient.HealthManager.GetServiceHealthAsync(serviceName).Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     Console.WriteLine("Service Health:");
            ///     Console.WriteLine("  Service {0}: {1}", serviceHealth.ServiceName, serviceHealth.AggregatedHealthState);
            /// 
            ///     // List the health states.
            ///     Console.WriteLine("    Partition Health States:");
            ///     IList&lt;PartitionHealthState&gt; partitionHealthStates = serviceHealth.PartitionHealthStates;
            ///     foreach (PartitionHealthState partitionHealthState in partitionHealthStates)
            ///     {
            ///         Console.WriteLine("      Aggregated Health State: " + partitionHealthState.AggregatedHealthState);
            ///         Console.WriteLine("      Partition ID: " + partitionHealthState.PartitionId);
            ///     }
            /// 
            ///     // List the health events.
            ///     Console.WriteLine("    Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEvents = serviceHealth.HealthEvents;
            ///     foreach (HealthEvent healthEvent in healthEvents)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     // List the unhealthy evaluations.
            ///     Console.WriteLine("    Unhealthy Evaluations:");
            ///     IList&lt;HealthEvaluation&gt; healthEvaluationList = serviceHealth.UnhealthyEvaluations;
            ///     foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            ///     {
            ///         Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///         Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///         Console.WriteLine("        Description: " + healthEvaluation.Description);
            ///     }
            /// 
            ///     Console.WriteLine();
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<ServiceHealth> GetServiceHealthAsync(Uri serviceName)
            {
                var queryDescription = new ServiceHealthQueryDescription(serviceName);
                return this.GetServiceHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the Service Fabric service.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate service health.</param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="serviceName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ServiceHealth> GetServiceHealthAsync(Uri serviceName, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new ServiceHealthQueryDescription(serviceName) { HealthPolicy = healthPolicy, };
                return this.GetServiceHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the Service Fabric service.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="serviceName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ServiceHealth> GetServiceHealthAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ServiceHealthQueryDescription(serviceName);
                return this.GetServiceHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the Service Fabric service.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="serviceName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ServiceHealth> GetServiceHealthAsync(Uri serviceName, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ServiceHealthQueryDescription(serviceName) { HealthPolicy = healthPolicy, };
                return this.GetServiceHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.ServiceHealthQueryDescription.ServiceName"/> 
            ///     is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ServiceHealth> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription)
            {
                Requires.Argument<ServiceHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetServiceHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric service by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.ServiceHealthQueryDescription.ServiceName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ServiceHealth> GetServiceHealthAsync(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ServiceHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetServiceHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a partition.</para>
            ///   <code language="c#">public static bool GetPartitionHealth(string clusterConnection)
            /// {
            ///     PartitionHealth partitionHealth;
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the partition health.
            ///     try
            ///     {
            ///         partitionHealth = fabricClient.HealthManager.GetPartitionHealthAsync(new Guid("a7206315-e53b-4d05-b59c-e210caa28893")).Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            ///             Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     // Display the partition health information.
            ///     Console.WriteLine("Partition Health:");
            ///     Console.WriteLine("  Partition ID: " + partitionHealth.PartitionId);
            ///     Console.WriteLine("    Aggregated Health State: " + partitionHealth.AggregatedHealthState);
            /// 
            ///     // List the health events.
            ///     Console.WriteLine("    Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEvents = partitionHealth.HealthEvents;
            ///     foreach (HealthEvent healthEvent in healthEvents)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     // List the replica health states.
            ///     Console.WriteLine("    Replica Health States:");
            ///     IList&lt;ReplicaHealthState&gt; replicaHealthStates = partitionHealth.ReplicaHealthStates;
            ///     foreach (ReplicaHealthState replicaHealthState in replicaHealthStates)
            ///     {
            ///         Console.WriteLine("      ID: " + replicaHealthState.Id);
            ///         Console.WriteLine("        Kind: " + replicaHealthState.Kind);
            ///         Console.WriteLine("        Aggregated Health State: " + replicaHealthState.AggregatedHealthState);
            ///         Console.WriteLine("        Partition ID: " + replicaHealthState.PartitionId);
            ///     }
            /// 
            ///     // List the unhealthy evaluations.
            ///     Console.WriteLine("    Unhealthy Evaluations:");
            ///     IList&lt;HealthEvaluation&gt; healthEvaluationList = partitionHealth.UnhealthyEvaluations;
            ///     foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            ///     {
            ///         Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///         Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///         Console.WriteLine("        Description: " + healthEvaluation.Description);
            ///     }
            /// 
            ///     Console.WriteLine();
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<PartitionHealth> GetPartitionHealthAsync(Guid partitionId)
            {
                var queryDescription = new PartitionHealthQueryDescription(partitionId);
                return this.GetPartitionHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<PartitionHealth> GetPartitionHealthAsync(Guid partitionId, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new PartitionHealthQueryDescription(partitionId) { HealthPolicy = healthPolicy, };
                return this.GetPartitionHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<PartitionHealth> GetPartitionHealthAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new PartitionHealthQueryDescription(partitionId);
                return this.GetPartitionHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<PartitionHealth> GetPartitionHealthAsync(Guid partitionId, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new PartitionHealthQueryDescription(partitionId) { HealthPolicy = healthPolicy, };
                return this.GetPartitionHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<PartitionHealth> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription)
            {
                Requires.Argument<PartitionHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetPartitionHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric partition by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric partition.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<PartitionHealth> GetPartitionHealthAsync(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<PartitionHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetPartitionHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }
            
            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The ID of the Service Fabric replica.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a replica.</para>
            ///   <code language="c#">public static bool GetReplicaHealth(string clusterConnection)
            /// {
            ///     ReplicaHealth replicaHealth;
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     // Get the replica health.
            ///     try
            ///     {
            ///         replicaHealth = fabricClient.HealthManager.GetReplicaHealthAsync(
            /// new Guid("a7206315-e53b-4d05-b59c-e210caa28893"), 130538257146083818).Result;
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     Console.WriteLine("Replica Health:");
            ///     Console.WriteLine("  ID: " + replicaHealth.Id);
            ///     Console.WriteLine("    Aggregated Health State: " + replicaHealth.AggregatedHealthState);
            ///     Console.WriteLine("    Kind: " + replicaHealth.Kind);
            ///     Console.WriteLine("    Partition ID: " + replicaHealth.PartitionId);
            /// 
            ///     // List the health events.
            ///     Console.WriteLine("    Health Events:");
            ///     IList&lt;HealthEvent&gt; healthEvents = replicaHealth.HealthEvents;
            ///     foreach (HealthEvent healthEvent in healthEvents)
            ///     {
            ///         Console.WriteLine("      Health Event:");
            ///         Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///         Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///         Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///         Console.WriteLine("        Health Information:");
            ///         Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///         Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///         Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///         Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///         Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///         Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///         Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            ///     }
            /// 
            ///     // List the unhealthy evaluations.
            ///     Console.WriteLine("    Unhealthy Evaluations:");
            ///     IList&lt;HealthEvaluation&gt; healthEvaluationList = replicaHealth.UnhealthyEvaluations;
            ///     foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            ///     {
            ///         Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///         Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///         Console.WriteLine("        Description: " + healthEvaluation.Description);
            ///     }
            /// 
            ///     Console.WriteLine(); 
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<ReplicaHealth> GetReplicaHealthAsync(Guid partitionId, long replicaId)
            {
                var queryDescription = new ReplicaHealthQueryDescription(partitionId, replicaId);
                return this.GetReplicaHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The ID of the Service Fabric replica.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ReplicaHealth> GetReplicaHealthAsync(Guid partitionId, long replicaId, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new ReplicaHealthQueryDescription(partitionId, replicaId) { HealthPolicy = healthPolicy, };
                return this.GetReplicaHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The ID of the Service Fabric replica.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ReplicaHealth> GetReplicaHealthAsync(Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ReplicaHealthQueryDescription(partitionId, replicaId);
                return this.GetReplicaHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The ID of the Service Fabric partition.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The ID of the Service Fabric replica.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ReplicaHealth> GetReplicaHealthAsync(Guid partitionId, long replicaId, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ReplicaHealthQueryDescription(partitionId, replicaId) { HealthPolicy = healthPolicy, };
                return this.GetReplicaHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica specified by the query description.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ReplicaHealth> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription)
            {
                Requires.Argument<ReplicaHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetReplicaHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a Service Fabric replica by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a Service Fabric replica.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ReplicaHealth> GetReplicaHealthAsync(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ReplicaHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetReplicaHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The node name where the Service Fabric application is deployed.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a deployed application.</para>
            ///   <code language="c#">public static bool GetDeployedApplicationsHealth(string clusterConnection)
            /// {
            ///     //DeployedApplicationHealth deployedApplicationHealth;
            ///     //ApplicationHealth applicationHealth;
            ///     Uri applicationName = new Uri("fabric:/myapp/todo");
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     Console.WriteLine("Deployed Application Health:");
            /// 
            ///     try
            ///     {
            ///         // Determine the nodes where the application has been deployed.
            ///         ApplicationHealth applicationHealth = fabricClient.HealthManager.GetApplicationHealthAsync(applicationName).Result;
            /// 
            ///         // Get the deployed application health for each node.
            ///         IList&lt;DeployedApplicationHealthState&gt; deployedAppHealthStateList = applicationHealth.DeployedApplicationHealthStates;
            ///         foreach (DeployedApplicationHealthState deployedAppHealthState in deployedAppHealthStateList)
            ///         {
            /// DeployedApplicationHealth deployedApplicationHealth = fabricClient.HealthManager.GetDeployedApplicationHealthAsync(applicationName, 
            ///     deployedAppHealthState.NodeName).Result;
            /// 
            /// // Display the deployed application health information for each node.
            /// 
            /// Console.WriteLine("  Application {0}: {1}", deployedApplicationHealth.ApplicationName, deployedApplicationHealth.AggregatedHealthState);
            /// Console.WriteLine("    Node Name: " + deployedApplicationHealth.NodeName);
            /// 
            /// // List the deployed application health states.
            /// Console.WriteLine("    Deployed applications:");
            /// IList&lt;DeployedServicePackageHealthState&gt; deployedSPHealthStateList = deployedApplicationHealth.DeployedServicePackageHealthStates;
            /// foreach (DeployedServicePackageHealthState deployedSPHealthState in deployedSPHealthStateList)
            /// {
            ///     Console.WriteLine("      Application: " + deployedSPHealthState.ApplicationName);
            ///     Console.WriteLine("        Aggregated Health State: " + deployedSPHealthState.AggregatedHealthState);
            ///     Console.WriteLine("        Node Name: " + deployedSPHealthState.NodeName);
            ///     Console.WriteLine("        Service Manifest Name: " + deployedSPHealthState.ServiceManifestName);
            /// }
            /// 
            /// // List the health events.
            /// Console.WriteLine("    Health Events:");
            /// IList&lt;HealthEvent&gt; healthEvents = deployedApplicationHealth.HealthEvents;
            /// foreach (HealthEvent healthEvent in healthEvents)
            /// {
            ///     Console.WriteLine("      Health Event:");
            ///     Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///     Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///     Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///     Console.WriteLine("        Health Information:");
            ///     Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///     Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///     Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///     Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///     Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///     Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///     Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            /// }
            /// 
            /// // List the unhealthy evaluations.
            /// Console.WriteLine("    Unhealthy Evaluations:");
            /// IList&lt;HealthEvaluation&gt; healthEvaluationList = deployedApplicationHealth.UnhealthyEvaluations;
            /// foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            /// {
            ///     Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///     Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///     Console.WriteLine("        Description: " + healthEvaluation.Description);
            /// }
            /// 
            /// Console.WriteLine();
            ///         }
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            /// 
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(Uri applicationName, string nodeName)
            {
                var queryDescription = new DeployedApplicationHealthQueryDescription(applicationName, nodeName);
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The node name where the Service Fabric application is deployed.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate entity health.</param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric application.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(Uri applicationName, string nodeName, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new DeployedApplicationHealthQueryDescription(applicationName, nodeName) { HealthPolicy = healthPolicy, };
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The node name where the Service Fabric application is deployed.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(Uri applicationName, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new DeployedApplicationHealthQueryDescription(applicationName, nodeName);
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The node name where the Service Fabric application is deployed.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(Uri applicationName, string nodeName, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new DeployedApplicationHealthQueryDescription(applicationName, nodeName) { HealthPolicy = healthPolicy, };
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.DeployedApplicationHealthQueryDescription.ApplicationName" /> 
            ///     is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription)
            {
                Requires.Argument<DeployedApplicationHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric application on the specified node by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.DeployedApplicationHealthQueryDescription.ApplicationName" /> 
            ///     is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsync(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<DeployedApplicationHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetDeployedApplicationHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest file for this Service Fabric service.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The name of the node that the Service Fabric service was deployed to.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            /// <remarks>
            /// <para>The following example gets the health of a deployed service package.</para>
            ///   <code language="c#">public static bool GetDeployedServicePackageHealth(string clusterConnection)
            /// {
            ///     DeployedApplicationHealth deployedApplicationHealth;
            ///     DeployedServicePackageHealth deployedServicePackageHealth;
            ///     ApplicationHealth applicationHealth;
            ///     Uri applicationName = new Uri("fabric:/myapp/todo");
            /// 
            ///     // Connect to the cluster.
            ///     FabricClient fabricClient = new FabricClient(clusterConnection);
            /// 
            ///     Console.WriteLine("Deployed Service Package Health:");
            /// 
            ///     try
            ///     {
            ///         // Determine the nodes where the application has been deployed.
            ///         applicationHealth = fabricClient.HealthManager.GetApplicationHealthAsync(applicationName).Result;
            /// 
            ///         // Get the deployed service package health for each node.
            ///         IList&lt;DeployedApplicationHealthState&gt; deployedApplicationHealthStateList = applicationHealth.DeployedApplicationHealthStates;
            ///         foreach (DeployedApplicationHealthState deployedApplicationHealthState in deployedApplicationHealthStateList)
            ///         {
            /// // Get the deployed application health, which contains the service manifest name and 
            /// // the names of the nodes where the service has been deployed.
            /// deployedApplicationHealth = fabricClient.HealthManager.GetDeployedApplicationHealthAsync(applicationName,
            ///     deployedApplicationHealthState.NodeName).Result;
            /// 
            /// // Return if empty.
            /// if (deployedApplicationHealth.DeployedServicePackageHealthStates.Count == 0)
            ///     return false;
            /// 
            /// // Get the deployed service package health.
            /// deployedServicePackageHealth = fabricClient.HealthManager.GetDeployedServicePackageHealthAsync(applicationName,
            ///     deployedApplicationHealth.DeployedServicePackageHealthStates[0].ServiceManifestName, 
            ///     deployedApplicationHealthState.NodeName).Result;
            /// 
            /// // Display the deployed service package health information.
            /// Console.WriteLine("  Application Name: " + deployedServicePackageHealth.ApplicationName);
            /// Console.WriteLine("    Node Name: " + deployedServicePackageHealth.NodeName);
            /// Console.WriteLine("    Aggregated Health State: " + deployedServicePackageHealth.AggregatedHealthState);
            /// Console.WriteLine("    Service Manifest Name: " + deployedServicePackageHealth.ServiceManifestName);
            /// 
            /// // List the health events.
            /// Console.WriteLine("    Health Events:");
            /// IList&lt;HealthEvent&gt; healthEvents = deployedServicePackageHealth.HealthEvents;
            /// foreach (HealthEvent healthEvent in healthEvents)
            /// {
            ///     Console.WriteLine("      Health Event:");
            ///     Console.WriteLine("        Last Modified: " + healthEvent.LastModifiedUtcTimestamp);
            ///     Console.WriteLine("        Source UTC Timestamp: " + healthEvent.SourceUtcTimestamp);
            ///     Console.WriteLine("        Is Expired: " + healthEvent.IsExpired);
            ///     Console.WriteLine("        Health Information:");
            ///     Console.WriteLine("          Description: " + healthEvent.HealthInformation.Description);
            ///     Console.WriteLine("          Source ID: " + healthEvent.HealthInformation.SourceId);
            ///     Console.WriteLine("          Health State: " + healthEvent.HealthInformation.HealthState);
            ///     Console.WriteLine("          Property: " + healthEvent.HealthInformation.Property);
            ///     Console.WriteLine("          Remove When Expired: " + healthEvent.HealthInformation.RemoveWhenExpired);
            ///     Console.WriteLine("          Sequence Number: " + healthEvent.HealthInformation.SequenceNumber);
            ///     Console.WriteLine("          Time to Live: " + healthEvent.HealthInformation.TimeToLive);
            /// }
            /// 
            /// // List the unhealthy evaluations.
            /// Console.WriteLine("    Unhealthy Evaluations:");
            /// IList&lt;HealthEvaluation&gt; healthEvaluationList = deployedServicePackageHealth.UnhealthyEvaluations;
            /// foreach (HealthEvaluation healthEvaluation in healthEvaluationList)
            /// {
            ///     Console.WriteLine("      Kind: " + healthEvaluation.Kind);
            ///     Console.WriteLine("        Aggregated Health State: " + healthEvaluation.AggregatedHealthState);
            ///     Console.WriteLine("        Description: " + healthEvaluation.Description);
            /// }
            /// 
            /// Console.WriteLine();
            ///         }
            ///     }
            ///     catch (Exception e)
            ///     {
            ///         Console.WriteLine("Error: " + e.Message);
            /// 
            ///         if (e.InnerException != null)
            /// Console.WriteLine("  Inner Exception: " + e.InnerException.Message);
            /// 
            ///         return false;
            ///     }
            ///     return true;
            /// }
            /// </code>
            /// </remarks>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(Uri applicationName, string serviceManifestName, string nodeName)
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(applicationName, nodeName, serviceManifestName);
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest file for this Service Fabric service.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The name of the node that the Service Fabric service was deployed to.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate the entity health.</param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(Uri applicationName, string serviceManifestName, string nodeName, ApplicationHealthPolicy healthPolicy)
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(applicationName, nodeName, serviceManifestName) { HealthPolicy = healthPolicy, };
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest file for this Service Fabric service.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The name of the node that the Service Fabric service was deployed to.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(Uri applicationName, string serviceManifestName, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(applicationName, nodeName, serviceManifestName);
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the Service Fabric application.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest file for this Service Fabric service.</para>
            /// </param>
            /// <param name="nodeName">
            /// <para>The name of the node that the Service Fabric service was deployed to.</para>
            /// </param>
            /// <param name="healthPolicy">The application health policy used to evaluate entity health.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the user provided <paramref name="timeout" /> to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="applicationName" /> is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(Uri applicationName, string serviceManifestName, string nodeName, ApplicationHealthPolicy healthPolicy, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new DeployedServicePackageHealthQueryDescription(applicationName, nodeName, serviceManifestName) { HealthPolicy = healthPolicy, };
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" />
            /// object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.OperationTimedOut" /> is returned when the operation takes more than the specified time to complete.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.DeployedServicePackageHealthQueryDescription.ApplicationName"/> 
            ///     is not a valid Service Fabric Name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CommunicationError" /> is returned when a communication error caused the operation to fail.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidCredentialType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509FindType" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreLocation" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509StoreName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Thumbprint" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidProtectionLevel" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidX509Store" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidSubjectName" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidAllowedCommonNameList" />.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription)
            {
                Requires.Argument<DeployedServicePackageHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously gets the health of a deployed Service Fabric service package by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="queryDescription">The query description.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The health of a deployed Service Fabric service package.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <see cref="System.Fabric.Description.DeployedServicePackageHealthQueryDescription.ApplicationName"/> 
            ///     is not a valid Service Fabric Name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsync(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<DeployedServicePackageHealthQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetDeployedServicePackageHealthAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// Gets the health of a Service Fabric cluster.
            /// </summary>
            /// <returns>The health chunk representing the cluster health.</returns>
            /// <remarks>The cluster aggregated health state is computed based on all entities in the cluster.
            /// No children are included in the results, because no filters are specified.</remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealthChunk> GetClusterHealthChunkAsync()
            {
                var queryDescription = new ClusterHealthChunkQueryDescription();
                return this.GetClusterHealthChunkAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the health of a Service Fabric cluster.
            /// </summary>
            /// <param name="timeout">The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException"/>.</param>
            /// <param name="cancellationToken">The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</param>
            /// <returns>The health chunk representing the cluster health.</returns>
            /// <remarks>The cluster aggregated health state is computed based on all entities in the cluster. No children are included in the results, because no filters are specified.</remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealthChunk> GetClusterHealthChunkAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new ClusterHealthChunkQueryDescription();
                return this.GetClusterHealthChunkAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// Gets the health of a Service Fabric cluster, including cluster entities as requested in the query description.
            /// </summary>
            /// <param name="queryDescription">The query description that defines how health evaluation should be performed and what entities should be included in the <see cref="System.Fabric.Health.ClusterHealthChunk"/>.</param>
            /// <returns>The health chunk representing the cluster health.</returns>
            /// <remarks>The cluster aggregated health state is computed based on all entities in the cluster. Only the children that respect the filters from the input query description (if any) are included in the results.</remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealthChunk> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription)
            {
                Requires.Argument<ClusterHealthChunkQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetClusterHealthChunkAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the health of a Service Fabric cluster, including cluster entities as requested in the query description.
            /// </summary>
            /// <param name="queryDescription">The query description that defines how health evaluation should be performed and what entities should be included in the <see cref="System.Fabric.Health.ClusterHealthChunk"/>.</param>
            /// <param name="timeout">The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException"/>.</param>
            /// <param name="cancellationToken">The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</param>
            /// <returns>The health chunk representing the cluster health.</returns>
            /// <remarks>The cluster aggregated health state is computed based on all entities in the cluster. Only the children that respect the filters from the input query description (if any) are included in the results.</remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.FabricHealthEntityNotFound" />.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTooBusy" /> is returned when the service is too busy to process the operation.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ACCESSDENIED is returned when the access check has failed for this operation.</para>
            /// </exception>
            public Task<ClusterHealthChunk> GetClusterHealthChunkAsync(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ClusterHealthChunkQueryDescription>("queryDescription", queryDescription).NotNull();
                return this.GetClusterHealthChunkAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Helpers

            #region Fabric Report Health

            private void ReportHealthHelper(HealthReport healthReport, HealthReportSendOptions sendOptions)
            {
                this.fabricClient.ThrowIfDisposed();
                using (PinCollection pinCollection = new PinCollection())
                {
                    IntPtr nativeHealthReport = healthReport.ToNative(pinCollection);
                    IntPtr nativeSendOptions = IntPtr.Zero;
                    if (sendOptions != null)
                    {
                        nativeSendOptions = sendOptions.ToNative(pinCollection);
                    }

                    this.nativeHealthClient.ReportHealth2(nativeHealthReport, nativeSendOptions);
                }
            }

            #endregion

            #region Fabric GetClusterHealth

            private Task<ClusterHealth> GetClusterHealthAsyncHelper(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<ClusterHealth>(
                    (callback) => this.GetClusterHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetClusterHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetClusterHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterHealthBeginWrapper(ClusterHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetClusterHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ClusterHealth GetClusterHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ClusterHealth.FromNativeResult(this.nativeHealthClient.EndGetClusterHealth2(context));
            }

            #endregion

            #region Fabric GetNodeHealth

            private Task<NodeHealth> GetNodeHealthAsyncHelper(NodeHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<NodeHealth>(
                    (callback) => this.GetNodeHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetNodeHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetNodeHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNodeHealthBeginWrapper(NodeHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetNodeHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NodeHealth GetNodeHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NodeHealth.FromNativeResult(this.nativeHealthClient.EndGetNodeHealth2(context));
            }

            #endregion

            #region Fabric GetApplicationHealth

            private Task<ApplicationHealth> GetApplicationHealthAsyncHelper(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationHealth>(
                    (callback) => this.GetApplicationHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetApplicationHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetApplicationHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationHealthBeginWrapper(ApplicationHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetApplicationHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationHealth GetApplicationHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationHealth.FromNativeResult(this.nativeHealthClient.EndGetApplicationHealth2(context));
            }

            #endregion

            #region Fabric GetServiceHealth

            private Task<ServiceHealth> GetServiceHealthAsyncHelper(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceHealth>(
                    (callback) => this.GetServiceHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetServiceHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetServiceHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceHealthBeginWrapper(ServiceHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetServiceHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceHealth GetServiceHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceHealth.FromNativeResult(this.nativeHealthClient.EndGetServiceHealth2(context));
            }

            #endregion
            
            #region Fabric GetPartitionHealth

            private Task<PartitionHealth> GetPartitionHealthAsyncHelper(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<PartitionHealth>(
                    (callback) => this.GetPartitionHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetPartitionHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetPartitionHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionHealthBeginWrapper(PartitionHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetPartitionHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private PartitionHealth GetPartitionHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return PartitionHealth.FromNativeResult(this.nativeHealthClient.EndGetPartitionHealth2(context));
            }

            #endregion

            #region Fabric GetReplicaHealth

            private Task<ReplicaHealth> GetReplicaHealthAsyncHelper(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<ReplicaHealth>(
                    (callback) => this.GetReplicaHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetReplicaHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetReplicaHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetReplicaHealthBeginWrapper(ReplicaHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetReplicaHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ReplicaHealth GetReplicaHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ReplicaHealth.FromNativeResult(this.nativeHealthClient.EndGetReplicaHealth2(context));
            }

            #endregion
            
            #region Fabric GetDeployedApplicationHealth

            private Task<DeployedApplicationHealth> GetDeployedApplicationHealthAsyncHelper(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedApplicationHealth>(
                    (callback) => this.GetDeployedApplicationHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetDeployedApplicationHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetDeployedApplicationHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedApplicationHealthBeginWrapper(DeployedApplicationHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetDeployedApplicationHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedApplicationHealth GetDeployedApplicationHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedApplicationHealth.FromNativeResult(this.nativeHealthClient.EndGetDeployedApplicationHealth2(context));
            }

            #endregion

            #region Fabric GetDeployedServicePackageHealth

            private Task<DeployedServicePackageHealth> GetDeployedServicePackageHealthAsyncHelper(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedServicePackageHealth>(
                    (callback) => this.GetDeployedServicePackageHealthBeginWrapper(queryDescription, timeout, callback),
                    this.GetDeployedServicePackageHealthEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetDeployedServicePackageHealth");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedServicePackageHealthBeginWrapper(DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetDeployedServicePackageHealth2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedServicePackageHealth GetDeployedServicePackageHealthEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedServicePackageHealth.FromNativeResult(this.nativeHealthClient.EndGetDeployedServicePackageHealth2(context));
            }

            #endregion

            #region Fabric GetClusterHealthChunk

            private Task<ClusterHealthChunk> GetClusterHealthChunkAsyncHelper(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return Utility.WrapNativeAsyncInvokeInMTA<ClusterHealthChunk>(
                    (callback) => this.GetClusterHealthChunkBeginWrapper(queryDescription, timeout, callback),
                    this.GetClusterHealthChunkEndWrapper,
                    InteropExceptionTracePolicy.WarningExceptInfoForTransient,
                    cancellationToken,
                    "HealthClient.GetClusterHealthChunk");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterHealthChunkBeginWrapper(ClusterHealthChunkQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeHealthClient.BeginGetClusterHealthChunk(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ClusterHealthChunk GetClusterHealthChunkEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ClusterHealthChunk.FromNativeResult(this.nativeHealthClient.EndGetClusterHealthChunk(context));
            }

            #endregion

            #endregion
        }
    }
}