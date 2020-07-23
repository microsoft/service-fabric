// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using System;
    using System.Collections.Generic;
    using System.Web.Http.Dependencies;

    internal class ControllerDependencyResolver<T> : IDependencyResolver
    {
        private readonly ControllerSetting controllerSettings;

        public ControllerDependencyResolver(ControllerSetting controllerSettings)
        {
            this.controllerSettings = controllerSettings;
        }

        public IDependencyScope BeginScope()
        {
            return new ControllerDependencyResolver<T>(this.controllerSettings);
        }

        public object GetService(Type serviceType)
        {
            if (serviceType == null)
            {
                return null;
            }

            if (typeof(T).IsAssignableFrom(serviceType))
            {
                return Activator.CreateInstance(serviceType, this.controllerSettings);
            }
            else
            {
                return null;
            }
        }

        public IEnumerable<object> GetServices(Type serviceType)
        {
            return new List<object>();
        }

        public void Dispose()
        {
        }
    }
}