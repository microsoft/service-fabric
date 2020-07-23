// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Globalization;

    /// <summary>
    /// A class for mocking out <see cref="FabricInfrastructureServiceAgent"/> for unit-testability.
    /// </summary>
    internal class MockInfrastructureAgentWrapper : IInfrastructureAgentWrapper
    {
        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="RegisterInfrastructureServiceFactory"/>.
        /// </summary>
        public Action<IStatefulServiceFactory> RegisterInfrastructureServiceFactoryAction { get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="RegisterInfrastructureServiceFactory"/>.
        /// </summary>
        public Func<Guid, long, IInfrastructureService, string> RegisterInfrastructureServiceFunc { get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="UnregisterInfrastructureService"/>.
        /// </summary>
        public Action<Guid, long> UnregisterInfrastructureServiceAction{ get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="StartInfrastructureTaskAsync"/>.
        /// </summary>
        public Action<InfrastructureTaskDescription, TimeSpan, CancellationToken> StartInfrastructureTaskAsyncAction { get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="FinishInfrastructureTaskAsync"/>.
        /// </summary>
        public Action<string, long, TimeSpan, CancellationToken> FinishInfrastructureTaskAsyncAction { get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="QueryInfrastructureTaskAsync"/>.
        /// </summary>
        public Func<TimeSpan, CancellationToken, InfrastructureTaskQueryResult> QueryInfrastructureTaskAsyncFunc { get; set; }

        public void RegisterInfrastructureServiceFactory(IStatefulServiceFactory factory)
        {
            if (RegisterInfrastructureServiceFactoryAction != null)
            {
                RegisterInfrastructureServiceFactoryAction(factory);
            }
        }

        public string RegisterInfrastructureService(Guid partitionId, long replicaId, IInfrastructureService service)
        {
            if (RegisterInfrastructureServiceFunc != null)
            {
                return RegisterInfrastructureServiceFunc(partitionId, replicaId, service);
            }
            
            string registrationId = string.Format(CultureInfo.InvariantCulture, "P{0}_R{1}", partitionId, replicaId);
            return registrationId;
        }

        public void UnregisterInfrastructureService(Guid partitionId, long replicaId)
        {
            if (UnregisterInfrastructureServiceAction != null)
            {
                UnregisterInfrastructureServiceAction(partitionId, replicaId);
            }
        }

        public Task StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(
                () =>
                {
                    if (StartInfrastructureTaskAsyncAction != null)
                    {
                        StartInfrastructureTaskAsyncAction(description, timeout, cancellationToken);
                    }
                    else
                    {
                        DefaultStartInfrastructureTask(description, timeout, cancellationToken);
                    }
                }, 
                cancellationToken);
        }

        public Task FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(
                () =>
                {
                    if (FinishInfrastructureTaskAsyncAction != null)
                    {
                        FinishInfrastructureTaskAsyncAction(taskId, instanceId, timeout, cancellationToken);
                    }
                    else
                    {
                        DefaultFinishInfrastructureTask(taskId, instanceId, timeout, cancellationToken);
                    }
                },
                cancellationToken);
        }

        public Task<InfrastructureTaskQueryResult> QueryInfrastructureTaskAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(
                () =>
                {
                    if (QueryInfrastructureTaskAsyncFunc != null)
                    {
                        return QueryInfrastructureTaskAsyncFunc(timeout, cancellationToken);
                    }

                    // TODO returning null is not a valid option. However, it is currently not possible to wrap
                    // InfrastructureTaskQueryResult or create a mock object since its constructor is private.
                    // So, we return null and handle null in the production code.
                    return null;
                },
                cancellationToken);            
        }

        private static void DefaultStartInfrastructureTask(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
        {            
        }

        private static void DefaultFinishInfrastructureTask(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
        }
    }
}