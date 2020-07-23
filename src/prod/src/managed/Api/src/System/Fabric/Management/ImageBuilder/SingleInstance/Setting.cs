// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class Setting
    {
        public static string DefaultSectionName = "Fabric_Reserved";

        public Setting()
        {
            this.name = null;
            this.value = null;
        }
        
        public string name
        {
            get
            {
                if (string.IsNullOrEmpty(SectionName))
                {
                    return ParameterName;
                }
                else
                {
                    return string.Format("{0}/{1}", SectionName, ParameterName);
                }
            }
            set
            {
                SectionName = DefaultSectionName;
                ParameterName = value;

                if (value != null)
                {
                    int index = value.IndexOf('/');
                    if (index >= 0)
                    {
                        SectionName = value.Substring(0, index);
                        if (index < value.Length - 1)
                        {
                            ParameterName = value.Substring(index + 1);
                        }
                    }
                }
            }
        }
        public string value;

        // Not for serialization
        public string SectionName;
        public string ParameterName;
    }
}
