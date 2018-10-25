// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the health state.</para>
    /// </summary>
    public enum HealthState
    {
        /// <summary>
        /// <para>Indicates that the health state is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_HEALTH_STATE.FABRIC_HEALTH_STATE_INVALID,
        
        /// <summary>
        /// <para>Indicates that the health state is ok.</para>
        /// </summary>
        Ok = NativeTypes.FABRIC_HEALTH_STATE.FABRIC_HEALTH_STATE_OK,
        
        /// <summary>
        /// <para>Indicates that the health state is warning. There may something wrong that requires investigation.</para>
        /// </summary>
        Warning = NativeTypes.FABRIC_HEALTH_STATE.FABRIC_HEALTH_STATE_WARNING,
        
        /// <summary>
        /// <para>Indicates that the health state is error, there is something wrong that needs to be investigated.</para>
        /// </summary>
        Error = NativeTypes.FABRIC_HEALTH_STATE.FABRIC_HEALTH_STATE_ERROR,
        
        /// <summary>
        /// <para>Indicates that the health state is unknown.</para>
        /// </summary>
        Unknown = NativeTypes.FABRIC_HEALTH_STATE.FABRIC_HEALTH_STATE_UNKNOWN
    }
}