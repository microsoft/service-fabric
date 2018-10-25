// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the node status.</para>
    /// </summary>
    public enum NodeStatus
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_INVALID,
        
        /// <summary>
        /// <para>Node is up.</para>
        /// </summary>
        Up = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_UP,
        
        /// <summary>
        /// <para>Node is down.</para>
        /// </summary>
        Down = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_DOWN,
        
        /// <summary>
        /// <para>Node is being enabled.</para>
        /// </summary>
        Enabling = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_ENABLING,
        
        /// <summary>
        /// <para>Node is being disabled.</para>
        /// </summary>
        Disabling = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_DISABLING,

        /// <summary>
        /// <para>Node is disabled.</para>
        /// </summary>
        Disabled = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_DISABLED,

        /// <summary>
        /// Node status is not known.
        /// </summary>
        Unknown   = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_UNKNOWN,

        /// <summary>
        /// Node is removed.
        /// </summary>
        Removed   = NativeTypes.FABRIC_QUERY_NODE_STATUS.FABRIC_QUERY_NODE_STATUS_REMOVED
    }

    /// <summary>
    /// Enumerates the filters used for matching the node status for nodes that should be returned by query.
    /// </summary>
    /// <remarks>This enumeration has a <see cref="System.FlagsAttribute"/> that allows a bitwise combination of its members.</remarks>
    [Flags]
    public enum NodeStatusFilter
    {
        /// <summary>
        /// Returns all nodes other than unknown and removed.
        /// </summary>
        Default = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT,

        /// <summary>
        /// Returns all nodes.
        /// </summary>
        All = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_ALL,

        /// <summary>
        /// Returns all up nodes.
        /// </summary>
        Up = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_UP,

        /// <summary>
        /// Returns all down nodes.
        /// </summary>
        Down = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_DOWN,

        /// <summary>
        /// Returns all nodes that are currently enabling.
        /// </summary>
        Enabling = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_ENABLING,

        /// <summary>
        /// Returns all nodes that are currently disabling.
        /// </summary>
        Disabling = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_DISABLING,

        /// <summary>
        /// Returns all disabled nodes.
        /// </summary>
        Disabled = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_DISABLED,

        /// <summary>
        /// Returns all nodes that are in Unknown state.
        /// </summary>
        Unknown = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_UNKNOWN,

        /// <summary>
        /// Returns all removed nodes.
        /// </summary>
        Removed = NativeTypes.FABRIC_QUERY_NODE_STATUS_FILTER.FABRIC_QUERY_NODE_STATUS_FILTER_REMOVED,
    }
}