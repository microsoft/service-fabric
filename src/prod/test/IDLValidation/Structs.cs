// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Test;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Setup
{
   public class Structs
    {
       private Dictionary<string, StructObject> infos = new Dictionary<string, StructObject>();     

        public Structs()
        {
        }

        public StructObject this[string name]
        {
            get
            {
                return this.infos.ContainsKey(name) ? this.infos[name] : null;               
            }
            set
            {
                if (infos.ContainsKey(name))
                {
                    this.infos[name] = value;
                }
            }
        }

        public int Count
        {
            get
            {
                return GetCount();
            }
        }

        public Dictionary<string, StructObject> Infos
        {
            get
            {
                return this.infos;
            }
        }

        public bool ContainsName(string name)
        {
            return this.infos.ContainsKey(name);
        }

        public int GetCount()
        {
            return this.infos.Count;
        }

        public void Add(string name, StructObject io)
        {
            this.infos.Add(name, io);
        }

        public List<string> Compare(Structs target)
        {
            if (this.Count == 0 || target == null || target.Count == 0)
            {
                return null;
            }

            List<string> differences = new List<string>();

            // Search structs which defined in previous release from current build
            // If it is found, compare the struct
            // If the content of same struct are differrent 
            // Compare and output the different members
            foreach (KeyValuePair<string, StructObject> pair in this.infos)
            {
                if (ValidationIgnoreList
                      .ValidationIgnoreListStructs
                      .ContainsKey(pair.Key))
                {
                    // Ignore a set of STRUCTS from this validation.
                    continue;
                }

                string name = pair.Key;
                if (target.infos.ContainsKey(name))
                {
                    StructObject cur = target[name];
                    StructObject pre = pair.Value;
                    if (!pre.Equals(cur))
                    {
                        Members curMethods = ValidationHelper.ParseMemberInformation(cur.Body);
                        Members preMethods = ValidationHelper.ParseMemberInformation(pre.Body);

                        string difference = preMethods.Compare(curMethods);

                        LogHelper.Log("Differences of {0}:", name);
                        LogHelper.Log(difference);
                        differences.Add(difference);
                    }
                }
            }

            return differences;
        }
    }
}