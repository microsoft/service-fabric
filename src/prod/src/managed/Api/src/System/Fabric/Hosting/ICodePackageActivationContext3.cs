// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;

    /// <summary>
    /// Represents activation context for the Service Fabric activated service.
    /// </summary>
    /// <remarks>Includes information from the service manifest as well as information
    /// about the currently activated code package like work directory, context id etc.</remarks>
    public interface ICodePackageActivationContext3 : ICodePackageActivationContext2
    {
        /// <summary>
        /// Retrieves the path to sub directory inside the work directory with the name "logicalDirectoryName".
        /// </summary>
        /// <returns>The path to sub directory with name logicalDirectoryName inside work directory</returns>
        /// <remarks>Throws an exception if logicalDirectoryName is not found under work directory.
        /// Otherwise will return directory path WorkDirectory\logicalDirectoryName which is a symbolic link to 
        /// directory specified with Name under LogicalDirectories Section of ClusterManifest.
        /// for example: if you have specified <LogicalDirectory LogicalDirectoryName="Foo" MappedTo="D:\\Foo" /> and call to this method
        /// with directoryName "Foo" will return you path WorkDirectory\Foo. Any writes made to returned path will go to D:\Foo\NodeId\ApplicationType_ApplicationVersion.
        /// This method is case insensitive so even if you say directoryName foo it will still return you directory path WorkDirectory\Foo.</remarks>
        string GetDirectory(string logicalDirectoryName);
    }
}