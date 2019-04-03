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
    using System.Text.RegularExpressions;

    public class StructObject
    {
        private string name = string.Empty;
        private string body = string.Empty;
        private readonly string[] newLine = { "\r\n"};
        private const string space = " ";

        public StructObject()
        {
        }

        public StructObject(string name, string body)
        {
            this.name = name;
            this.body = body;
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

        public string Body
        {
            get
            {
                return this.body;
            }
            set
            {
                this.body = value;
            }
        }
                
        public override bool Equals(object obj)
        {
            StructObject target = (StructObject)obj;

            if (target == null || this.name != target.name)
            {
                return false;
            }

            if (this.body == target.body)
            {
                return true;
            }

            //
            // Each line in the body belongs to a struct member.
            // The type and the variable name, should match exactly,
            // IDL compiler can sometimes add/remove space between the type
            // and the member name. This is to handle that case.
            //
            Console.WriteLine("Member's dont exactly match for {0} against the previous versions, doing additional checks", this.name);
            var thisMembers = this.body.Split(this.newLine, StringSplitOptions.RemoveEmptyEntries);
            var targetMembers = target.body.Split(this.newLine, StringSplitOptions.RemoveEmptyEntries);

            if (thisMembers.Count() != targetMembers.Count())
            {
                return false;
            }

            for (int i = 0; i < thisMembers.Count(); ++i)
            {
                if (Regex.Replace(thisMembers[i], @" +", "") != Regex.Replace(targetMembers[i], @" +", ""))
                {
                    return false;
                }
            }

            return true;
        }

        public override int GetHashCode()
        {
            return this.name.GetHashCode() ^ this.body.GetHashCode();
        }
    }
}