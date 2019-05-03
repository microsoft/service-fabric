// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Serialization
{
    using Newtonsoft.Json.Linq;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    internal static class SerializationUtil
    {
        static readonly double EventHealthEvaluationProb = 0.40;
        static readonly Dictionary<Type, Func<Random, object>> customCreators = new Dictionary<Type, Func<Random, object>>();

        static SerializationUtil()
        {
            customCreators.Add(typeof(ServicePlacementPolicyDescription), CreateRandomServicePlacementPolicyDescription);
            customCreators.Add(typeof(ServiceTypeDescription), CreateRandomServiceTypeDescription);
            customCreators.Add(typeof(HealthEvaluation), CreateRandomHealthEvaluation);
            customCreators.Add(typeof(ReplicaHealth), CreateRandomHealth);
            customCreators.Add(typeof(ServicePartitionInformation), CreateRandomServicePartitionInformation);
            customCreators.Add(typeof(NodeId), CreateRandomNodeId);
            customCreators.Add(typeof(SafetyCheck), CreateRandomSafetyCheck);
        }

        public static T CreateRandom<T>(this Random random)
        {
            return (T)CreateRandom(random, typeof(T));
        }

        public static object CreateRandom(this Random random, Type type)
        {
            if (customCreators.ContainsKey(type))
            {
                return customCreators[type](random);
            }

            object value;
            if (type.IsValueType || type == typeof(string) || type == typeof(Uri) || type.IsEnum)
            {
                value = CreateRandomValue(random, type);
            }
            else
            {
                value = CreateRandomInstance(random, type);
            }

            return value;
        }

        /// For struct types and string, Uri
        public static object CreateRandomValue(this Random random, Type targetType)
        {
            dynamic value;
            int randomInt = random.Next();

            if (targetType == typeof(byte)
                || targetType.IsAssignableFrom(typeof(byte)))
            {
                value = (byte)(randomInt % 100); // Some places byte is used for percentage value
            }
            else if (targetType == typeof(int)
                || targetType == typeof(uint)
                || targetType.IsAssignableFrom(typeof(int)))
            {
                value = (int)randomInt;
            }
            else if (targetType == typeof(long)
                || targetType.IsAssignableFrom(typeof(long)))
            {
                value = (long)randomInt;
            }
            else if (targetType == typeof(double)
                || targetType == typeof(float)
                || targetType.IsAssignableFrom(typeof(double)))
            {
                // Native serializer rounds double to 5 significant digits.
                value = (float)random.Next(10000);
            }
            else if (targetType.IsAssignableFrom(typeof(System.Numerics.BigInteger)))
            {
                value = new System.Numerics.BigInteger((long)randomInt * (long)random.Next());
            }
            else if (targetType.IsAssignableFrom(typeof(Guid)))
            {
                value = Guid.NewGuid();
            }
            else if (targetType.IsAssignableFrom(typeof(DateTime)))
            {
                var dateTime = new DateTime(DateTime.Now.Ticks, DateTimeKind.Utc);
                dateTime.AddTicks(randomInt);
                value = dateTime;
            }
            else if (targetType.IsAssignableFrom(typeof(TimeSpan)))
            {
                value = TimeSpan.FromMilliseconds(random.Next());
            }
            else if (targetType.IsAssignableFrom(typeof(string)))
            {
                value = "testStr_" + randomInt;
            }
            else if(targetType.IsAssignableFrom(typeof(Uri)))
            {
                value = new Uri("fabric:/" + "testUri_" + randomInt);
            }
            else if (targetType.IsAssignableFrom(typeof(bool)))
            {
                value = random.Next() % 2 == 0;
            }
            else if (targetType.IsAssignableFrom(typeof(HealthEvaluationKind)))
            {
                // Generates 'HealthEvaluationKind.Event' with probability 'EventHealthEvaluationProb'
                bool createEvent = random.NextDouble() < EventHealthEvaluationProb;
                if(createEvent)
                {
                    value = HealthEvaluationKind.Event;
                }
                else
                {
                    var minEvalKind = HealthEvaluationKind.Event;
                    var maxEvalKind = HealthEvaluationKind.UpgradeDomainDeltaNodesCheck;
                    var randomKind = minEvalKind + random.Next((int)maxEvalKind);
                    value = randomKind;
                }
            }
            else if (targetType.IsAssignableFrom(typeof(HealthState)))
            { 
                /// All : 0,1,2,3 65555
                /// Valid values : 1, 2 ,3
                value = 1 + random.Next(3);
            }
            else if (targetType.IsEnum)
            {
                Array enumValues = targetType.GetEnumValues();
                int randIndex = 0;
                if (enumValues.Length > 1)
                {
                    // skiping the first one as generally that is invalid.
                    randIndex = 1 + random.Next(enumValues.Length - 1);
                }

                value = enumValues.GetValue(randIndex);
            }
            else if(targetType.IsGenericType && targetType.GetGenericTypeDefinition() == typeof(KeyValuePair<,>))
            {
                var t1 = targetType.GetGenericArguments()[0];
                var t2 = targetType.GetGenericArguments()[1];
                dynamic key = CreateRandom(random, t1);
                dynamic val = CreateRandom(random, t2);
                value = Activator.CreateInstance(targetType, key, val);
            }
            else
            {
                value = Activator.CreateInstance(targetType, true);
                PopulateObject(random, value);
            }

            return value;
        }

        // targetType:type needs to have default constructor
        public static object CreateRandomInstance(this Random random, Type targetType)
        {
            var result = CreateInstance(random, targetType);

            PopulateObject(random, result);
            return result;
        }

        public static object CreateInstance(this Random random, Type targetType)
        {
            if (targetType.IsAssignableFrom(typeof(ReplicaHealth)))
            {
                var toss = random.Next() % 2 == 0;
                targetType = toss ? typeof(StatefulServiceReplicaHealth) : typeof(StatelessServiceInstanceHealth);
            }
            
            if (targetType.IsAssignableFrom(typeof(ReplicaHealthState)))
            {
                var toss = random.Next() % 2 == 0;
                targetType = toss ? typeof(StatefulServiceReplicaHealthState) : typeof(StatelessServiceInstanceHealthState);
            }

            if (targetType.IsAssignableFrom(typeof(System.Fabric.Query.Partition)))
            {
                var toss = random.Next() % 2 == 0;
                targetType = toss ? typeof(System.Fabric.Query.StatelessServicePartition) : typeof(System.Fabric.Query.StatefulServicePartition);
            }

            if (targetType.IsAssignableFrom(typeof(ServicePartitionInformation)))
            {
                int toss = random.Next() % 3;
                if (toss == 0)
                {
                    targetType = typeof(SingletonPartitionInformation);
                }
                else if (toss == 1)
                {
                    targetType = typeof(Int64RangePartitionInformation);
                }
                else
                {
                    targetType = typeof(NamedPartitionInformation);
                }
            }

            if (targetType.IsAssignableFrom(typeof(SafetyCheck)))
            {
                int toss = random.Next() % 3;
                if (toss == 0)
                {
                    targetType = typeof(PartitionSafetyCheck);
                }
                else if (toss == 1)
                {
                    targetType = typeof(SeedNodeSafetyCheck);
                }
                else
                {
                    targetType = typeof(UnknownSafetyCheck);
                }
            }

            // Abstract
            if (targetType.IsAbstract)
            {
                var implType = GetImplementationType(random, targetType);
                if (implType != null && targetType.IsAssignableFrom(implType))
                {
                    return CreateInstance(random, implType);
                }            

                return null;
            }

            dynamic result;
            try
            {
                // Try using parameterless constructor
                result = Activator.CreateInstance(targetType, true);
                return result;
            }
            catch (Exception)
            {
            }

            try
            {
                // Create uninitialized object.
                result = FormatterServices.GetUninitializedObject(targetType);
                return result;
            }
            catch (Exception)
            {
                return null;
            }
        }

        public static void PopulateObject(this Random random, object obj)
        {
            if( obj == null)
            {
                return;
            }

            Type type = obj.GetType();
            // if IList<T> return
            if (DoesImplementICollection(type) != null)
            {
                PopulateICollection(random, (obj as IEnumerable), 1);
                return;
            }
            else
            {
                // complex object handle each property
                foreach (var p in type.GetProperties(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public))
                {
                    // Skip if this property is Obselete
                    if (p.GetCustomAttribute<ObsoleteAttribute>() != null)
                    {
                        continue;
                    }

                    SetRandomPropertyValue(random, obj, p);
                }
            }
        }

        // If the property has already been instantiated then don't re-create instance.
        public static void SetRandomPropertyValue(this Random random, object obj, PropertyInfo pi)
        {
            var type = pi.PropertyType;
            if (type.IsValueType || type == typeof(string) || type == typeof(Uri) || type.IsEnum || pi.GetValue(obj) == null || type == typeof(System.Numerics.BigInteger))
            {
                // Skip if this property is ReadOnly
                if (!pi.CanWrite)
                {
                    return;
                }

                pi.SetValue(obj, CreateRandom(random, pi.PropertyType));
                return;
            } 

            // property is class type and has already been instantiated.
            PopulateObject(random, pi.GetValue(obj));
        }

        public static int PopulateICollection(this Random random, IEnumerable icollection, int n)
        {
            if (icollection != null && icollection.GetType().GetInterface("System.Collections.Generic.ICollection`1") != null)
            {
                return PopulateICollectionGeneric(random, (dynamic)icollection, n);
            }
            else
            {
                // Don't know what kind of objects are stored. Do nothing.
                return 0;
            }
        }

        public static int PopulateICollectionGeneric<T>(this Random random, ICollection<T> list, int n)
        {
            if(list.IsReadOnly)
            {
                return 0;
            }

            for (int i = 0; i < n; i++)
            {
                T item = random.CreateRandom<T>();
                if(item == null || item.Equals(default(T)))
                {
                    return i;
                }

                list.Add(item);
            }

            return n;
        }

        /// Create instance of a derived class using known type attribute.
        private static Type GetRandomKnownImplementationType(this Random random, Type baseType)
        {
            Type[] knownImplTypes =
                System.Attribute.GetCustomAttributes(baseType)   // Select all custom attribute
                .Where(ca => ca is KnownTypeAttribute)           // filter which are KnownTypeAttribute
                .Select(kta => (kta as KnownTypeAttribute).Type) // return type of Attribute as type of KnownTypeAttribute
                .ToArray();

            if (knownImplTypes == null || knownImplTypes.Length == 0)
            {
                return null;
            }

            int randomIndex = random.Next(knownImplTypes.Length);
            Type targetType = knownImplTypes[randomIndex];
            return targetType;
        }

        // Returns typeof(List<>) or typeof(Collection<>)
        public static Type DoesImplementICollection(Type type)
        {
            var ienumerable = type.GetInterfaces().FirstOrDefault(t => t == typeof(ICollection));
            if(ienumerable != null)
            {
                return type;
            }

            // Find IList<>
            Type genericTypeImpl = GetImplementingGenericType(type, typeof(IList<>));

            if (genericTypeImpl != null)
            {
                // implements IList<>
                var genericType = typeof(List<>);
                return genericType.MakeGenericType(genericTypeImpl.GetGenericArguments()[0]);
            }

            // Find ICollection<> if needed
            genericTypeImpl = GetImplementingGenericType(type, typeof(ICollection<>));
            if (genericTypeImpl != null)
            {
                // implements ICollection<>
                var genericType = typeof(Collection<>);
                return genericType.MakeGenericType(genericTypeImpl.GetGenericArguments()[0]);
            }

            return null;
        }

        public static Type GetImplementingGenericType(Type type, Type genericType)
        {
            Type genericTypeImpl;
            if (type.IsGenericType && type.GetGenericTypeDefinition() == genericType)
            {
                genericTypeImpl = type;
            }
            else
            {
                genericTypeImpl = type.GetInterfaces().FirstOrDefault(
                    x =>
                        x.IsGenericType &&
                        x.GetGenericTypeDefinition() == genericType);
            }

            return genericTypeImpl;
        }

        public static Boolean ObjectMatches(object x, object y, string path = "DefaultRoot")
        {
            bool? decision = ReferenceMatchOrTypeMismatch(x, y, path);
            if (decision != null)
            {
                Assert.IsTrue(decision.Value, "ObjectMatches(): Compare Type and Reference. path: " + path);
                return decision.Value;
            }

            // Enumerable
            if (x is IEnumerable)
            {
                return EnumerableMatches((IEnumerable)x, (IEnumerable)y, path);
            }

            // Complex object
            foreach (PropertyInfo propInfo in x.GetType().GetProperties(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance))
            {
                if (!propInfo.CanRead)
                {
                    /// can't compare this property if it is not readlable.
                    continue;
                }

                string pathCurrent = path + "." + propInfo.Name;
                Type propType = propInfo.PropertyType;
                var vx = propInfo.GetValue(x);
                var vy = propInfo.GetValue(y);

                if (propType == typeof(float) || propType == typeof(double) || propType == typeof(TimeSpan))
                {
                    double dx;
                    double dy;

                    if (propType == typeof(TimeSpan))
                    {
                        dx = ((TimeSpan)vx).TotalMilliseconds;
                        dy = ((TimeSpan)vy).TotalMilliseconds;
                    }
                    else
                    {
                        dx = (double)vx;
                        dy = (double)vy;
                    }

                    bool eq = dx == dy;

                    if (!eq)
                    {
                        // Test if dx is within +/-1% of dy.
                        eq =   dx <= dy * 1.01 // + 1%
                            && dx >= dy * 0.99; // - 1%
                    }

                    Assert.IsTrue(eq, string.Format(
                        "ObjectMatches(): Assert property {0} matches. dx: {1} == dy: {2} path: {3} ", x.GetType(), dx, dy, pathCurrent));
                }
                else if (propType.IsValueType || propType == typeof(Uri) || propType.IsAssignableFrom(typeof(string)))
                {
                    string s1 = vx == null ? null : vx.ToString();
                    string s2 = vy == null ? null : vy.ToString();

                    // Print path where mismatch happened.
                    Assert.AreEqual<string>(s1, s2, "ObjectMatches(): Assert property matches. path: " + pathCurrent);
                }
                else if (vx is IEnumerable)
                {
                    Assert.IsTrue(EnumerableMatches((IEnumerable)vx, (IEnumerable)vy, pathCurrent), "ObjectMatches(): Assert property EnumerableMatches. path: " + pathCurrent);
                } 
                else
                {
                    /// Recurse if not value type.
                    Assert.IsTrue(ObjectMatches(vx, vy, pathCurrent), "ObjectMatches(): Assert property matches. path: " + pathCurrent);
                }
            }

            return true;
        }

        public static bool EnumerableMatches(IEnumerable x, IEnumerable y, string path = "DefaultRoot")
        {
            // If both are null or empty return true
            if (IsEnumerableNullOrEmpty((IEnumerable)x) && IsEnumerableNullOrEmpty((IEnumerable)y))
            {
                // Matches if both enumerables are empty or null.
                return true;
            }

            if (x == null || y == null)
            {
                Assert.Fail("EnumerableMatches(): Assert exactly one is null other is not.: " + path);
            }

            bool elemMatch = true;
            bool countMatch = true;

            int i = 0;
            string pathCurrent = path + "[ " + i + " ]";
            var yitr = y.GetEnumerator();
            foreach (var xi in x)
            {
                if (yitr.MoveNext())
                {
                    var yi = yitr.Current;
                    elemMatch = ObjectMatches(xi, yi, pathCurrent);
                }
                else
                {
                    countMatch = false;
                    Assert.IsTrue(countMatch, "EnumerableMatches(): Assert count matches. path : " + pathCurrent);
                }

                if (!elemMatch || !countMatch)
                {
                    Assert.IsTrue(elemMatch && countMatch, "EnumerableMatches(): Assert elemMatch and countMatch path : " + pathCurrent);
                    break;
                }

                i++;
                pathCurrent = path + "[ " + i + " ]";
            }

            // fail if y still have elements.
            countMatch = !yitr.MoveNext();
            Assert.IsTrue(elemMatch && countMatch, "EnumerableMatches(): Assert all elements and count matches. path : " + pathCurrent);

            return elemMatch && countMatch;
        }
        
        public static bool IsEnumerableNullOrEmpty(IEnumerable x)
        {
            // x is null?
            if(x == null)
            {
                return true;
            }

            // Test if enumerator is empty
            return x.GetEnumerator().MoveNext() == false;
        }

        public static bool? ReferenceMatchOrTypeMismatch(object x, object y, string path)
        {
            // Both are null then decision: true
            if (x == y)
            {
                // references are not equal. No need to compare more.
                return true;
            }

            // Enumerable
            if (x is IEnumerable || y is IEnumerable)
            {
                if(IsEnumerableNullOrEmpty((IEnumerable)x) && IsEnumerableNullOrEmpty((IEnumerable)y))
                {
                    // Matches if both enumerables are empty or null.
                    return true;
                }
            }else if (x is IEnumerable && y is IEnumerable)

            // Any of them them null imply exactly one is null but other is not.
            Assert.IsNotNull(x, "ReferenceMatchOrTypeMismatch(): Assert x is not null. path: " + path);
            Assert.IsNotNull(y, "ReferenceMatchOrTypeMismatch(): Assert y is not null. path: " + path);

            // type mismatch then decision: false.
            Assert.AreEqual<Type>(x.GetType(), y.GetType(), "ReferenceMatchOrTypeMismatch(): Assert Type matches Type. path: " + path);

            // References are not equal and no decisive mismatch found.
            // Continue comparing..
            return null;
        }

        public static List<string> CompareJsonObject(JObject x, JObject y, string path)
        {
            List<string> result;
            if (ReferenceMatch(x, y, path, out result).HasValue)
            {
                return result;
            }

            var xprops = x.Properties();
            var yprops = y.Properties();
            var uprops = xprops.Select(p => p.Name).Union(yprops.Select(p => p.Name));

            foreach (var propName in uprops)
            {
                var xprop = xprops.FirstOrDefault(p => p.Name == propName);
                var yprop = yprops.FirstOrDefault(p => p.Name == propName);

                result.AddRange(CompareJsonProperty(xprop, yprop, path + "." + propName));
            }

            return result;
        }

        public static List<string> CompareJsonProperty(JProperty x, JProperty y, string path)
        {
            List<string> result;
            if (ReferenceMatch(x, y, path, out result).HasValue)
            {
                return result;
            }

            result.AddRange(CompareJsonToken(x.Value, y.Value, path));
            return result;
        }

        public static List<string> CompareJsonArray(JArray x, JArray y, string path)
        {
            List<string> result;
            if (ReferenceMatch(x, y, path, out result).HasValue)
            {
                return result;
            }

            int i = 0;
            string pathCurrent = path + "[ " + i + " ]"; ;
            var yitr = y.Children().GetEnumerator();
            foreach (var xi in x)
            {
                if (yitr.MoveNext())
                {
                    var yi = yitr.Current;
                    result.AddRange(CompareJsonToken(xi, yi, pathCurrent));
                }
                else
                {
                    result.Add("CompareJsonArray(): Count mis-match, x has more elements. path : " + pathCurrent);
                }

                i++;
                pathCurrent = path + "[ " + i + " ]";
            }

            // error if y still have elements.
            if (yitr.MoveNext())
            {
                result.Add("CompareJsonArray(): Count mis-match, y has more elements. path : " + pathCurrent);
            }

            return result;
        }

        public static List<string> CompareJsonToken(JToken x, JToken y, string path)
        {
            List<string> result;
            if(ReferenceMatch(x, y, path, out result).HasValue)
            {
                return result;
            }

            if(x.Type == y.Type)
            {
                if(x.Type == JTokenType.Array)
                {
                    return CompareJsonArray((JArray)x, (JArray)y, path);
                }
                else if(x.Type == JTokenType.Object)
                {
                    return CompareJsonObject((JObject)x, (JObject)y, path);
                }
                else
                {
                    return CompareJsonValue((JValue)x, (JValue)y, path);
                }
            }
            else if(x is JValue && y is JValue)
            {
                return CompareJsonValue((JValue)x, (JValue)y, path);
            }
            else
            {
                result.Add(string.Format("CompareJsonToken(): type mismatch x.Type: {0} y.Type:: {1} path: {2}.", x.Type, y.Type, path));
            }

            return result;
        }

        public static List<string> CompareJsonValue(JValue x, JValue y, string path)
        {
            var result = new List<string>();

            // Default comparer or string equals
            if (JValue.EqualityComparer.Equals(x, y)
                || string.Equals(x.Value.ToString(), y.Value.ToString()))
            {
                return result;
            }

            result.Add(string.Format("CompareJsonValue(): value mismatch x: {0} y: {1} path: {2}.", x.Value, y.Value, path));
            return result;
        }

        public static bool? ReferenceMatch(object x, object y, string path, out List<string> result)
        {
            result = new List<string>();
            if (x == null && y == null)
            {
                result = new List<string>();
                return true;
            }

            if (x == null || y == null)
            {
                result.Add(string.Format("ReferenceMatch(): x: '{0}' y: '{1}' path: {2}.", x ?? "NULL", y ?? "NULL", path));
                return false;
            }

            return null;
        }

        public static ReplicaHealth CreateRandomHealth(this Random random)
        {
            ReplicaHealth health = null;
            Type targetType;
            
            if (random.Next() % 2 == 0)
            {
                targetType = typeof(StatefulServiceReplicaHealth);
            }
            else
            {
                targetType = typeof(StatelessServiceInstanceHealth);
            }

            health = (ReplicaHealth)random.CreateRandom(targetType);
            Assert.IsNotNull(health, "Assert EntityHealth 'health' is not null. Input targetType: " + targetType.Name);
            return health;
        }

        public static ServicePartitionInformation CreateRandomServicePartitionInformation(this Random random)
        {
            Type targetType;
            if (random.Next() % 3 == 0)
            {
                targetType = typeof(SingletonPartitionInformation);
            }
            else if (random.Next() % 3 == 1)
            {
                targetType = typeof(Int64RangePartitionInformation);
            }
            else
            {
                targetType = typeof(NamedPartitionInformation);
            }

            ServicePartitionInformation partitionInfo = (ServicePartitionInformation)random.CreateRandom(targetType);
            Assert.IsNotNull(partitionInfo, "Assert ServicePartitionInformation 'partitionInfo' is not null. Input targetType: " + targetType.Name);
            return partitionInfo;
        }

        public static SafetyCheck CreateRandomSafetyCheck(this Random random)
        {
            SafetyCheck result;
            SafetyCheckKind kind = random.CreateRandom<SafetyCheckKind>();
            switch (kind)
            {
                case SafetyCheckKind.EnsureSeedNodeQuorum:
                    result = new SeedNodeSafetyCheck(kind);
                    break;

                case SafetyCheckKind.EnsurePartitionQuorum:
                case SafetyCheckKind.WaitForInBuildReplica:
                case SafetyCheckKind.WaitForPrimaryPlacement:
                case SafetyCheckKind.WaitForPrimarySwap:
                case SafetyCheckKind.WaitForReconfiguration:
                case SafetyCheckKind.EnsureAvailability:
                    result = new PartitionSafetyCheck(kind, random.CreateRandom<Guid>());
                    break;

                default:
                    result = new UnknownSafetyCheck(kind);
                    break;
            }
            
            return result;
        }
        
        public static NodeId CreateRandomNodeId(this Random random)
        {
            return new NodeId(random.CreateRandom<System.Numerics.BigInteger>(), random.CreateRandom<System.Numerics.BigInteger>());
        }

        public static HealthEvaluation CreateRandomHealthEvaluation(this Random random)
        {
            var chooseEvent = random.Next() % 3 == 0;
            HealthEvaluationKind kind = chooseEvent ? HealthEvaluationKind.Event : CreateRandom<HealthEvaluationKind>(random);
            Type targetType = HealthEvaluation.GetDerivedHealthEvaluationClassTypeFromKind(kind);
            return (HealthEvaluation) random.CreateRandom(targetType);
        }

        public static ServiceTypeDescription CreateRandomServiceTypeDescription(this Random random)
        {
            ServiceTypeDescription result;
            ServiceDescriptionKind kind = random.CreateRandom<ServiceDescriptionKind>();
            switch (kind)
            {
                case ServiceDescriptionKind.Stateful:
                    result = random.CreateRandom<StatefulServiceTypeDescription>();
                    break;

                case ServiceDescriptionKind.Stateless:
                    result = random.CreateRandom<StatelessServiceTypeDescription>();
                    break;

                default:
                    return null;
            }

            result.ServiceTypeKind = kind;
            return result;
        }

        public static ServicePlacementPolicyDescription CreateRandomServicePlacementPolicyDescription(this Random random)
        {
            ServicePlacementPolicyType type = random.CreateRandom<ServicePlacementPolicyType>();
            ServicePlacementPolicyDescription result;

            switch (type)
            {
                case ServicePlacementPolicyType.InvalidDomain:
                    result = random.CreateRandom<ServicePlacementInvalidDomainPolicyDescription>();
                    break;

                case ServicePlacementPolicyType.PreferPrimaryDomain:
                    result = random.CreateRandom<ServicePlacementPreferPrimaryDomainPolicyDescription>();
                    break;

                case ServicePlacementPolicyType.RequireDomain:
                    result = random.CreateRandom<ServicePlacementRequiredDomainPolicyDescription>();
                    break;

                case ServicePlacementPolicyType.RequireDomainDistribution:
                    result = random.CreateRandom<ServicePlacementRequireDomainDistributionPolicyDescription>();
                    break;
                
                case ServicePlacementPolicyType.NonPartiallyPlaceService:
                    result = random.CreateRandom<ServicePlacementNonPartiallyPlaceServicePolicyDescription>();
                    break;

                default:
                    return null;
            }

            result.Type = type;
            return result;
        }

        public static Type GetImplementationType(this Random random, Type type) // type is abstract
        {
            // Search for KnowTypeAttributes.
            var knownType = GetRandomKnownImplementationType(random, type);
            if (knownType != null)
            {
                return knownType;
            }

            // if implements IList<elmT> or ICollection<elmT>
            Type specificType = DoesImplementICollection(type);
            if (specificType != null && type.IsAssignableFrom(specificType))
            {
                return specificType;
            }

            return null;
        }
    }
}