// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Fabric.Query;

    static class StateInfoExtensionMethods
    {
        // ActiveApplicationsInfo
        public static int GetPartitionCount(this ApplicationEntityList applications, Uri serviceName)
        {
            IEnumerable < ServiceEntity > services = applications.GetAllServiceInstances();
            foreach(var svc in services)
            {
                if(svc.ServiceName() == serviceName)
                {
                    return svc.PartitionList.Count();
                }
            }

            return 0;
        }

        // Returns non-null List of ServiceInstanceInfo objects.. empty list if null input.
        // No throw
        public static ReadOnlyCollection<ServiceEntity> GetAllServiceInstances(this ApplicationEntityList applications)
        {
            var services = new List<ServiceEntity>();

            if (applications != null)
            {
                foreach (var app in applications)
                {
                    services.AddRange(app.ServiceList);
                }
            }

            return services.AsReadOnly();
        }

        // No throw
        // Returns non-null.
        public static ReadOnlyCollection<Uri> GetAllServiceNames(this ApplicationEntityList applications)
        {
            // As applications.GetAllServiceInstances() returns non-null results, it should always return non-null.
            return applications.GetAllServiceInstances().Select(svc => svc.Service.ServiceName).ToList().AsReadOnly();
        }

        // No throw
        // Returns non-null.
        public static Uri ServiceName(this ServiceEntity svcInfo)
        {
            return DefaultOnNullException<Uri>(
                () => 
                    svcInfo.Service.ServiceName);
        }

        // No throw
        // Returns non-null.
        public static Uri ApplicationName(this ApplicationEntity appInfo)
        {
            return DefaultOnNullException<Uri>(
                () => 
                    appInfo.Application.ApplicationName);
        }

        public static bool IsStateful(this ServiceKind serviceKind)
        {
            return serviceKind == ServiceKind.Stateful;
        }

        public static T DefaultOnNullException<T>(Func<T> funcToRun)
        {
            T result = default(T);

            try
            {
                result = funcToRun();
            }
            catch (NullReferenceException)
            {
            }

            return result;
        }
    }
}