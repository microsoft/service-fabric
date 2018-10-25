// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using IO;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;

    /// <summary>
    /// This is a helper class for selecting partitions. 
    /// </summary>
    /// <remarks>
    /// It allows the user to select partitions to be targeted by the testability APIs. The selection can be a particular partition of a service based on the 
    /// Id or Key or a random partition of a service.
    /// </remarks>
    [Serializable]
    public class PartitionSelector
    {
        private const string PartitionKindSingleton = "PartitionKindSingleton";
        private const string PartitionKindUniformInt64 = "PartitionKindUniformInt64";
        private const string PartitionKindNamed = "PartitionKindNamed";
        private const string PartitionId = "PartitionId";
        private const string PartitionKey = "PartitionKey";
        private const string PartitionKind = "PartitionKind";
        private const string ServiceNameParameter = "ServiceName";

        private readonly Uri serviceName;
        private readonly PartitionSelectorType selectorType;
        private readonly string partitionKey;

        private PartitionSelector(Uri serviceName, PartitionSelectorType selectorType, string partitionKey)
        {
            Requires.ThrowIfNull(serviceName, "serviceName");

            this.serviceName = serviceName;
            this.selectorType = selectorType;
            this.partitionKey = partitionKey;
            this.PowershellParameters = new Dictionary<string, string>();
            this.PowershellParameters.Add(ServiceNameParameter, serviceName.OriginalString);
            switch (this.selectorType)
            {
                case PartitionSelectorType.Singleton:
                    this.PowershellParameters.Add(PartitionKind, PartitionKindSingleton);
                    break;

                case PartitionSelectorType.Named:
                    this.PowershellParameters.Add(PartitionKind, PartitionKindNamed);
                    this.PowershellParameters.Add(PartitionKey, this.partitionKey);
                    break;

                case PartitionSelectorType.UniformInt64:
                    this.PowershellParameters.Add(PartitionKind, PartitionKindUniformInt64);
                    this.PowershellParameters.Add(PartitionKey, this.partitionKey);
                    break;

                case PartitionSelectorType.PartitionId:
                    this.PowershellParameters.Add(PartitionId, this.partitionKey);
                    break;

                case PartitionSelectorType.Random:
                    break;

                default:
                    this.ThrowInvalidPartitionSelector();
                    break;
            }
        }

        [Serializable]
        private enum PartitionSelectorType
        {
            None = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_NONE,
            Singleton = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON,
            Named = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_NAMED,
            UniformInt64 = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64,
            PartitionId = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID,
            Random = NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_RANDOM
        }

        internal Uri ServiceName
        {
            get
            {
                return this.serviceName;
            }
        }

        internal Dictionary<string, string> PowershellParameters
        {
            get;
            private set;
        }

        /// <summary>
        /// Selects a random partition for given service.
        /// </summary>
        /// <param name="serviceName">Name of the service whose partition needs to be selected.</param>
        /// <returns>Constructed PartitionSelector.</returns>
        public static PartitionSelector RandomOf(Uri serviceName)
        {
            return new PartitionSelector(serviceName, PartitionSelectorType.Random, string.Empty);
        }

        /// <summary>
        /// Selects the singleton partition for a service.
        /// </summary>
        /// <param name="serviceName">Name of the service whose partition needs to be selected.</param>
        /// <returns>Constructed PartitionSelector.</returns>
        public static PartitionSelector SingletonOf(Uri serviceName)
        {
            return new PartitionSelector(serviceName, PartitionSelectorType.Singleton, string.Empty);
        }

        /// <summary>
        /// Selects a partition for the service with the specified PartitionName.
        /// </summary>
        /// <param name="serviceName">Name of the service whose partition needs to be selected.</param>
        /// <param name="partitionName">Name of the partition that needs to be selected.</param>
        /// <returns>Constructed PartitionSelector.</returns>
        public static PartitionSelector PartitionKeyOf(Uri serviceName, string partitionName)
        {
            //ThrowIf.NullOrEmpty(partitionName, "partitionName");
            Requires.ThrowIfNullOrWhiteSpace(partitionName, "partitionName");
            return new PartitionSelector(serviceName, PartitionSelectorType.Named, partitionName);
        }

        /// <summary>
        /// Selects a partition for the service to which the specified partition key belongs.
        /// </summary>
        /// <param name="serviceName">Name of the service whose partition needs to be selected.</param>
        /// <param name="partitionKey">The partition key which belongs to the partition to be selected.</param>
        /// <returns>Constructed PartitionSelector.</returns>
        public static PartitionSelector PartitionKeyOf(Uri serviceName, long partitionKey)
        {
            return new PartitionSelector(serviceName, PartitionSelectorType.UniformInt64, partitionKey.ToString());
        }

        /// <summary>
        /// Selects a partition for the service given the PartitionId.
        /// </summary>
        /// <param name="serviceName">Name of the service whose partition needs to be selected.</param>
        /// <param name="partitionId">The PartitionId for the partition.</param>
        /// <returns>Constructed PartitionSelector.</returns>
        public static PartitionSelector PartitionIdOf(Uri serviceName, Guid partitionId)
        {
            return new PartitionSelector(serviceName, PartitionSelectorType.PartitionId, partitionId.ToString());
        }

        internal void Write(BinaryWriter bw)
        {
            bw.Write(this.ServiceName.OriginalString);
            bw.Write((int)this.selectorType);
            bw.Write(this.partitionKey);
        }

        internal static PartitionSelector Read(BinaryReader br)
        {

            string s = br.ReadString();

            Uri serviceName = new Uri(s);
            PartitionSelectorType selectorType = (PartitionSelectorType)br.ReadInt32();
            string partitionKey = br.ReadString();

            return new PartitionSelector(serviceName, selectorType, partitionKey);
        }

        internal int GetPartitionSelectorType()
        {
            int type = Convert.ToInt32(this.selectorType);
            return type;
        }

        internal string GetPartitionKey()
        {
            return this.partitionKey;
        }

        /// <summary>
        /// Compares whether two PartitionSelectors are the same.
        /// </summary>
        /// <param name="obj">PartitionSelector to compare t.o</param>
        /// <returns>true if same false if not.</returns>
        public override bool Equals(object obj)
        {
            PartitionSelector partitionObj = obj as PartitionSelector;
            if (partitionObj == null)
            {
                return false;
            }

            if (this.serviceName != partitionObj.serviceName)
            {
                return false;
            }

            if (this.selectorType != partitionObj.selectorType)
            {
                return false;
            }

            if (this.partitionKey != partitionObj.partitionKey)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Calls the base GetHashCode()
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        /// <summary>
        /// String representation of the partition selector.
        /// </summary>
        /// <returns>A string representation of the selector.</returns>
        public override string ToString()
        {
            return string.Format("Service Name {0} with {1} {2}", this.serviceName, this.selectorType, this.partitionKey).Trim();
        }

        internal Partition GetSelectedPartition(Partition[] results, Random random)
        {
            if (results.Length == 0)
            {
                throw new FabricException(FabricErrorCode.PartitionNotFound);
            }

            Partition result = null;
            switch (this.selectorType)
            {
                case PartitionSelectorType.Singleton:
                    if ((results[0].PartitionInformation.Kind != ServicePartitionKind.Singleton) || (results.Length != 1))
                    {
                        this.ThrowInvalidPartitionSelector();
                    }

                    result = results[0];
                    break;

                case PartitionSelectorType.Named:
                    if (results[0].PartitionInformation.Kind != ServicePartitionKind.Named)
                    {
                        throw new FabricInvalidPartitionSelectorException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidPartitionSelector, this.selectorType));
                    }

                    if (string.IsNullOrEmpty(this.partitionKey))
                    {
                        this.ThrowInvalidPartitionKey();
                    }

                    foreach (var partition in results)
                    {
                        if ((partition.PartitionInformation as NamedPartitionInformation).Name.Equals(this.partitionKey))
                        {
                            result = partition;
                            break;
                        }
                    }

                    break;

                case PartitionSelectorType.UniformInt64:
                    if (results[0].PartitionInformation.Kind != ServicePartitionKind.Int64Range)
                    {
                        this.ThrowInvalidPartitionSelector();
                    }

                    if (string.IsNullOrEmpty(this.partitionKey))
                    {
                        this.ThrowInvalidPartitionKey();
                    }

                    long partitionKeyValue = 0;
                    if (!long.TryParse(this.partitionKey, out partitionKeyValue))
                    {
                        this.ThrowInvalidPartitionKey();
                    }

                    foreach (var qpartition in results)
                    {
                        var partitionInfo = (qpartition.PartitionInformation as Int64RangePartitionInformation);
                        if (partitionKeyValue >= partitionInfo.LowKey && partitionKeyValue <= partitionInfo.HighKey)
                        {
                            result = qpartition;
                            break;
                        }
                    }

                    break;

                case PartitionSelectorType.PartitionId:
                    if (string.IsNullOrEmpty(this.partitionKey))
                    {
                        this.ThrowInvalidPartitionKey();
                    }

                    result = results.Where(p => p.PartitionInformation.Id == new Guid(this.partitionKey)).First();
                    break;

                case PartitionSelectorType.Random:
                    int index = random.Next(results.Length);
                    result = results[index];
                    break;

                default:
                    break;
            }

            if (result == null)
            {
                this.ThrowInvalidPartitionKey();
            }

            return result;
        }

        // This method is used when getting ParititionId. If PartitionSelector does nto specify the PartitionId
        // directly then the service name is needed to query teh PartitionId. Basically either or needs to be set.
        // This method though a long name returns either PartitionId or ServiceName
        internal bool TryGetPartitionIdIfNotGetServiceName(out Guid partitionId, out Uri serviceName)
        {
            partitionId = Guid.Empty;

            if (this.serviceName == null)
            {
                throw new FabricServiceNotFoundException();
            }

            serviceName = this.serviceName;

            if (this.selectorType == PartitionSelectorType.PartitionId)
            {
                if (string.IsNullOrEmpty(this.partitionKey))
                {
                    ThrowInvalidPartitionKey();
                }

                partitionId = new Guid(this.partitionKey);
                return true;
            }

            return false;
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativePartitionSelector = new NativeTypes.FABRIC_PARTITION_SELECTOR();

            nativePartitionSelector.ServiceName = pin.AddObject(this.ServiceName);

            switch (this.selectorType)
            {
                case PartitionSelectorType.Singleton:
                    nativePartitionSelector.PartitionSelectorType =
                        NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON;
                    break;
                case PartitionSelectorType.Named:
                    nativePartitionSelector.PartitionSelectorType =
                        NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_NAMED;
                    break;
                case PartitionSelectorType.UniformInt64:
                    nativePartitionSelector.PartitionSelectorType =
                        NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64;
                    break;
                case PartitionSelectorType.PartitionId:
                    nativePartitionSelector.PartitionSelectorType =
                        NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID;
                    break;
                case PartitionSelectorType.Random:
                    nativePartitionSelector.PartitionSelectorType =
                        NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_RANDOM;
                    break;
                default:
                    this.ThrowInvalidPartitionSelector();
                    break;
            }

            nativePartitionSelector.PartitionKey = pin.AddObject(this.partitionKey);

            return pin.AddBlittable(nativePartitionSelector);
        }

        internal static unsafe PartitionSelector CreateFromNative(NativeTypes.FABRIC_PARTITION_SELECTOR nativePartitionSelector)
        {
            Uri serviceName = NativeTypes.FromNativeUri(nativePartitionSelector.ServiceName);

            PartitionSelectorType partitionSelectorType;
            switch (nativePartitionSelector.PartitionSelectorType)
            {
                case NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_SINGLETON:
                    partitionSelectorType = PartitionSelectorType.Singleton;
                    break;
                case NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_NAMED:
                    partitionSelectorType = PartitionSelectorType.Named;
                    break;
                case NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_PARTITION_ID:
                    partitionSelectorType = PartitionSelectorType.PartitionId;
                    break;
                case NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_UNIFORM_INT64:
                    partitionSelectorType = PartitionSelectorType.UniformInt64;
                    break;
                case NativeTypes.FABRIC_PARTITION_SELECTOR_TYPE.FABRIC_PARTITION_SELECTOR_TYPE_RANDOM:
                    partitionSelectorType = PartitionSelectorType.Random;
                    break;
                default:
                    AppTrace.TraceSource.WriteError("PartitionSelector.CreateFromNative", "Unknown partition selector type: {0}", nativePartitionSelector.PartitionSelectorType);
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, "Error_Unknown_PartitionSelectorType", nativePartitionSelector.PartitionSelectorType));
            }

            string partitionKey = NativeTypes.FromNativeString(nativePartitionSelector.PartitionKey);

            return new PartitionSelector(serviceName, partitionSelectorType, partitionKey);
        }
        
        internal void ThrowInvalidPartitionSelector()
        {        
            throw new FabricInvalidPartitionSelectorException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidPartitionSelector, this.selectorType), FabricErrorCode.InvalidPartitionSelector);
        }
        
        private void ThrowInvalidPartitionKey()
        {        
            throw new FabricInvalidPartitionKeyException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_InvalidPartitionKey, this.partitionKey, this.selectorType), FabricErrorCode.InvalidPartitionKey);
        }
    }
}