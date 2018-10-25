// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    /// <summary>
    /// This is the base abstract class that other validators need to implement.
    /// </summary>
    abstract class BaseFabricConfigurationValidator
    {
        public abstract string SectionName { get; }

        public abstract void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings);

        public abstract void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings);
    }        
}