// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Common
{
    // Signal to retry the same step that threw, instead of rolling back.  THis is an internal exception that only exists in FAS and 
    // does not make it's way out to the user.    
    internal class RetrySameStepException : Exception
    {
        public RetrySameStepException(string message, Exception inner)
            : base(message, inner)
        {
            this.HResult = inner.HResult;
        }
    }
}