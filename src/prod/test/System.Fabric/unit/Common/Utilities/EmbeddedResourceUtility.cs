// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Collections.Specialized;
    using System.IO;
    using System.Reflection;

    public static class EmbeddedResourceUtility
    {
        public static string ReadAllText(string resourceName)
        {
            resourceName = EmbeddedResourceUtility.FormatResourceName(resourceName);

            using (StreamReader sr = new StreamReader(Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName)))
            {
                return sr.ReadToEnd();
            }
        }

        public static byte[] ReadAllBytes(string resourceName)
        {
            resourceName = EmbeddedResourceUtility.FormatResourceName(resourceName);

            using (BinaryReader br = new BinaryReader(Assembly.GetExecutingAssembly().GetManifestResourceStream(resourceName)))
            {
                return br.ReadBytes((int)br.BaseStream.Length);
            }
        }

        public static string ReadTemplateAndApplyVariables(string resourceName, NameValueCollection values)
        {
            string contents = EmbeddedResourceUtility.ReadAllText(resourceName);

            return EmbeddedResourceUtility.ApplyVariables(contents, values);
        }

        public static string ApplyVariables(string contents, NameValueCollection values)
        {
            foreach (var item in values.AllKeys)
            {
                string replaceText;
                if (item.StartsWith("[[") && item.EndsWith("]]"))
                {
                    replaceText = item;
                }
                else
                {
                    replaceText = "[[" + item + "]]";
                }

                contents = contents.Replace(replaceText, values[item]);
            }

            return contents;
        }

        private static string FormatResourceName(string resourceName)
        {
            const string resourceNamePrefix = "System.Fabric.Test.Templates.";
            if (!resourceName.StartsWith(resourceNamePrefix))
            {
                resourceName = resourceNamePrefix + resourceName;
            }

            return resourceName;
        }
    }
}