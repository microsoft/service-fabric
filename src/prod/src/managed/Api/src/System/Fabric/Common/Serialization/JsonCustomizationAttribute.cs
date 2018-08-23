// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Serialization
{
    [AttributeUsage(AttributeTargets.All, Inherited = true, AllowMultiple = false)]
    sealed internal class JsonCustomizationAttribute : Attribute
    {
        private int? appearanceOrder;

        public JsonCustomizationAttribute()
        {
        }

        /// Defines the name of current property in JsonString.
        public string PropertyName { get; set; }

        /// Defines the name of current property in JsonString.
        public bool IsIgnored { get; set; }

        /// Defines the name of current property in JsonString.
        public string JsonConverterTypeName { get; set; }

        /// Defines the name of current property in JsonString.
        public bool ReCreateMember { get; set; }

        /// Defines order of the this property in json string relative to other properties of the defining class.
        public int AppearanceOrder 
        {
            get { return this.appearanceOrder ?? -1; }
            set { this.appearanceOrder = value; }
        }

        public int? GetAppearanceOrder()
        {
            return this.AppearanceOrder;
        }

        public bool IsDefaultValueIgnored { get; set; }

        public bool ConvertAsJsonArray { get; set; }
    }

}