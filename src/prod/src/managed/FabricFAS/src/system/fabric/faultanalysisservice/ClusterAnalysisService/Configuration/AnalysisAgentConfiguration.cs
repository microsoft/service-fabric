// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Configuration
{
    using System.Collections.Generic;
    using global::ClusterAnalysis.Common.Util;

    /// <summary>
    /// Configuration for a Single Analysis Agent.
    /// </summary>
    /// <remarks>
    /// An agent is a piece of code that runs in response to a trigger signal/event. Once the trigger is seen,
    /// the framework initializes the Agent with its config and Agent code takes over from there.
    /// </remarks>
    public sealed class AnalysisAgentConfiguration
    {
        /// <summary>
        /// Create an instance of <see cref="AnalysisAgentConfiguration"/>
        /// </summary>
        /// <param name="agentName"></param>
        /// <param name="agentContext"></param>
        public AnalysisAgentConfiguration(string agentName, IDictionary<string, string> agentContext)
        {
            Assert.IsNotEmptyOrNull(agentName, "Agent Name can't be null");
            Assert.IsNotNull(agentContext, "Agent Config cannot be null");

            this.AgentName = agentName;
            this.AgentContext = agentContext;
        }

        /// <summary>
        /// Gets or sets the name of the Agent.
        /// </summary>
        public string AgentName { get; set; }

        /// <summary>
        /// Gets or sets the Agent Configuration.
        /// </summary>
        /// <remarks>
        /// The configuration is in form of Name-Value pair. This is an itentional design
        /// to ensure that new Agents can be easily added or config updated without changes to
        /// API surface. This is required since it's expected that new agents will be added
        /// at a fast pace. Documentation will be published with list of agents and supported 
        /// configuration knobs for each Agent.
        /// </remarks>
        public IDictionary<string, string> AgentContext { get; set; }
    }
}