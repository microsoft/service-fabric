// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// This is an enum used to indicate when the API should complete. 
    /// </summary>
    /// <remarks>
    /// The values indicate whether the API should complete when the request for the operation is done or when the requested operation has completed. 
    /// For example,  a request to restart a node  could complete as soon the request is accepted or when the API can verify that the node has restarted. 
    /// The actual verification depends upon the API being used.
    /// </remarks>
    [Serializable]
    public enum CompletionMode
    {
        /// <summary>
        /// Completion mode does not have a valid value.
        /// </summary>
        Invalid = NativeTypes.FABRIC_COMPLETION_MODE.FABRIC_COMPLETION_MODE_INVALID,
        /// <summary>
        /// Do not verify the completion of the action.
        /// </summary>
        DoNotVerify,
        /// <summary>
        /// Verify the completion of the action.
        /// </summary>
        Verify
    }
}