// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Text;

    internal class ApplicationEntityList : IReadOnlyCollection<ApplicationEntity>
    {
        private IList<ApplicationEntity> list;
        private ClusterStateSnapshot clusterStateSnapshot;

        public ApplicationEntityList(ClusterStateSnapshot clusterStateSnapshot)
        {
            this.list = new List<ApplicationEntity>();
            this.clusterStateSnapshot = clusterStateSnapshot;
        }

        public ApplicationEntity AddApplication(Application application)
        {
            ApplicationEntity applicationEntity = new ApplicationEntity(application, this.clusterStateSnapshot);
            this.list.Add(applicationEntity);
            return applicationEntity;
        }

        public ApplicationEntity this[int index]
        {
            get
            {
                return this.list[index];
            }
        }

        public int Count
        {
            get { return this.list.Count; }
        }

        public IEnumerator<ApplicationEntity> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            foreach (var app in this.list)
            {
                sb.AppendLine(app.ToString());
            }

            return sb.ToString();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }
    }
}