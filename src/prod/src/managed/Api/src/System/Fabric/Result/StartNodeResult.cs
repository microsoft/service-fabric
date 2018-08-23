// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Numerics;

    /// <summary>
    /// Returns Start node result object.
    /// </summary>
    /// <remarks>
    /// This class returns the NodeResult for Start Node action.
    /// </remarks>
    [Serializable]
    public class StartNodeResult : NodeResult
    {
        /// <summary>
        /// Start node result constructor.
        /// </summary>
        /// <param name="nodeName">Node name</param>
        /// <param name="nodeInstance">node instance id</param>
        internal StartNodeResult(string nodeName, BigInteger nodeInstance)
            : base(nodeName, nodeInstance)
        {
        }
    }
}