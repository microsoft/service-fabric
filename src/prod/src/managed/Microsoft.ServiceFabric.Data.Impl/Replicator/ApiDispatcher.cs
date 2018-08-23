// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading.Tasks;

    /// <summary>
    /// Dispatches APIs to the State Provider.
    /// </summary>
    internal sealed class ApiDispatcher
    {
        public enum FailureAction
        {
            Throw = 0,                  // Just throw
            AbortStateProvider = 1,     // Abort the state provider
        }

        /// <summary>
        /// Trace information.
        /// </summary>
        private readonly string traceType;

        public ApiDispatcher(
            string traceType)
        {
            this.traceType = traceType;
        }

        /// <summary>
        /// Closes a given state provider
        /// </summary>
        public async Task CloseAsync(
            Metadata metadata,
            FailureAction failureAction)
        {
            Utility.Assert(
                metadata != null,
                "{0}: ApiDispatcher::CloseAsync: metadata passed to the function should not be null.",
                this.traceType);

            try
            {
                await metadata.StateProvider.CloseAsync().ConfigureAwait(false);
            }
            catch (Exception)
            {
#if !DotNetCoreClr
                // These are new events defined in System.Fabric, existing CoreCLR apps would break 
                // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
                // along with application
                // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

                FabricEvents.Events.ISP2_ApiError(
                    this.traceType,
                    metadata.StateProviderId,
                    "CloseAsync");
#endif

                // In the native, this part is out of catch block, but if we do this in managed code, the
                // stack trace will be lost, so keep it in the catch block.
                if (failureAction == FailureAction.AbortStateProvider)
                {
                    Abort(metadata);
                }
                else
                {
                    throw;
                }
            }
        }

        /// <summary>
        /// Abort a given state provider
        /// </summary>
        /// <devnote>
        /// Abort state provider should not throw.
        /// </devnote>
        public void Abort(
            Metadata metadata)
        {
            Utility.Assert(
                metadata != null,
                "{0}: ApiDispatcher::Abort: metadata passed to the function should not be null.",
                this.traceType);

            try
            {
                metadata.StateProvider.Abort();
            }
            catch (Exception e)
            {
                // #11361628: Double Check StateProvider Abort should not throw any exceptions, trace it for now, but will change to assert.
                // Abort on a state provider is not expected to fail. if it does, trace, swallow exception and continue.
                FabricEvents.Events.AbortFailed(
                    this.traceType,
                    metadata.StateProviderId,
                    e.Message,
                    e.StackTrace);
            }
        }

        public async Task CloseAsync(
            IEnumerable<Metadata> metadataEnumerator,
            FailureAction failureAction)
        {
            // Note: Since we only support one action, we could have chosen not to pass in a failure action.
            // I have it there to make it explicit and easily readable.
            Utility.Assert(
                failureAction == FailureAction.AbortStateProvider,
                "{0}: ApiDispatcher::CloseAsync: Action only supports AbortStateProvider for CloseAsync. FailureAction: {1}",
                this.traceType,
                failureAction);

            List<Task> taskList = new List<Task>();

            foreach (Metadata metadata in metadataEnumerator)
            {
                // Since the failureAction must be AbortStateProvider, exceptions cannot happen here.
                taskList.Add(CloseAsync(metadata, failureAction));
            }

            // The Task for waiting all CloseAsync finish is exception free, since for every CloseAsync
            // failure, the state provider will call Abort which will never throw exception.
            // We expect the c# scheduler to ensure the optimal number of awaitables execute asynchronously.
            await Task.WhenAll(taskList).ConfigureAwait(false);
        }

        public void Abort(
            IEnumerable<Metadata> metadataEnumerator)
        {
            foreach (Metadata metadata in metadataEnumerator)
            {
                try
                {
                    // Rule: Abort cannot throw.
                    Abort(metadata);
                }
                catch (Exception e)
                {
                    Utility.Assert(
                        false,
                        "{0}: ApiDispatcher::Abort: Exception threw from Abort. SPId: {1}. ExceptionMsg: {2}.",
                        this.traceType,
                        metadata.StateProviderId,
                        e.Message);
                }
            }
        }
    }
}