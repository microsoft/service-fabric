// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class Application
    {
        internal class ApplicationProperty
        {
            public ApplicationProperty()
            {
                this.description = null;
                this.debugParams = null;
                this.services = null;
                this.diagnostics = null;
            }

            public string description;
            public string debugParams;
            public List<Service> services;
            public DiagnosticsDescription diagnostics;
        }

        public Application()
        {
            this.name = null;
            this.properties = new ApplicationProperty();
        }

        public string description
        {
            get
            {
                return properties.description;
            }
        }

        public string debugParams
        {
            get
            {
                return properties.debugParams;
            }
        }

        public List<Service> services
        {
            get
            {
                return properties.services;
            }
        }

        public DiagnosticsDescription diagnostics
        {
            get
            {
                return properties.diagnostics;
            }
        }

        public string name;
        public ApplicationProperty properties;
    };
}
