// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates filters for parameters of type <see cref="System.Fabric.Health.HealthState" />. 
    /// This enumeration has a <see cref="System.FlagsAttribute" /> 
    /// that allows a bitwise combination of its member values.</para>
    /// </summary>
    [Flags]
    public enum HealthStateFilter : long
    {
        /// <summary>
        /// <para>Default value. Depending on usage, may match any <see cref="System.Fabric.Health.HealthState" /> or none.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_DEFAULT,
        
        /// <summary>
        /// <para>Filter that doesn’t match any <see cref="System.Fabric.Health.HealthStateFilter" />. 
        /// Used in order to return no results on a given collection of states.</para>
        /// </summary>
        None = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_NONE,
        
        /// <summary>
        /// <para>Filter that matches input with value <see cref="System.Fabric.Health.HealthState.Ok" />.</para>
        /// </summary>
        Ok = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_OK,
        
        /// <summary>
        /// <para>Filter that matches input with value <see cref="System.Fabric.Health.HealthState.Warning" />.</para>
        /// </summary>
        Warning = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_WARNING,
        
        /// <summary>
        /// <para>Filter that matches input with value <see cref="System.Fabric.Health.HealthState.Error" />.</para>
        /// </summary>
        Error = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_ERROR,
        
        /// <summary>
        /// <para>Filter that matches any <see cref="System.Fabric.Health.HealthState" />.</para>
        /// </summary>
        All = NativeTypes.FABRIC_HEALTH_STATE_FILTER.FABRIC_HEALTH_STATE_FILTER_ALL,
    }
}
