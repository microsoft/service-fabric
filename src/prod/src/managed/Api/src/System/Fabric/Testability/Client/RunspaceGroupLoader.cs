// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    using System;
    using System.Fabric.Testability.ClientSwitch;
    using System.Management.Automation.Runspaces;
    using System.Threading.Tasks;

    public class RunspaceGroupLoader
    {
        public static void PreloadPowershellRunspaces()
        {
            // Assuming there are 5 runspaces in group and preloading those to avoid wait times later in the test
            // This is a workaround for now and we need to look into a better option or somehow get rid of powershell from 
            // these tests and have a different powershell test
            Task.Run(() => PreloadPowershellRunspace());
            Task.Run(() => PreloadPowershellRunspace());
            Task.Run(() => PreloadPowershellRunspace());
            Task.Run(() => PreloadPowershellRunspace());
            Task.Run(() => PreloadPowershellRunspace());
        }
        
        private static void PreloadPowershellRunspace()
        {
            RunspaceGroup group = RunspaceGroup.Instance;
            using (RunspaceGroup.Token runspaceToken = group.Take())
            {
            }
        }
    }
}