// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class Label
    {
        public Label()
        {
            Name = null;
            Value = null;
        }

        public string Name;
        public string Value;

        public void Validate(string codePackageName)
        {
            if (string.IsNullOrEmpty(this.Name))
            {
                throw new FabricApplicationException(String.Format("required parameter 'Name' not specified for label in code package {0}", codePackageName));
            }

            if (string.IsNullOrEmpty(this.Value))
            {
                throw new FabricApplicationException(String.Format("required parameter 'Value' not specified for label {0} in code package {1}", this.Name, codePackageName));
            }
        }
    };
}
