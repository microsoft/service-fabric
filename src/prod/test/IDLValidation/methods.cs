// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Setup
{
    public class Methods
    {
        private Dictionary<string, string> infos = new Dictionary<string, string>();

        public Methods()
        {
        }

        public Methods(Dictionary<string, string> infos)
        {
            this.infos = infos;
        }

        /// <summary>
        /// Dictionary which contains method name and method body
        /// </summary>
        public Dictionary<string, string> Infos
        {
            get
            {
                return this.infos;
            }
            set
            {
                this.infos = value;
            }
        }

        public int Count
        {
            get
            {
                return this.GetCount();
            }
        }

        public int GetCount()
        {
            return this.infos.Count;
        }

        /// <summary>
        /// Compare method (use release version as based version)
        /// </summary>
        /// <param name="target">Methods to compare</param>
        /// <returns>Differences contains which methods are added , removed or modified. 
        /// If it is modified, output method body for both version
        /// </returns>
        public string Compare(Methods target)
        {
            if (this.Count == 0 || target == null || target.Count == 0)
            {
                return null;
            }

            StringBuilder sbRemove = new StringBuilder();
            StringBuilder sbModify = new StringBuilder();
            StringBuilder sbAdd = new StringBuilder();
            Dictionary<string, string> targetInfos = target.infos;

            foreach (KeyValuePair<string, string> pair in this.infos)
            {
                string name = pair.Key;

                // If the method is modified, output the methods from both version
                if (!targetInfos.ContainsKey(name))
                {
                    sbRemove.AppendLine(string.Format("Method {0} is removed.", name));
                }
                else
                {
                    if (target.infos[name] != pair.Value)
                    {
                        sbModify.AppendLine(string.Format("Method {0} is modified.", name));
                        sbModify.AppendLine(string.Format("Before: {0}", pair.Value));
                        sbModify.AppendLine(string.Format("Current: {0}", targetInfos[name]));
                    }

                    targetInfos.Remove(name);
                }
            }

            foreach (KeyValuePair<string, string> pair in targetInfos)
            {
                sbAdd.AppendLine(string.Format("Method {0} is added.", pair.Key));
            }

            sbAdd.Append(sbRemove.ToString());
            sbAdd.Append(sbModify.ToString());

            if (sbAdd.Length == 0)
            {
                sbAdd.AppendLine("C++ interfaces are same.");
                sbAdd.AppendLine("C style interfaces are different. The base interface is changed.");
            }

            return sbAdd.ToString();
        }
    }
}