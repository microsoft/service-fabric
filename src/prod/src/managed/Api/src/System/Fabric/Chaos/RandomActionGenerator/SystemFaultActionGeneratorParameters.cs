// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;

    /// <summary>
    /// This class adds test parameters which are relevant to specific part of RandomSession -- SystemFaultActionsManager.
    /// This class exposed configurations/parameters relevant for generating random actions(add, remove, restart) for nodes in the cluster.
    /// </summary>
    [Serializable]
    internal class SystemFaultActionGeneratorParameters
    {
        /// <summary>
        /// Default weight for FmRebuild system fault action
        /// </summary>
        public const double FmRebuildFaultWeightDefault = 100;

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.Chaos.RandomActionGenerator.SystemFaultActionGeneratorParameters"/> class.
        /// </summary>
        public SystemFaultActionGeneratorParameters()
        {
            this.FmRebuildFaultWeight = FmRebuildFaultWeightDefault;
        }

        /// <summary>
        /// Gets or sets the weight to determine the probability with which 
        /// the action generator will create FmRebuildAction.
        /// </summary>
        public double FmRebuildFaultWeight { get; set; }

        /// <summary>
        /// This method will do a basic validation of the testParameters.
        /// </summary>
        /// <param name="errorMessage">Help message/hint if some parameters are not valid.</param>
        /// <returns>True only if all parameters in this class are valid.</returns>
        public bool ValidateParameters(out string errorMessage)
        {
            errorMessage = string.Empty;
            return true;
        }
    }
}