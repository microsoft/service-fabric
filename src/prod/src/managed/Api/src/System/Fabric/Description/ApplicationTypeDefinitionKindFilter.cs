// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the filters used for matching the definition kind of applications type that should be returned by query.</para>
    /// </summary>
    /// <remarks>This enumeration has a <see cref="System.FlagsAttribute"/> that allows a bitwise combination of its members.</remarks>
    [Flags]
    public enum ApplicationTypeDefinitionKindFilter
    {
        /// <summary>
        /// <para>Indicates no filter is added on definition kind.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT,

        /// <summary>
        /// <para>Indicates no filter is added on definition kind.</para>
        /// </summary>
        All = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_ALL,

        /// <summary>
        /// <para>Indicates a filter that matches application types defined and created by a Service Fabric application package provided by the user.</para>
        /// </summary>
        ServiceFabricApplicationPackage = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_SERVICE_FABRIC_APPLICATION_PACKAGE,

        /// <summary>
        /// <para>Indicates a filter that matches application types defined and created implicitly as part of a compose deployment.</para>
        /// </summary>
        Compose = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_COMPOSE,

        /// <summary>
        /// <para>Indicates a filter that matches application types defined and created implicitly as part of a mesh application deployment.</para>
        /// </summary>
        MeshApplicationDescription = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_MESH_APPLICATION_DESCRIPTION
    }
}