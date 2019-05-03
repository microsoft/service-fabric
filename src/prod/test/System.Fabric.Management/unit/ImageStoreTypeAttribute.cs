// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    enum ImageStoreEnum
    {
        File,
        XStore
    }

    [AttributeUsage(AttributeTargets.Method)]
    class ImageStoreTypeAttribute : Attribute
    {
        public ImageStoreEnum ImageStoreEnum { get; private set; }

        public ImageStoreTypeAttribute(ImageStoreEnum imageStoreEnum)
        {
            this.ImageStoreEnum = imageStoreEnum;
        }   
    }
}