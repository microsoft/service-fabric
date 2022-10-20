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
    public class InterfaceObject
    {
        private Guid id;
        private string name;
        private string cplusplusContent;
        private string cContent;

        public InterfaceObject ()
        {
        }

        public InterfaceObject(Guid id)
        {
            this.id = id;
        }

        public InterfaceObject(Guid id, string name)
        {
            this.id = id;
            this.name = name;
        }

        public Guid ID
        {
            get
            {
                return this.id;
            }
            set
            {
                this.id = value;
            }
        }

        public string Name
        {
            get
            {
                return this.name;
            }
            set
            {
                this.name = value;
            }
        }

        /// <summary>
        /// C++ interface body
        /// </summary>
        public string CplusplusContent
        {
            get
            {
                return this.cplusplusContent;
            }
            set
            {
                this.cplusplusContent = value;
            }
        }

        /// <summary>
        /// C style interface body
        /// </summary>
        public string CContent
        {
            get
            {
                return this.cContent;
            }
            set
            {
                this.cContent = value;
            }
        }


        /// <summary>
        /// Only compare C++ interface body and C interface body, not Guid
        /// Since we only output the differences of methods
        /// </summary>
        /// <returns>The target object equals to current one</returns>
        public override bool Equals(object obj)
        {
            InterfaceObject target = (InterfaceObject)obj;
            return target != null &&               
                this.cplusplusContent == target.cplusplusContent &&
                this.cContent == target.cContent;
        }

        public override int GetHashCode()
        {
            return this.id.GetHashCode() ^ this.cplusplusContent.GetHashCode() ^ this.cContent.GetHashCode();
        }
    }
}