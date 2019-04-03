// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;
    using System.Web.Http.Dependencies;
    using Microsoft.Practices.Unity;

    [TestClass]
    public class UnityResolverTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ResolveTest()
        {
            FilePathResolver filePathResolver = new FilePathResolver("hiahia", "biabia");
            TraceLogger logger = new TraceLogger(new MockUpTraceEventProvider());

            using (UnityContainer container = new UnityContainer())
            {
                container.RegisterInstance<IFilePathResolver>(filePathResolver, new ContainerControlledLifetimeManager());
                container.RegisterInstance<TraceLogger>(logger, new ContainerControlledLifetimeManager());
                using (UnityResolver resolver = new UnityResolver(container))
                {
                    for (int i = 0; i < 3; i++)
                    {
                        using (IDependencyScope dependencyScope = resolver.BeginScope())
                        {
                            object resolvedFilePathResolver = dependencyScope.GetService(typeof(IFilePathResolver));
                            Assert.AreSame(filePathResolver, resolvedFilePathResolver);

                            object resolvedLogger = dependencyScope.GetService(typeof(TraceLogger));
                            Assert.AreSame(logger, resolvedLogger);
                        }
                    }
                }
            }
        }
    }
}