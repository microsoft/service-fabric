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
    public class Interfaces
    {
        private Dictionary<string, InterfaceObject> infos = new Dictionary<string, InterfaceObject>();
        private Dictionary<Guid, List<string>> sameGuidInterfaces = new Dictionary<Guid, List<string>>();
     
        public Interfaces()
        {
        }        

        public InterfaceObject this[string name]
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

        public Dictionary<string, InterfaceObject> Infos
        {
            get
            {
                return this.infos;
            }
        }

        public Dictionary<Guid, List<string>> SameGuidInterfaces
        {
            get
            {
                return this.sameGuidInterfaces;
            }
            set
            {
                this.sameGuidInterfaces = value;
            }
        }

        public void Add(string name, InterfaceObject io)
        {
            this.infos.Add(name, io);
        }

        public void Remove(string name)
        {
            if (infos.ContainsKey(name))
            {
                this.infos.Remove(name);
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

        /// <summary>
        /// Compare interfaces which defined in previous release with current build
        /// If it is found, compare the interface
        /// If the content of same interface are differrent 
        /// Compare and output the different methods
        /// </summary>
        /// <param name="target">Interfaces Object</param>
        /// <returns>Differences of interfaces</returns>
        public List<string> Compare(Interfaces target)
        {
            if (this.Count == 0 || target == null || target.Count == 0)
            {
                return null;
            }           

            List<string> differences = new List<string>();

            // Search interfaces which defined in previous release from current build
            // If it is found, compare the interface
            // If the content of same interface are differrent 
            // Compare and output the different methods
            foreach (KeyValuePair<string, InterfaceObject> pair in this.infos)
            {
                string name = pair.Key;
                if (target.infos.ContainsKey(name))
                {
                    InterfaceObject cur = target[name];
                    InterfaceObject pre = pair.Value;
                    if (!pre.Equals(cur))
                    {
                        Methods curMethods = ValidationHelper.ParseMethodInformation(cur.CplusplusContent);
                        Methods preMethods = ValidationHelper.ParseMethodInformation(pre.CplusplusContent);

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