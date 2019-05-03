using System;
using System.Collections;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

namespace NetFwTypeLib
{
    [ComImport, Guid("98325047-C671-4174-8D81-DEFCD3F03186")]
    public interface INetFwPolicy2
    {
        [DispId(1)]
        int CurrentProfileTypes
        {
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(2)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        bool get_FirewallEnabled(NET_FW_PROFILE_TYPE2_ profileType);

        [DispId(2)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        void put_FirewallEnabled(NET_FW_PROFILE_TYPE2_ profileType, bool enabled);

        [DispId(3)]
        object ExcludedInterfaces
        {
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Struct)]
            get;
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.Struct)]
            set;
        }

        [DispId(4)]
        bool BlockAllInboundTraffic
        {
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(5)]
        bool NotificationsDisabled
        {
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(6)]
        bool UnicastResponsesToMulticastBroadcastDisabled
        {
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(7)]
        INetFwRules Rules
        {
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Interface)]
            get;
        }

        [DispId(8)]
        INetFwServiceRestriction ServiceRestriction
        {
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Interface)]
            get;
        }

        [DispId(12)]
        NET_FW_ACTION_ DefaultInboundAction
        {
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(13)]
        NET_FW_ACTION_ DefaultOutboundAction
        {
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(14)]
        bool IsRuleGroupCurrentlyEnabled
        {
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(15)]
        NET_FW_MODIFY_STATE_ LocalPolicyModifyState
        {
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(9)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        void EnableRuleGroup([In] int profileTypesBitmask, [MarshalAs(UnmanagedType.BStr)] [In] string group, [In] bool enable);

        [DispId(10)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        bool IsRuleGroupEnabled([In] int profileTypesBitmask, [MarshalAs(UnmanagedType.BStr)] [In] string group);

        [DispId(11)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        void RestoreLocalFirewallDefaults();
    }

    [ComImport, Guid("AF230D27-BABA-4E42-ACED-F524F22CFCE2")]
    public interface INetFwRule
    {
        [DispId(1)]
        string Name
        {
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(2)]
        string Description
        {
            [DispId(2)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(2)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(3)]
        string ApplicationName
        {
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(4)]
        string serviceName
        {
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(5)]
        int Protocol
        {
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(6)]
        string LocalPorts
        {
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(7)]
        string RemotePorts
        {
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(8)]
        string LocalAddresses
        {
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(9)]
        string RemoteAddresses
        {
            [DispId(9)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(9)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(10)]
        string IcmpTypesAndCodes
        {
            [DispId(10)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(10)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(11)]
        NET_FW_RULE_DIRECTION_ Direction
        {
            [DispId(11)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(11)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(12)]
        object Interfaces
        {
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Struct)]
            get;
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.Struct)]
            set;
        }

        [DispId(13)]
        string InterfaceTypes
        {
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(14)]
        bool Enabled
        {
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(15)]
        string Grouping
        {
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(16)]
        int Profiles
        {
            [DispId(16)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(16)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(17)]
        bool EdgeTraversal
        {
            [DispId(17)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(17)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(18)]
        NET_FW_ACTION_ Action
        {
            [DispId(18)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(18)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }
    }

    [ComImport, Guid("9C4C6277-5027-441E-AFAE-CA1F542DA009")]
    public interface INetFwRules : IEnumerable
    {
        [DispId(1)]
        int Count
        {
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(2)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        void Add([MarshalAs(UnmanagedType.Interface)] [In] NetFwRule rule);

        [DispId(3)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        void Remove([MarshalAs(UnmanagedType.BStr)] [In] string Name);

        [DispId(4)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        [return: MarshalAs(UnmanagedType.Interface)]
        NetFwRule Item([MarshalAs(UnmanagedType.BStr)] [In] string Name);
    }

    [ComImport, Guid("8267BBE3-F890-491C-B7B6-2DB1EF0E5D2B")]
    public interface INetFwServiceRestriction
    {
        [DispId(1)]
        void RestrictService([In, MarshalAs(UnmanagedType.BStr)] string serviceName, [In, MarshalAs(UnmanagedType.BStr)] string appName, [In] bool RestrictService, [In] bool serviceSidRestricted);
        [DispId(2)]
        bool ServiceRestricted([In, MarshalAs(UnmanagedType.BStr)] string serviceName, [In, MarshalAs(UnmanagedType.BStr)] string appName);
        [DispId(3)]
        INetFwRules Rules { [return: MarshalAs(UnmanagedType.Interface)] [DispId(3)] get; }
    }

    [ComImport, Guid("98325047-C671-4174-8D81-DEFCD3F03186"), CoClass(typeof(NetFwPolicy2Class))]
    public interface NetFwPolicy2 : INetFwPolicy2
    {
    }

    [ComImport, Guid("E2B3C97F-6AE1-41AC-817A-F6F92166D7DD"), ClassInterface(ClassInterfaceType.None)]
    public class NetFwPolicy2Class : INetFwPolicy2, NetFwPolicy2
    {
        [DispId(1)]
        public virtual extern int CurrentProfileTypes
        {
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(2)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        public virtual extern bool get_FirewallEnabled(NET_FW_PROFILE_TYPE2_ profileType);

        [DispId(2)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        public virtual extern void put_FirewallEnabled(NET_FW_PROFILE_TYPE2_ profileType, bool enabled);

        [DispId(3)]
        public virtual extern object ExcludedInterfaces
        {
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Struct)]
            get;
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.Struct)]
            set;
        }

        [DispId(4)]
        public virtual extern bool BlockAllInboundTraffic
        {
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(5)]
        public virtual extern bool NotificationsDisabled
        {
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(6)]
        public virtual extern bool UnicastResponsesToMulticastBroadcastDisabled
        {
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(7)]
        public virtual extern INetFwRules Rules
        {
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Interface)]
            get;
        }

        [DispId(8)]
        public virtual extern INetFwServiceRestriction ServiceRestriction
        {
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Interface)]
            get;
        }

        [DispId(12)]
        public virtual extern NET_FW_ACTION_ DefaultInboundAction
        {
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(13)]
        public virtual extern NET_FW_ACTION_ DefaultOutboundAction
        {
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(14)]
        public virtual extern bool IsRuleGroupCurrentlyEnabled
        {
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(15)]
        public virtual extern NET_FW_MODIFY_STATE_ LocalPolicyModifyState
        {
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
        }

        [DispId(9)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        public virtual extern void EnableRuleGroup([In] int profileTypesBitmask, [MarshalAs(UnmanagedType.BStr)] [In] string group, [In] bool enable);

        [DispId(10)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        public virtual extern bool IsRuleGroupEnabled([In] int profileTypesBitmask, [MarshalAs(UnmanagedType.BStr)] [In] string group);

        [DispId(11)]
        [MethodImplAttribute((MethodImplOptions)4096)]
        public virtual extern void RestoreLocalFirewallDefaults();
    }


    [ComImport, Guid("AF230D27-BABA-4E42-ACED-F524F22CFCE2"), CoClass(typeof(NetFwRuleClass))]
    public interface NetFwRule : INetFwRule
    {
    }

    [ClassInterface(ClassInterfaceType.None), Guid("2C5BC43E-3369-4C33-AB0C-BE9469677AF4")]
    [ComImport]
    public class NetFwRuleClass : INetFwRule, NetFwRule
    {
        [DispId(1)]
        public virtual extern string Name
        {
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(1)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(2)]
        public virtual extern string Description
        {
            [DispId(2)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(2)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(3)]
        public virtual extern string ApplicationName
        {
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(3)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(4)]
        public virtual extern string serviceName
        {
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(4)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(5)]
        public virtual extern int Protocol
        {
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(5)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(6)]
        public virtual extern string LocalPorts
        {
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(6)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(7)]
        public virtual extern string RemotePorts
        {
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(7)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(8)]
        public virtual extern string LocalAddresses
        {
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(8)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(9)]
        public virtual extern string RemoteAddresses
        {
            [DispId(9)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(9)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(10)]
        public virtual extern string IcmpTypesAndCodes
        {
            [DispId(10)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(10)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(11)]
        public virtual extern NET_FW_RULE_DIRECTION_ Direction
        {
            [DispId(11)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(11)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(12)]
        public virtual extern object Interfaces
        {
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.Struct)]
            get;
            [DispId(12)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.Struct)]
            set;
        }

        [DispId(13)]
        public virtual extern string InterfaceTypes
        {
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(13)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(14)]
        public virtual extern bool Enabled
        {
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(14)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(15)]
        public virtual extern string Grouping
        {
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [return: MarshalAs(UnmanagedType.BStr)]
            get;
            [DispId(15)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            [param: MarshalAs(UnmanagedType.BStr)]
            set;
        }

        [DispId(16)]
        public virtual extern int Profiles
        {
            [DispId(16)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(16)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(17)]
        public virtual extern bool EdgeTraversal
        {
            [DispId(17)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(17)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }

        [DispId(18)]
        public virtual extern NET_FW_ACTION_ Action
        {
            [DispId(18)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            get;
            [DispId(18)]
            [MethodImplAttribute((MethodImplOptions)4096)]
            set;
        }
    }

    public enum NET_FW_ACTION_
    {
        NET_FW_ACTION_BLOCK,
        NET_FW_ACTION_ALLOW,
        NET_FW_ACTION_MAX
    }

    public enum NET_FW_IP_PROTOCOL_
    {
        NET_FW_IP_PROTOCOL_ANY = 0x100,
        NET_FW_IP_PROTOCOL_TCP = 6,
        NET_FW_IP_PROTOCOL_UDP = 0x11
    }

    public enum NET_FW_IP_VERSION_
    {
        NET_FW_IP_VERSION_V4,
        NET_FW_IP_VERSION_V6,
        NET_FW_IP_VERSION_ANY,
        NET_FW_IP_VERSION_MAX
    }

    public enum NET_FW_MODIFY_STATE_
    {
        NET_FW_MODIFY_STATE_OK,
        NET_FW_MODIFY_STATE_GP_OVERRIDE,
        NET_FW_MODIFY_STATE_INBOUND_BLOCKED
    }

    public enum NET_FW_PROFILE_TYPE_
    {
        NET_FW_PROFILE_DOMAIN,
        NET_FW_PROFILE_STANDARD,
        NET_FW_PROFILE_CURRENT,
        NET_FW_PROFILE_TYPE_MAX
    }

    public enum NET_FW_PROFILE_TYPE2_
    {
        NET_FW_PROFILE2_ALL = 0x7fffffff,
        NET_FW_PROFILE2_DOMAIN = 1,
        NET_FW_PROFILE2_PRIVATE = 2,
        NET_FW_PROFILE2_PUBLIC = 4
    }

    public enum NET_FW_RULE_DIRECTION_
    {
        NET_FW_RULE_DIR_IN = 1,
        NET_FW_RULE_DIR_MAX = 3,
        NET_FW_RULE_DIR_OUT = 2
    }

    public enum NET_FW_SCOPE_
    {
        NET_FW_SCOPE_ALL,
        NET_FW_SCOPE_LOCAL_SUBNET,
        NET_FW_SCOPE_CUSTOM,
        NET_FW_SCOPE_MAX
    }

    public enum NET_FW_SERVICE_TYPE_
    {
        NET_FW_SERVICE_FILE_AND_PRINT,
        NET_FW_SERVICE_UPNP,
        NET_FW_SERVICE_REMOTE_DESKTOP,
        NET_FW_SERVICE_NONE,
        NET_FW_SERVICE_TYPE_MAX
    }
}