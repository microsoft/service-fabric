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
    public class Members
    {
        private Dictionary<string, string> infos = new Dictionary<string, string>();

        public Members()
        {
        }

        public Members(Dictionary<string, string> infos)
        {
            this.infos = infos;
        }

        /// <summary>
        /// Dictionary which contains member name and member body
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
        /// Compare members (use release version as based version)
        /// </summary>
        /// <param name="target">Members to compare</param>
        /// <returns>Differences contains which members are added , removed or modified. 
        /// If it is modified, output member body for both version
        /// </returns>
        public string Compare(Members target)
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

                // If the member is modified, output the members from both version
                if (!targetInfos.ContainsKey(name))
                {
                    sbRemove.AppendLine(string.Format("Member {0} is removed.", name));
                }
                else
                {
                    if (target.infos[name] != pair.Value)
                    {
                        sbModify.AppendLine(string.Format("Member {0} is modified.", name));
                        sbModify.AppendLine(string.Format("Before: {0}", pair.Value));
                        sbModify.AppendLine(string.Format("Current: {0}", targetInfos[name]));
                    }

                    targetInfos.Remove(name);
                }
            }

            foreach (KeyValuePair<string, string> pair in targetInfos)
            {
                sbAdd.AppendLine(string.Format("Member {0} is added.", pair.Key));
            }

            sbAdd.Append(sbRemove.ToString());
            sbAdd.Append(sbModify.ToString());

            return sbAdd.ToString();
        }
    }
}