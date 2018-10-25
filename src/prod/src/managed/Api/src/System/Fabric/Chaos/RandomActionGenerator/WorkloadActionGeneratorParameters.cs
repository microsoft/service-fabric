// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;

    [Serializable]
    class WorkloadActionGeneratorParameters
    {
        public WorkloadActionGeneratorParameters()
        {
            this.WorkloadScripts = new List<string>();
            this.WorkloadScriptsWeights = new List<double>();
        }

        // Comma seperated workload names. Example: <WorkloadScripts = workloadName1,workloadName2>.
        // For each workload, say workloadName1, there must be two files with exactly named: <workloadName1.start> and <workloadName1.stop>.
        public List<string> WorkloadScripts { get; set; }

        public List<double> WorkloadScriptsWeights { get; set; }

        public bool ValidateParameters(out string errorMessage)
        {
            errorMessage = string.Empty;
            return true;
        }
    }
}