// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the application definition kind.</para>
    /// <para>Specifies the mechanism the user used to define a Service Fabric application.</para>
    /// </summary>
    public enum ApplicationDefinitionKind
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_APPLICATION_DEFINITION_KIND.FABRIC_APPLICATION_DEFINITION_KIND_INVALID,

        /// <summary>
        /// <para>Indicates that the application is defined by <see cref="System.Fabric.Description.ApplicationDescription"/>.</para>
        /// </summary>
        ServiceFabricApplicationDescription = NativeTypes.FABRIC_APPLICATION_DEFINITION_KIND.FABRIC_APPLICATION_DEFINITION_KIND_SERVICE_FABRIC_APPLICATION_DESCRIPTION,

        /// <summary>
        /// <para>Indicates that the application is defined by compose file(s).</para>
        /// </summary>
        Compose = NativeTypes.FABRIC_APPLICATION_DEFINITION_KIND.FABRIC_APPLICATION_DEFINITION_KIND_COMPOSE,

        /// <summary>
        /// <para>Indicates that the application is defined by Mesh application description.</para>
        /// </summary>
        MeshApplicationDescription = NativeTypes.FABRIC_APPLICATION_DEFINITION_KIND.FABRIC_APPLICATION_DEFINITION_KIND_MESH_APPLICATION_DESCRIPTION
    }
}