// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Text;

    /// <summary>
    /// <para>Provides an application parameter override to be applied when creating or upgrading an application.</para>
    /// </summary>
    public sealed class ApplicationParameter
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationParameter" /> class.</para>
        /// </summary>
        public ApplicationParameter()
        {
        }

        /// <summary>
        /// <para>Gets the name of the application parameter to override.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application parameter to override.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Key)]
        public string Name { get; set; }

        /// <summary>
        /// <para>Gets the value of the application parameter to override.</para>
        /// </summary>
        /// <value>
        /// <para>The value of the application parameter to override.</para>
        /// </value>
        public string Value { get; set; }

        /// <summary>
        /// Returns a string representation of the <see cref="System.Fabric.Description.ApplicationParameter" /> class.
        /// </summary>
        /// <returns>
        /// Returns a string that displays an application parameter with format "name" = "value" followed by a newline.
        /// </returns>
        public override string ToString()
        {
            var returnString = new StringBuilder();

            returnString.AppendFormat(Globalization.CultureInfo.InvariantCulture, "\"{0}\" = \"{1}\"", this.Name, this.Value);
            returnString.AppendLine();

            return returnString.ToString();
        }

        internal void ToNative(PinCollection pinCollection, out NativeTypes.FABRIC_APPLICATION_PARAMETER nativeParameter)
        {
            nativeParameter.Name = pinCollection.AddObject(this.Name);
            nativeParameter.Value = pinCollection.AddBlittable(this.Value); // allow empty strings.
            nativeParameter.Reserved = IntPtr.Zero;
        }

        internal static ApplicationParameter CreateFromNative(NativeTypes.FABRIC_APPLICATION_PARAMETER nativeParameter)
        {
            return new ApplicationParameter()
            {
                Name = NativeTypes.FromNativeString(nativeParameter.Name),
                Value = NativeTypes.FromNativeString(nativeParameter.Value)
            };
        }
    }
}