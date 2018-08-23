// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
    public interface IOperationStream2: IOperationStream
    {
        /// <summary>This supports the Service Fabric infrastructure and is not meant to be used directly from your code.</summary>
        void ReportFault(FaultType faultType);
    }
}