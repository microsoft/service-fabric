// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Fabric.Test;
using System.IO;

namespace System.Fabric.Setup
{
    internal class SectionSetting
    {
        private string section = string.Empty;
        private string setting = string.Empty;

        // Section name of config
        public string Section
        {
            get
            {
                return this.section;
            }
            set
            {
                value = this.section;
            }
        }

        // Setting Name of config
        public string Setting
        {
            get
            {
                return this.setting;
            }
            set
            {
                value = this.setting;
            }
        }

        public SectionSetting()
        {
        }

        public SectionSetting(string section, string setting)
        {
            this.section = section;
            this.setting = setting;
        }

        public override bool Equals(object obj)
        {
            SectionSetting target = (SectionSetting)obj;
            return target != null && this.section == target.section && this.setting == target.setting;
        }

        public override int GetHashCode()
        {
            return this.section.GetHashCode() ^ this.setting.GetHashCode();
        }

    }
}