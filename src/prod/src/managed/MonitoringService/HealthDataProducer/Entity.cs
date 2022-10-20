// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Monitoring.Filters;
    using Microsoft.ServiceFabric.Monitoring.Health;
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    /// <summary>
    /// Abstraction for the entity about which we query information from the HM. 
    /// </summary>
    /// <typeparam name="TEntityHealth">The concrete type that derives from System.Fabric.Health.EntityHealth.</typeparam>
    internal abstract class Entity<TEntityHealth> : IEntity where TEntityHealth : EntityHealth
    {
        protected Entity(
            FabricHealthClientWrapper healthClient,
            HealthDataConsumer consumer, 
            TraceWriterWrapper traceWriter, 
            IServiceConfiguration config,
            EntityFilterRepository filters)
        {
            this.HealthClient = Guard.IsNotNull(healthClient, nameof(healthClient));
            this.Consumer = Guard.IsNotNull(consumer, nameof(consumer));
            this.TraceWriter = Guard.IsNotNull(traceWriter, nameof(traceWriter));
            this.Config = Guard.IsNotNull(config, nameof(config));
            this.Filters = Guard.IsNotNull(filters, nameof(filters));
        }

        /// <summary>
        /// Gets the concrete value that derives from EntityHealth.
        /// </summary>
        public TEntityHealth Health { get; protected set; }

        protected FabricHealthClientWrapper HealthClient { get; private set; }

        protected HealthDataConsumer Consumer { get; private set; }

        protected TraceWriterWrapper TraceWriter { get; private set; }

        protected IServiceConfiguration Config { get; private set; }

        protected EntityFilterRepository Filters { get; private set; }

        /// <summary>
        /// This method implements the core logic of processing health data from an entity.
        /// It 1. Reads health data from entity, 2. Invokes the consumer to process the data and 3. returns the child entities if any.
        /// Steps 1, 2 and 3 are specific to each entity and are implemented by the concrete classes that derive from this class.
        /// </summary>
        /// <param name="cancellationToken">CancellationToken instance.</param>
        /// <returns>Sequence of child entities.</returns>
        async Task<IEnumerable<IEntity>> IEntity.ProcessAsync(CancellationToken cancellationToken)
        {
            this.Health = await this.GetHealthAsync(this.Config.HealthQueryTimeoutInSeconds, cancellationToken).ConfigureAwait(false);

            cancellationToken.ThrowIfCancellationRequested();

            if (this.Health == null)
            {
                this.TraceWriter.WriteInfo("Entity.ProcessAsync: HealthClient.GetHealthAsync returned a null EntityHealth object.");
                return Enumerable.Empty<IEntity>();
            }

            await this.ProcessHealthAsync().ConfigureAwait(false);

            return this.GetChildren(this.Health);
        }

        public bool IsHealthEventReportingEnabled(string appName, System.Fabric.Health.HealthState aggregatedHealthState)
        {
            var healthEventsFilter = this.Filters.HealthEventsFilter;
            return healthEventsFilter.IsEntityEnabled(appName)
                && healthEventsFilter.IsEntityEnabled(aggregatedHealthState);
        }

        /// <summary>
        /// Reads the health data from the entity.
        /// </summary>
        /// <param name="timeout">Health query timeout.</param>
        /// <param name="cancellationToken">CancellationToken instance.</param>
        /// <returns>The entity health.</returns>
        protected abstract Task<TEntityHealth> GetHealthAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Processes the health data.
        /// </summary>
        /// <returns>Task instance.</returns>
        protected abstract Task ProcessHealthAsync();

        /// <summary>
        /// Returns the child entities if any. 
        /// Returns empty sequence if this entity does not have children.
        /// </summary>
        /// <param name="health">EntityHealth value.</param>
        /// <returns>Sequence if entities.</returns>
        protected virtual IEnumerable<IEntity> GetChildren(TEntityHealth health)
        {
            return Enumerable.Empty<IEntity>();
        }
    }
}