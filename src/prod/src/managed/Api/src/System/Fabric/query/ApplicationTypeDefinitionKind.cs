// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the application type definition kind.</para>
    /// <para>Specifies mechanism user used to define a Service Fabric application type.</para>
    /// </summary>
    public enum ApplicationTypeDefinitionKind
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_INVALID,

        /// <summary>
        /// <para>Indicates that the application type is defined and created by a Service Fabric application package provided by the user.</para>
        /// </summary>
        ServiceFabricApplicationPackage = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_PACKAGE,

        /// <summary>
        /// <para>Indicates that the application type is defined and created implicitly as part of a compose deployment.</para>
        /// </summary>
        Compose = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_COMPOSE,

        /// <summary>
        /// <para>Indicates that the application type is defined and created implicitly as part of a Mesh application deployment.</para>
        /// </summary>
        MeshApplicationDescription = NativeTypes.FABRIC_APPLICATION_TYPE_DEFINITION_KIND.FABRIC_APPLICATION_TYPE_DEFINITION_KIND_MESH_APPLICATION_DESCRIPTION
    }
}