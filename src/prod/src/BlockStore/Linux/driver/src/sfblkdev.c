// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under both GPLv2 license.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/blkdev.h>
#include <linux/preempt.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/version.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <linux/inet.h>
#include <linux/proc_fs.h>
#include "sfblkdev.h"
#include <linux/types.h>
#include <linux/signal.h>
#include <linux/kthread.h>

// Linux Kernel version below 4.5.0
// * doesn't define macro for READ
// * defines READ/WRITE macros with different name
// * doesn't define macro to access flags
// So define them here for code readability
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,0)
#define REQ_OP_WRITE  REQ_WRITE
#define REQ_OP_READ 0
#define req_op(req) (int)(req->cmd_flags & 1)
#endif

#define MAX_SOCK_MGMT_TIMEOUT_JIFFIES   (300*HZ)
#define MAX_SOCK_TIMEOUT_JIFFIES        (30*HZ)
//Maximum number of times driver retries operation 
//failed with `operationfailed` || `invalidpayload` || `serverinvalidpayload`
//error.
#define MAX_FAILED_RETRY_OPERATION      10
//Maximum number of times driver retries operation when blockstore service is 
//unreachable.
#define MAX_DISCONNECT_RETRY_OPERATION  2000

static struct proc_dir_entry *g_sfdev_proc_dir, *g_sfdev_proc_file;
static int g_sfdev_major = -1;
static struct ida g_sfdev_ida;
static DEFINE_HASHTABLE(g_sfdev_hash, 8);
static spinlock_t g_sfdev_hash_lock;
static struct list_head g_sfdev_del_list;
static spinlock_t g_sfdev_del_list_lock;
static struct kmem_cache *g_rq_state_slab;
static atomic64_t tracking_counter;
static struct task_struct *g_kworker;
static atomic64_t module_reference_count;

static int connect_sfblkstore_svc(struct socket **blksock, uint32_t srvip, uint32_t srvport, int timeout)
{
    int ret;
    struct socket *sock;
    struct sockaddr_in srvaddr;

    ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0)
        goto out_nosock;

    //set timeout to 30 seconds.
    sock->sk->sk_rcvtimeo = sock->sk->sk_sndtimeo = timeout;

    //connect to remote server.
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(srvport);
    srvaddr.sin_addr.s_addr = srvip;

    ret = kernel_connect(sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr), 0);
    if (ret < 0)
        goto out;

    *blksock = sock;
    return 0;

out:
    sock_release(sock);
out_nosock:
    return ret;
}

static int disconnect_sfblkstore_svc(struct socket *blksock)
{
    //kernel_sock_shutdown(blksock, SHUT_RDWR);
    blksock->ops->release(blksock);
    sock_release(blksock);
    return 0;
}

static inline bool request_expired(unsigned long start_jiff)
{
    return time_after(jiffies, start_jiff + MAX_SOCK_TIMEOUT_JIFFIES);
}

static int sock_recv(struct socket *sock, void *data, int len, uint64_t tracking_id)
{
    struct msghdr msg;
    struct kvec iov;
    int ret = 0;
    int total_rx = 0;
    unsigned long start_time;
    sigset_t blocked, oldset;

    iov.iov_base = data;
    iov.iov_len = len;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = MSG_WAITALL|MSG_NOSIGNAL;

    start_time = jiffies;

    //mask all signals except SIGKILL.
    siginitsetinv(&blocked, sigmask(SIGKILL));
    sigprocmask(SIG_SETMASK, &blocked, &oldset);

    while(total_rx < len) {
        ret = kernel_recvmsg(sock, &msg, &iov, 1, iov.iov_len, 
                                MSG_WAITALL|MSG_NOSIGNAL);
        if (ret <= 0){
            if (ret == 0) {
                TRACE_ERR("Connection closed by block store service.");
                sigprocmask(SIG_SETMASK, &oldset, NULL);
                return -EIO;
            } else if (ret == -EAGAIN && !request_expired(start_time)) {
                TRACE_WARN("Received EAGAIN, retrying as request has not expired.");
                continue;
            } else if (ret < 0) {
                TRACE_ERR("RX failed! tracking_id:%llu, error %d, start_time %ld, "
                            "curr_time %ld, total_rcvd %d, expected_rcv %d, iov_len %ld "
                            "sport %d",
                            tracking_id, ret, start_time, jiffies, total_rx, len, iov.iov_len, 
                            ntohs(inet_sk(sock->sk)->inet_sport));
                sigprocmask(SIG_SETMASK, &oldset, NULL);
                return ret;
            }
        }
        total_rx += ret;
        iov.iov_base += total_rx;
        iov.iov_len -= total_rx;
    }
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    if(total_rx != len){
        TRACE_ERR("RX FAILED! tracking_id:%llu, total_rcvd:%d, expected_rcv:%d",
                    tracking_id, total_rx, len);
    }

    return total_rx;
}

static int sock_xmit(struct socket *sock, void *data, int len, int sockflags, uint64_t tracking_id)
{
    struct msghdr msg;
    struct kvec iov;
    int ret = 0;
    int total_tx = 0;
    unsigned long start_time;
    sigset_t blocked, oldset;
    
    iov.iov_base = data;
    iov.iov_len = len;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = MSG_NOSIGNAL|sockflags;

    start_time = jiffies;

    //mask all signals except SIGKILL.
    siginitsetinv(&blocked, sigmask(SIGKILL));
    sigprocmask(SIG_SETMASK, &blocked, &oldset);

    while (total_tx < len) {
        ret = kernel_sendmsg(sock, &msg, &iov, 1, len);
        //TODO: Does kernel_sendmsg ever return 0? If yes, we may run into 
        //infinite loop.
        if (ret < 0) {
            if (ret == -EAGAIN && !request_expired(start_time)){
                TRACE_WARN("Received EAGAIN, retrying as request has not expired.");
                continue;
            }else {
                TRACE_ERR("TX failed! tracking_id:%llu, error %d, start_time %ld, "
                            "curr_time %ld, total_xmit %d, expected_xmit %d, iov_len %ld "
                            "socket %d",
                            tracking_id, ret, start_time, jiffies, total_tx, len, iov.iov_len, 
                            ntohs(inet_sk(sock->sk)->inet_sport));
                sigprocmask(SIG_SETMASK, &oldset, NULL);
                return ret;
            }
        }
        total_tx += ret;
        iov.iov_base += total_tx;
        iov.iov_len -= total_tx;
    }
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    if(total_tx != len){
        TRACE_ERR("RX FAILED! tracking_id:%llu, total_xmit:%d, xmitted:%d",
                    tracking_id, total_tx, len);
    }


    return total_tx;
}

static void setup_common_params(block_cmd_header *header, blockcmd cmd, int payload_len_actual)
{
    int luid_len;
    //Name of the reliable dictionary in which LU registration are maintained.
    const char *luid_name = "LURegister";
    char *luid_name_in_header;
    int offset;
    void *payload_end;
    uint32_t end_of_payload_marker = END_OF_PAYLOAD_MARKER;

    header->cmd = cmd;
    header->rw_offset = 0;
    header->payload_len = BLOCK_SIZE_MANAGEMENT_REQUEST;
    header->payload_len_actual = payload_len_actual;
    header->srb = 0;

    // TODO: Overflow handling. Reboot scenario handling
    header->tracking_id = atomic64_inc_return(&tracking_counter);

    luid_len = strlen(luid_name) + 1;
    header->luid_name_length = luid_len * sizeof(wchar_t);

    //get the offset to luid name.
    offset = offsetof(block_cmd_header, luid_name);
    luid_name_in_header = (void *)(header) + offset;
    //convert ascii to utf-16 before copying data.
    convert_ascii_to_utf16((wchar_t *)luid_name_in_header, luid_name, luid_len);

    //move offset to reach payload end.
    offset += header->luid_name_length;
    payload_end = ((void *)header + offset);
    memcpy(payload_end, &end_of_payload_marker, sizeof(end_of_payload_marker));

    return;
}

static void setup_mgmt_xmit_buffer(void *buffer, blockcmd cmd, const char *luid_name, size_t size)
{
    block_cmd_header *header;
    int dev_luid_len;
    luid_registration_info *reginfo;
    int offset = GET_PAYLOAD_HEADER_SIZE;

    header = (block_cmd_header *)buffer;
    memset(header, 0, sizeof(block_cmd_header));

    switch(cmd)
    {
        case fetchregisterlu:
            setup_common_params(header, cmd, 0);
            //No payload data for fetchregisterlu request.
            break;

        case registerlu:
        case unregisterlu:
            setup_common_params(header, cmd, sizeof(luid_registration_info));
            //setup registration info. payload data starts after luid name.
            reginfo = (void *)((void *)header + offset);
            reginfo->disksize = BYTES_TO_MB(size);
            reginfo->unit = SIZE_IN_MB;
            reginfo->mounted = 0;
            dev_luid_len = strlen(luid_name) + 1;
            reginfo->luid_len = dev_luid_len * sizeof(wchar_t);
            convert_ascii_to_utf16((wchar_t *)reginfo->luid, luid_name, dev_luid_len);
            break;

        case umountlu:
        case mountlu:
            break;
        default:
            break;
    }
}

static int register_lu_request(const char *luid_name, size_t size, uint32_t srvip, uint16_t port)
{
    void *buffer;
    int ret = 0;
    struct socket *sock;
    block_cmd_header *header;
    void *payload_data;
    uint32_t end_of_payload_marker = END_OF_PAYLOAD_MARKER;
    int len = GET_PAYLOAD_HEADER_SIZE + BLOCK_SIZE_MANAGEMENT_REQUEST;

    TRACE_INFO("Setting up RegisterLU request for luid %s, size %ld, "
                    "srvip %#x, port %d", luid_name, size, srvip, port);

    ret = -ENOMEM;
    buffer = kmalloc(len, GFP_KERNEL);
    if(!buffer){
        TRACE_ERR("luid %s, buffer allocation failed.",luid_name);
        goto out_nobuf;
    }

    ret = connect_sfblkstore_svc(&sock, srvip, port, MAX_SOCK_MGMT_TIMEOUT_JIFFIES);
    if (ret < 0){
        TRACE_ERR("Remote connection to register luid %s with srvip %d, "
                    "port %d failed with error %d", luid_name, srvip, 
                    port, ret);
        goto out_connfail;
    }

    //Setup buffer.
    setup_mgmt_xmit_buffer(buffer, registerlu, luid_name, size);

    //Send Request.
    ret = sock_xmit(sock, buffer, len, 0, 0);
    if (ret < 0){
        TRACE_ERR("Sending registration request failed for luid %s with err %d",
                    luid_name, ret);
        goto out_srverr;
    }

    TRACE_INFO("Sent RegisterLU request for luid %s", luid_name);
    ////////////////////////////////////////////
    ////////////WAIT FOR RESPONSE///////////////
    ////////////////////////////////////////////

    header = (block_cmd_header *)buffer;
    memset(header, 0, sizeof(block_cmd_header));
    ret = sock_recv(sock, buffer, len, 0);
    if (ret < 0){
        TRACE_ERR("Failed receving registration request response for luid %s with err %d",
                    luid_name, ret);
        goto out_srverr;
    }
    
    if (header->cmd != operationcompleted) {
        TRACE_ERR("Registration of luid %s failed with result %d", luid_name, header->cmd);
        ret = -EIO;
        goto out_srverr;
    }

    //payload data starts after luid name.
    payload_data = (void *)(header) + offsetof(block_cmd_header, luid_name) + header->luid_name_length;
    memcpy(&end_of_payload_marker, payload_data, sizeof(end_of_payload_marker));

    if (end_of_payload_marker != END_OF_PAYLOAD_MARKER) {
        TRACE_ERR("RegisterLU for luid %s response contains invalid payload "
                    "marker %#x", luid_name, end_of_payload_marker);
        ret = -EIO;
        goto out_srverr;
    }
    ret = 0;
    TRACE_INFO("Successfully registered luid %s with blockstore service.", luid_name);

out_srverr:
    //Shutdown socket.
    disconnect_sfblkstore_svc(sock);
out_connfail:
    kfree(buffer);
out_nobuf:
    return ret;
}

static int fetch_register_lu_request(lu_registration_list *registration_list,
                                        uint32_t srvip, uint16_t port)
{
    void *buffer;
    int ret = 0;
    struct socket *sock;
    block_cmd_header *header;
    void *payload_data;
    uint32_t end_of_payload_marker;
    int len = GET_PAYLOAD_HEADER_SIZE + BLOCK_SIZE_MANAGEMENT_REQUEST;
    plu_registration_info reginfo;
    int i, bytes;

    TRACE_INFO("Fetch registered lu list from srvip %#x port %#x", srvip, port);

    buffer = kmalloc(len, GFP_KERNEL);
    if (!buffer) {
        TRACE_ERR("Buffer allocation failed.");
        ret = -ENOMEM;
        goto out_nobuf;
    }

    ret = connect_sfblkstore_svc(&sock, srvip, port, MAX_SOCK_MGMT_TIMEOUT_JIFFIES);
    if (ret < 0){
        TRACE_ERR("Remote connection to fetch luid list with srvip %d, "
                    "port %d failed with error %d", srvip, 
                    port, ret);
        goto out_connfail;
    }

    //Setup buffer.
    setup_mgmt_xmit_buffer(buffer, fetchregisterlu, NULL, 0);

    //Send Request.
    ret = sock_xmit(sock, buffer, len, 0, 0);
    if (ret < 0) {
        TRACE_ERR("Sending fetchluid request failed for with err %d",
                    ret);
        goto out_srverr;
    }

    ////////////////////////////////////////////
    ////////////WAIT FOR RESPONSE///////////////
    ////////////////////////////////////////////

    header = (block_cmd_header *)buffer;
    memset(header, 0, sizeof(block_cmd_header));
    ret = sock_recv(sock, buffer, len, 0);
    if (ret < 0) {
        TRACE_ERR("Failed receving fetchlu list response with err %d",
                    ret);
        goto out_srverr;
    }

    //payload data starts after luid name.
    payload_data = (void *)(header) + offsetof(block_cmd_header, luid_name) + header->luid_name_length;
    memcpy(&end_of_payload_marker, payload_data, sizeof(end_of_payload_marker));

    if (end_of_payload_marker != END_OF_PAYLOAD_MARKER) {
        TRACE_ERR("FetchLU list esponse contains invalid payload "
                    "marker %#x", end_of_payload_marker);
        ret = -EIO;
        goto out_srverr;
    }

    if (header->cmd != operationcompleted) {
        TRACE_ERR("FetchLU list failed with result %d", header->cmd);
        ret = -EIO;
        goto out_srverr;
    }

    ret = 0;
    //point to actual data.    
    payload_data = (void *)header + GET_PAYLOAD_HEADER_SIZE;

    //Get number of registration list.
    memcpy(&registration_list->count_entries, 
                payload_data, sizeof(registration_list->count_entries));
    payload_data += sizeof(registration_list->count_entries);

    if(registration_list->count_entries < 0 || 
        registration_list->count_entries > MAX_LU_REGISTRATION){
        ret = -EINVAL;
        goto out_srverr;
    }

    for(i=0; i<registration_list->count_entries; i++) {
        reginfo = &registration_list->registration_info[i];

        //Copy luid_len, disksize, unit and mounted.
        bytes = offsetof(luid_registration_info, luid);
        memcpy(reginfo, payload_data, bytes);

        //Move payload by bytes copied.
        payload_data += bytes;

        //Copy luid name.
        memcpy(reginfo->luid, payload_data, reginfo->luid_len);
        payload_data += reginfo->luid_len;
    }

    TRACE_INFO("FetchLU list returned %d registered entries from blockstore service.", 
                registration_list->count_entries);

out_srverr:
    //Shutdown socket.
    disconnect_sfblkstore_svc(sock);
out_connfail:
    //free buffer
    kfree(buffer);
out_nobuf:
    return ret;
}

static int unregister_lu_request(const char *luid_name, uint32_t srvip, uint16_t port)
{
    void *buffer;
    int ret;
    struct socket *sock;
    block_cmd_header *header;
    void *payload_data;
    uint32_t end_of_payload_marker;
    int len = GET_PAYLOAD_HEADER_SIZE + BLOCK_SIZE_MANAGEMENT_REQUEST;

    TRACE_INFO("Setting up UnregisterLU request for luid %s, "
                    "srvip %#x, port %d", luid_name, srvip, port);

    ret = -ENOMEM;
    buffer = kmalloc(len, GFP_KERNEL);
    if (!buffer){
        TRACE_ERR("Buffer allocation failed.");
        goto out_nobuf;
    }

    ret = connect_sfblkstore_svc(&sock, srvip, port, MAX_SOCK_MGMT_TIMEOUT_JIFFIES);
    if (ret < 0){
        TRACE_ERR("Remote connection to unregister luid %s with srvip %d, "
                    "port %d failed with error %d", luid_name, srvip, 
                    port, ret);
        goto out_connfail;
    }

    //Setup buffer.
    setup_mgmt_xmit_buffer(buffer, unregisterlu, luid_name, 0);

    //Send Request.
    ret = sock_xmit(sock, buffer, len, 0, 0);
    if (ret < 0){
        TRACE_ERR("Sending unregistration request failed for luid %s with err %d",
                    luid_name, ret);
        goto out_srverr;
    }

    ////////////////////////////////////////////
    ////////////WAIT FOR RESPONSE///////////////
    ////////////////////////////////////////////

    header = (block_cmd_header *)buffer;
    memset(header, 0, sizeof(block_cmd_header));
    ret = sock_recv(sock, buffer, len, 0);
    if (ret < 0){
        TRACE_ERR("Failed receving unregistration request response for luid %s with err %d",
                    luid_name, ret);
        goto out_srverr;
    }

    if (header->cmd != operationcompleted) {
        TRACE_ERR("Registration of luid %s failed with result %d", luid_name, header->cmd);
        //REMOVE BELOW ERROR ONCE WE HAVE CLEANUP_ON_CLOSE IN PLACE.
        //ret = -EIO;
        goto out_srverr;
    }

    //payload data starts after luid name.
    payload_data = (void *)(header) + offsetof(block_cmd_header, luid_name) + header->luid_name_length;
    memcpy(&end_of_payload_marker, payload_data, sizeof(end_of_payload_marker));

    if (end_of_payload_marker != END_OF_PAYLOAD_MARKER) {
        TRACE_ERR("UnregisterLU for luid %s response contains invalid payload "
                    "marker %#x", luid_name, end_of_payload_marker);
        ret = -EIO;
        goto out_srverr;
    }
    ret = 0;
    TRACE_INFO("Successfully unregistered luid %s with blockstore service.", luid_name);

out_srverr:
    //Shutdown socket.
    disconnect_sfblkstore_svc(sock);
out_connfail:
    kfree(buffer);
out_nobuf:
    return ret;
}

int sfblkdev_read_data(struct rw_disk_param *param)
{
    int err = 0;
    int i;
    uint32_t size;
    uint64_t pos;
    struct socket *sock;
    block_cmd_header *header;
    char *luid_name_in_header;
    void *payload_end;
    uint32_t end_of_payload_marker = END_OF_PAYLOAD_MARKER;
    int luid_len;
    int offset;
    void *data = NULL;

    err = connect_sfblkstore_svc(&sock, param->remote.ip, param->remote.port, MAX_SOCK_TIMEOUT_JIFFIES);
    if (err < 0)
        goto out_connfail;

    size = param->no_of_pages*PAGE_SIZE;
    pos = param->pos;

    header = (block_cmd_header *)kmalloc(GET_PAYLOAD_HEADER_SIZE, GFP_KERNEL);
    if (!header)
        goto out_memory;

    header->cmd = read; 
    header->tracking_id = atomic64_inc_return(&tracking_counter);
    header->rw_offset = pos;
    header->payload_len = size;
    header->payload_len_actual = header->srb = 0;
    luid_len = strlen(param->volume_name) + 1;
    header->luid_name_length = luid_len * sizeof(wchar_t);
    //get the offset to luid name.
    offset = offsetof(block_cmd_header, luid_name);
    luid_name_in_header = ((void *)header) + offset;
    //convert ascii to utf-16 before copying data.
    convert_ascii_to_utf16((wchar_t *)luid_name_in_header, 
                            param->volume_name, luid_len);
    //move offset to reach payload end.
    offset += header->luid_name_length;
    payload_end = ((void *)header + offset);
    memcpy(payload_end, &end_of_payload_marker, sizeof(end_of_payload_marker));

    //send header.
    err = sock_xmit(sock, header, GET_PAYLOAD_HEADER_SIZE, 0, 0);
    if (err < 0)
        goto out_srverr;

    data = (void *)__get_free_page(GFP_KERNEL);
    if (!data)
        goto out_data;

    memset(header, 0, GET_PAYLOAD_HEADER_SIZE);
    err = sock_recv(sock, header, GET_PAYLOAD_HEADER_SIZE, 0);

    if (header->cmd != operationcompleted) {
        goto out_srverr;
    }
    
    for (i=0; i<param->no_of_pages; i++) {
        err = sock_recv(sock, data, PAGE_SIZE, 0);
        if (err < 0)
            break;
        copy_to_user(param->data + i*PAGE_SIZE, data, PAGE_SIZE);
    }
out_srverr:
    if(data) free_page((unsigned long)data);
    err = 0;
    kfree(header);

out_data:
out_memory:
    disconnect_sfblkstore_svc(sock);
out_connfail:
    return err;
}

int sfblkdev_write_data(struct rw_disk_param *param)
{
    int err = 0;
    int ret, i;
    uint32_t size;
    uint64_t pos;
    struct socket *sock;
    block_cmd_header *header;
    char *luid_name_in_header;
    void *payload_end;
    uint32_t end_of_payload_marker = END_OF_PAYLOAD_MARKER;
    int luid_len;
    int offset;
    void *data;

    err = -EIO;
    ret = connect_sfblkstore_svc(&sock, param->remote.ip, param->remote.port, MAX_SOCK_TIMEOUT_JIFFIES);
    if (ret < 0)
        goto out_connfail;

    size = param->no_of_pages*PAGE_SIZE;
    pos = param->pos;

    header = (block_cmd_header *)kmalloc(GET_PAYLOAD_HEADER_SIZE, GFP_KERNEL);
    if (!header)
        goto out_memory;

    header->cmd = write; 
    header->rw_offset = pos;
    header->payload_len = size;
    header->payload_len_actual = header->srb = 0;
    luid_len = strlen(param->volume_name) + 1;
    header->luid_name_length = luid_len * sizeof(wchar_t);
    //get the offset to luid name.
    offset = offsetof(block_cmd_header, luid_name);
    luid_name_in_header = (void *)(header) + offset;
    //convert ascii to utf-16 before copying data.
    convert_ascii_to_utf16((wchar_t *)luid_name_in_header, 
                            param->volume_name, luid_len);
    //move offset to reach payload end.
    offset += header->luid_name_length;
    payload_end = ((void *)header + offset);
    memcpy(payload_end, &end_of_payload_marker, sizeof(end_of_payload_marker));

    //send header.
    err = sock_xmit(sock, header, GET_PAYLOAD_HEADER_SIZE, 0, 0);
    if (err < 0)
        goto out_srverr;

    data = (void *)__get_free_page(GFP_KERNEL);
    if (!data)
        goto out_data;

    for (i=0; i<param->no_of_pages; i++) {
        copy_from_user(data, param->data + i*PAGE_SIZE, PAGE_SIZE);
        err = sock_xmit(sock, data, PAGE_SIZE, 0, 0);
        if (err < 0)
            break;
    }
    free_page((unsigned long)data);
    err = 0;
    kfree(header);

out_data:
out_srverr:
out_memory:
    disconnect_sfblkstore_svc(sock);
out_connfail:
    return err;
}

/*blockstore device functions.*/
static int _sfblkdev_process_request(struct sfblkdisk *bdev, 
                                        struct request *req, 
                                        uint64_t tracking_id)
{
    uint32_t size;
    uint64_t pos;
    int err = 0;
    block_cmd_header *header = NULL;
    struct socket *sock;
    int offset;
    uint32_t count = 0;
    void *data;
    struct req_iterator iter;
    struct bio_vec bvec;
    char *luid_name_in_header;
    void *payload_end;
    uint32_t end_of_payload_marker = END_OF_PAYLOAD_MARKER;
  
    err = connect_sfblkstore_svc(&sock, bdev->remote.ip, bdev->remote.port, MAX_SOCK_TIMEOUT_JIFFIES);
    if(err < 0){
        TRACE_ERR("Couldn't connect to blockstore service: svcip %#x, port %d", 
                        bdev->remote.ip, bdev->remote.port);
        goto out_connfail;
    }

    size = blk_rq_bytes(req);
    pos = blk_rq_pos(req) * 512;    

    err = -ENOMEM;
    header = (block_cmd_header *)kmalloc(GET_PAYLOAD_HEADER_SIZE, GFP_KERNEL);
    if (!header) {
        TRACE_ERR("Couldn't allocate memory for request header.");
        goto out_memory;
    }
    header->rw_offset = pos;
    header->payload_len = size;
    header->luid_name_length = bdev->luid_name_len;
    header->payload_len_actual = header->srb = 0;
    header->tracking_id = tracking_id; 

    offset = offsetof(block_cmd_header, luid_name);
    luid_name_in_header = (void *)header + offset;
    memcpy(luid_name_in_header, bdev->volume_name_uni, bdev->luid_name_len);
    //move offset to reach payload end.
    offset += header->luid_name_length;
    payload_end = ((void *)header + offset);
    memcpy(payload_end, &end_of_payload_marker, sizeof(end_of_payload_marker));

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,0)
    switch (req->cmd_flags & 0x1)
#else
    switch (req_op(req))
#endif
    {
        case REQ_OP_WRITE:
            //prepare header.
            header->cmd = write;
       
            //send header.
            err = sock_xmit(sock, header, GET_PAYLOAD_HEADER_SIZE, 0, header->tracking_id);
            if (err < 0)
                goto out_srverr;

            //loop through each page and call sendpage.
            rq_for_each_segment(bvec, req, iter) {
                count += bvec.bv_len;
                data = kmap(bvec.bv_page);
                err = sock_xmit(sock, data+bvec.bv_offset, bvec.bv_len, 
                                    count == size ? 0 : 0, header->tracking_id);
                kunmap(bvec.bv_page);
                if (err < 0){
                    TRACE_ERR("sock_xmit failed for luid %s. Tracking:%llu, Offset %lld,"
                                " totalsize %u, transmitted %u, curr_size %u", 
                                bdev->volume_name, tracking_id, pos, size, 
                                count-bvec.bv_len, bvec.bv_len);
                    goto out_srverr;
                }
            }

            //wait for response.
            memset(header, 0, sizeof(block_cmd_header));
            err = sock_recv(sock, header, GET_PAYLOAD_HEADER_SIZE, tracking_id);
            if (err < 0){
                TRACE_ERR("sock_recv failed for luid %s. Tracking:%llu",
                                bdev->volume_name, tracking_id);
                goto out_srverr;
            }

            if (header->cmd != operationcompleted){
                TRACE_ERR("WRITE from luid %s failed. Tracking:%llu, Offset %lld,"
                                " size %u", bdev->volume_name, tracking_id, 
                                pos, size);
                err = header->cmd;
                goto out_srverr;
            }
            err = 0;
            break;

        case REQ_OP_READ: //READ
            header->cmd = read;

            //send header.
            err = sock_xmit(sock, header, GET_PAYLOAD_HEADER_SIZE, 0, header->tracking_id);
            if (err < 0){
                goto out_srverr;
            }

            //wait for response.
            memset(header, 0, sizeof(block_cmd_header));
            err = sock_recv(sock, header, GET_PAYLOAD_HEADER_SIZE, tracking_id);
            if (err < 0){
                goto out_srverr;
            }

            if (header->cmd != operationcompleted) {
                TRACE_ERR("READ from luid %s failed. Tracking:%llu, Offset %lld," 
                            "size %u", bdev->volume_name, tracking_id, pos, size);
                err = header->cmd;
                goto out_srverr;
            }

            rq_for_each_segment(bvec, req, iter) {
                data = kmap(bvec.bv_page);
                err = sock_recv(sock, data+bvec.bv_offset, bvec.bv_len, header->tracking_id);
                kunmap(bvec.bv_page);
                if (err < 0){
                    TRACE_ERR("READ from luid:%s failed. Tracking:%llu, "
                                "cmd:%d, offset:%lld, size:%u", bdev->volume_name, 
                                tracking_id, header->cmd, pos, size);
                    goto out_srverr;
                } 
                count += bvec.bv_len;
                if (count >= size) {
                    break;
                }
            }
            err = 0;
            break;
        default:
            TRACE_ERR("Invalid command %#x received!", req_op(req));
            err = -EIO;
            break;
    }

out_srverr:
    kfree(header);
out_memory:
    //Shutdown socket.
    disconnect_sfblkstore_svc(sock);
out_connfail:
    if(err != 0){
        TRACE_ERR("Request %d for luid %s failed with error %d", req_op(req),
                            bdev->volume_name, err);
    }else{
        TRACE_NOISE("Request %d for luid %s completed.", req_op(req),
                            bdev->volume_name);
    }
	return err;
}

static void sfblkdev_process_request(struct work_struct *work)
{
    struct sfreq_state *state = container_of(to_delayed_work(work), struct sfreq_state, dwork);
    int err;
    struct sfblkdisk *bdev = state->bdev;
    struct request *req = state->req;
    int retried_request = 0;
    unsigned long delay;
    int requeue = 0;

    if(state->start_jiff == 0){
        state->start_jiff = jiffies;
        state->tracking_id = atomic64_inc_return(&tracking_counter);
    }else{
        retried_request = 1;
    }

    err = _sfblkdev_process_request(bdev, req, state->tracking_id);

    if(err != 0) {
        if(!request_expired(state->start_jiff) && bdev->state == ready){
            requeue = 1;
            //Retry delay for error response from blockstore service.
            delay = msecs_to_jiffies(SFBLKDEV_RETRY_DELAY_IN_MS);
            if(err == operationfailed || err == invalidpayload || 
                err == serverinvalidpayload){
                //BlockStore service is reachable, but operation has failed, 
                if(state->retry_count > MAX_FAILED_RETRY_OPERATION)
                {
                    requeue = 0;
                }
            }else{
                //BlockStore service is not reachable
                if(state->retry_count > MAX_DISCONNECT_RETRY_OPERATION)
                {
                    requeue = 0;
                }
            }
            if(requeue)
            {
                INIT_DELAYED_WORK(&state->dwork, sfblkdev_process_request);
                queue_delayed_work(bdev->wq, &state->dwork, delay);
                state->retry_count++;
                return;
            }
        }
        if(err == operationfailed || err == invalidpayload || 
                err == serverinvalidpayload){
            err = -EIO;
        }
    }

    if(retried_request && err == 0){
        TRACE_INFO("***BlockStore service seems to have recovered!"
                        " Tracking:%lld completed successfully!***",
                        state->tracking_id);
    }

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,13,0)
    if(err != 0)
        err = BLK_STS_IOERR;
    else
        err = BLK_STS_OK;
#endif

    kmem_cache_free(g_rq_state_slab, state);
    blk_end_request_all(state->req, err);
    return;
}

static void blk_rq_func(struct request_queue *q)
{
    struct sfblkdisk *bdev = q->queuedata;
    struct request *req;
    struct sfreq_state *state;
    unsigned long flags;

    while ((req = blk_peek_request(q)) != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,0)
	    if ((req->cmd_type != REQ_TYPE_FS))
#else
        if((req_op(req) != REQ_OP_READ &&
            req_op(req) != REQ_OP_WRITE))
#endif
        {
            blk_start_request(req);
            blk_end_request_all(req, -EIO);
            TRACE_ERR("Received unsupported request %d, completing with -EIO", req_op(req));
            continue;
        }

        state = kmem_cache_alloc(g_rq_state_slab, GFP_ATOMIC);
        if (!state) {
            continue;
        }
        state->start_jiff = 0;
        state->tracking_id = 0;
        state->req = req;
        state->bdev = bdev;
        state->retry_count = 0;

        blk_start_request(req);
        spin_lock_irqsave(&bdev->lock, flags);
        if (bdev->state == ready){
            //Enqueue the request.
            INIT_DELAYED_WORK(&state->dwork, sfblkdev_process_request);
            queue_delayed_work(bdev->wq, &state->dwork, 0);
        }else{
            blk_end_request_all(req, -EIO);
            TRACE_ERR("Volume %s, device %s, not ready - %d. Completing request with -EIO", 
                            bdev->volume_name, bdev->device_name, bdev->state);
        }
        spin_unlock_irqrestore(&bdev->lock, flags);
    }
}

static int sfblkdev_open(struct block_device *bldev, fmode_t mode)
{
    struct sfblkdisk *bdev = bldev->bd_disk->private_data;
    unsigned long flags;

    spin_lock_irqsave(&bdev->lock, flags);
    if(bdev->state != ready){
        spin_unlock_irqrestore(&bdev->lock, flags);
        TRACE_INFO("open failed for device %s, volume %s, state %d", bdev->device_name, bdev->volume_name, bdev->state);
        return -EIO;
    }
    bdev->open_count++;
    //kref_get(&bdev->refcount);
    spin_unlock_irqrestore(&bdev->lock, flags);
    TRACE_INFO("Volume %s, device %s, open_count %d, refcount %ld "
                    "successfully opened.", bdev->volume_name, 
                    bdev->device_name, bdev->open_count,
                    (long)atomic64_inc_return(&module_reference_count));
    return 0;
}

static void sfblkdev_release(struct gendisk *disk, fmode_t mode)
{
    struct sfblkdisk *bdev = disk->private_data;
    unsigned long flags;

    spin_lock_irqsave(&bdev->lock, flags);
    bdev->open_count--;
    spin_unlock_irqrestore(&bdev->lock, flags);
    TRACE_INFO("Volume %s, device %s, open_count %d, refcount %ld "
                    "successfully closed.", bdev->volume_name, 
                    bdev->device_name, bdev->open_count,
                    (long)atomic64_dec_return(&module_reference_count));
}

static struct block_device_operations block_fops =
{
    .owner = THIS_MODULE,
    .open = sfblkdev_open,
    .release = sfblkdev_release,
};

static int create_disk(struct sfblkdisk *bdev, int skip_registration)
{
    struct request_queue *q;
    int err;
    struct gendisk *disk;

    err = -ENOMEM;
    disk = alloc_disk(1);
    if (!disk)
        goto out;

    disk->major = bdev->major;
    disk->first_minor = (bdev->minor - 1);
    disk->flags |= GENHD_FL_REMOVABLE | GENHD_FL_NO_PART_SCAN;
    disk->private_data = bdev;
    disk->fops = &block_fops;
    //set disk size.
    set_capacity(disk, bdev->size / BLK_SECTOR_SIZE);

    err = -ENOMEM;
    q = blk_init_queue(blk_rq_func, &bdev->qlock);
    if (!q)
        goto out_queue;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
    queue_flag_clear_unlocked(QUEUE_FLAG_DISCARD, q);
    queue_flag_set_unlocked(QUEUE_FLAG_NONROT, q);
    queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, q);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,6,0)
    queue_flag_clear(QUEUE_FLAG_WC, q);
    queue_flag_clear(QUEUE_FLAG_FUA, q);
#endif
#else
    blk_queue_flag_clear(QUEUE_FLAG_DISCARD, q);
    blk_queue_flag_set(QUEUE_FLAG_NONROT, q);
    blk_queue_flag_clear(QUEUE_FLAG_ADD_RANDOM, q);
    blk_queue_flag_clear(QUEUE_FLAG_WC, q);
    blk_queue_flag_clear(QUEUE_FLAG_FUA, q);
#endif
    blk_queue_max_hw_sectors(q, 65536);
    blk_queue_max_segment_size(q, BLK_MAX_SEGMENT_LMT);
    blk_queue_io_min(q, PAGE_SIZE);
    blk_queue_max_discard_sectors(q, 0);
    blk_queue_max_write_same_sectors(q, 0);

    q->limits.max_sectors = 256;
    q->queuedata = bdev;

    disk->queue = q;
    bdev->disk = disk;
    strcpy(disk->disk_name, bdev->device_name);

    if(!skip_registration) {
        err = register_lu_request(bdev->volume_name, bdev->size, 
                            bdev->remote.ip, bdev->remote.port);
        if (err < 0){
            TRACE_ERR("Registration of disk %s failed with blkstore service "
                            "ip %#x, port %d", bdev->volume_name,
                            bdev->remote.ip, bdev->remote.port);
            goto out_blkstore_err;
        }
    }else{
        TRACE_INFO("Skipping registration of volume %s with blkstore service.",
                        bdev->volume_name);
    }

    //Mark disk as ready.
    bdev->state = ready;

    //Add disk in system.
    add_disk(bdev->disk);

    TRACE_INFO("volume %s with device file %s created succesfully.", 
                            bdev->volume_name, bdev->device_name);
    return 0;

out_blkstore_err:
    blk_cleanup_queue(disk->queue);
out_queue:
    put_disk(disk);
out:
    return err;
}

static lu_registration_list *create_registration_list(void)
{
    return kmalloc(sizeof(lu_registration_list), GFP_KERNEL);
}

static void free_registration_list(lu_registration_list *lu_list)
{
    kfree(lu_list);
    return;
}

static bool volume_exists(const char *volume_name,
                            unsigned int ip, unsigned short port, 
                            struct sfblkdisk **bldev)
{
    unsigned int hash;
    bool exist = false;
    struct sfblkdisk *bdev;
    unsigned long flags;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,5,0)
    hash = full_name_hash(volume_name, strlen(volume_name));
#else
    hash = full_name_hash(NULL, volume_name, strlen(volume_name));
#endif
    spin_lock(&g_sfdev_hash_lock);
    hash_for_each_possible(g_sfdev_hash, bdev, list, hash) {
        spin_lock_irqsave(&bdev->lock, flags); 
        if (bdev->state == ready && 
                strcmp(bdev->volume_name, volume_name) == 0 &&
                bdev->remote.ip == ip) {
            if(bdev->remote.port != port) {
                //update the port. This can happen when blockstore service crashes.
                TRACE_INFO("Updating volume %s port. oldport:%d newport:%d",
                                 bdev->volume_name, bdev->remote.port, port);
                bdev->remote.port = port;
            }
            exist = true;
            if(bldev) 
                *bldev = bdev;
            spin_unlock_irqrestore(&bdev->lock, flags);
            break;
        }
        spin_unlock_irqrestore(&bdev->lock, flags);
    }
    spin_unlock(&g_sfdev_hash_lock);
    return exist;
}

static int sfblkdev_create_disk(struct create_disk_param *param, int skip_registration)
{
    struct sfblkdisk *bdev;
    int minor;
    int max_minor = 1<<MINORBITS;
    int err = 0;
    int name_len;
    unsigned char wq_name[24];

    TRACE_INFO("Creating disk %s, ip %#x, port %d, size %#llx", param->volume_name, param->remote.ip, param->remote.port, param->size);
    //If volume exists, return EEXIST.
    if(volume_exists(param->volume_name,
                        param->remote.ip, param->remote.port, &bdev)){
        //copy device file name.
        TRACE_INFO("Disk with same name %s exists, returning EEXIST", param->volume_name);
        strcpy(param->device_name, bdev->device_name);
        return -EEXIST;
    }
    
    //Hold the module reference to avoid unloading of the module 
    //without deleting the device.
    err = -ENODEV;
    if (!try_module_get(THIS_MODULE)){
        TRACE_ERR("Couldn't get volume reference!");
        goto out;
    }
    TRACE_INFO("Acquired Module Reference Count %ld", (long)atomic64_inc_return(&module_reference_count));

    err = -ENOMEM;
    bdev = kzalloc(sizeof(*bdev), GFP_KERNEL);
    if (!bdev){
        TRACE_ERR("Couldn't allocate bdev!");
        goto out_module;
    }
    bdev->size = param->size;

    //Initialize device lock.    
    spin_lock_init(&bdev->lock);
    bdev->open_count = 0;

    //Initialize request queue lock.
    spin_lock_init(&bdev->qlock);
   
    //Allocate minor.
    err = minor = ida_simple_get(&g_sfdev_ida, 1, max_minor, GFP_KERNEL);
    if(err < 0)
        goto out_minor;

    //Allocate workqueue.
    snprintf(wq_name, sizeof(wq_name), "sfblkdev_%d", minor);

    bdev->wq = alloc_workqueue(wq_name, WQ_MEM_RECLAIM, 1);
    if(!bdev->wq)
        goto out_wq;

    bdev->minor = minor;
    bdev->major = g_sfdev_major;
    bdev->devno = MKDEV(g_sfdev_major, minor);
    //remote endpoint of blockstore service.
    bdev->remote = param->remote;
    snprintf(bdev->device_name, SFBLKDEV_MAX_DEV_NAME_LEN, "sfblkdev_%d", minor);
    //copy volume name. This is not same as device file name. 
    //BlockStoreService needs volume_name to be passed with every read/write ops.
    strcpy(bdev->volume_name, param->volume_name);
    name_len = strlen(bdev->volume_name)+1;
    bdev->luid_name_len = name_len*2;

    //remote end needs wchar representation of device id.
    convert_ascii_to_utf16(bdev->volume_name_uni, 
                            bdev->volume_name, name_len);
 
    //Create disk.
    err = create_disk(bdev, skip_registration);
    if (err < 0){
        TRACE_ERR("Creating disk %s failed", bdev->volume_name);
        goto out_disk;
    }

    //copy device file name.
    strcpy(param->device_name, bdev->device_name);

    //Add bdev into list.
    spin_lock(&g_sfdev_hash_lock);
    hash_add(g_sfdev_hash, &bdev->list, 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,5,0)
    		full_name_hash(bdev->volume_name, strlen(bdev->volume_name))
#else
    		full_name_hash(NULL, bdev->volume_name, strlen(bdev->volume_name))
#endif 
                );
    spin_unlock(&g_sfdev_hash_lock);

    return 0;

out_disk:
    destroy_workqueue(bdev->wq);
out_wq:
    ida_simple_remove(&g_sfdev_ida, minor);
out_minor:
    kfree(bdev);
out_module:
    module_put(THIS_MODULE);
    TRACE_INFO("Released Module Reference Count %ld", (long)atomic64_dec_return(&module_reference_count));
out:
    return err;
}

static int sfblkdev_refresh_disks(struct refresh_disk_param *param)
{
    int count = 0;
    int err = 0;
    lu_registration_list *lu_list;
    int i;
    size_t size;
    disksizeunit unit;
    struct create_disk_param create_disk_param;
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];

    lu_list = create_registration_list();
    if (!lu_list){
        TRACE_ERR("Couldn't allocate registration list.");
        err = -ENOMEM;
        goto out_no_list;
    }

    TRACE_INFO("Refresh disk with blkstore service ip %#x, port %d", param->remote.ip, param->remote.port);

    err = fetch_register_lu_request(lu_list, param->remote.ip, 
                                    param->remote.port);
    if (err < 0)
    {
        TRACE_ERR("fetch_register_lu_request failed with error %d", err);
        goto out_blkstore_service;
    }
    
    count = lu_list->count_entries;
    TRACE_INFO("Found %d registered entries with blockstore service.", count);

    for (i=0; i<count; i++)
    { 
        unit = lu_list->registration_info[i].unit;
        size = lu_list->registration_info[i].disksize;
        
        if(unit == SIZE_IN_MB)
            size = size*MB(1);
        else if(unit == SIZE_IN_GB)
            size = size*GB(1);
        else if(unit == SIZE_IN_TB)
            size = size*TB(1UL);

        convert_utf16_to_ascii(volume_name, 
                            lu_list->registration_info[i].luid, 
                            lu_list->registration_info[i].luid_len);

        TRACE_INFO("Disk[%d], Name[%s], Size[%lu]\n", i, volume_name, size);
        //If volume exists, ignore disk creation.
        if(!volume_exists(volume_name, param->remote.ip, 
                            param->remote.port, NULL)){
            //setup create_disk_param.
            memset(&create_disk_param, 0, sizeof(create_disk_param));
            create_disk_param.type = param->type;
            create_disk_param.remote = param->remote;
            create_disk_param.size = size;
            strcpy(create_disk_param.volume_name, volume_name);

            if((err = sfblkdev_create_disk(&create_disk_param, 1)) != 0){
                TRACE_ERR("create disk failed with error %d", err);
                break;
            }
        }else{
            TRACE_INFO("Disk %s already exists for ip %#x, port %#x", volume_name, 
                        param->remote.ip, param->remote.port);
        }
    }

    free_registration_list(lu_list);
    return 0;

out_blkstore_service:
    free_registration_list(lu_list);
out_no_list:
    return err;
}

static void add_bdev_in_del_list(struct sfblkdisk *bdev)
{
    TRACE_INFO("Adding volume %s, device %s in delete list." 
                " Current open count is %d.", bdev->volume_name, 
                    bdev->device_name, bdev->open_count);
    spin_lock(&g_sfdev_del_list_lock);
    list_add_tail(&bdev->del_list, &g_sfdev_del_list);
    spin_unlock(&g_sfdev_del_list_lock);
}

static int _sfblkdev_delete_disk(struct sfblkdisk *bdev, int skip_unregistration)
{
    struct gendisk *disk;
    int err;
    unsigned long flags;

    spin_lock_irqsave(&bdev->lock, flags);
    if(bdev->state != ready){
        TRACE_ERR("Delete disk of volume %s, device %s, state %d failed", 
                        bdev->volume_name, bdev->device_name, bdev->state);
        spin_unlock_irqrestore(&bdev->lock, flags);
        return -1;
    }
    //mark disk as ready-to-delete
    bdev->state = marked_for_delete;
    spin_unlock_irqrestore(&bdev->lock, flags);

    TRACE_INFO("Deleting volume %s, skip %d", bdev->volume_name, skip_unregistration);

    if(!skip_unregistration) {
        //Remove disk from BlockStore Service.
        err = unregister_lu_request(bdev->volume_name, bdev->remote.ip, 
                                        bdev->remote.port);
        if (err < 0) {
            TRACE_ERR("Failed removing disk %s from blockstore service.", bdev->volume_name);
            //Mark disk as ready again and fail the delete_disk operation.
            spin_lock_irqsave(&bdev->lock, flags);
            bdev->state = ready;
            spin_unlock_irqrestore(&bdev->lock, flags);
            return err;
        }
    }
    disk = bdev->disk;

    //Drain and mark request queue dying. All future requests to this queue 
    //will fail with -ENODEV.
    blk_cleanup_queue(disk->queue);

    synchronize_rcu();

    //drain any pending work.
    drain_workqueue(bdev->wq);

    hash_del(&bdev->list); 

    //Add this bdev in marked_for_delete list. Kthread will pick up this bdev
    //and free rest of data structure after bdev->open_count reaches 0.
    add_bdev_in_del_list(bdev);
    return 0;
}

static int sfblkdev_free_bdev(struct sfblkdisk *bdev)
{
    struct gendisk *disk;
    
    TRACE_INFO("Freeing volume %s, device %s, minor number %d", bdev->volume_name, bdev->device_name, bdev->minor);

    disk = bdev->disk;
    del_gendisk(disk);
    put_disk(disk);

    //destroy work queue.
    destroy_workqueue(bdev->wq);

    ida_simple_remove(&g_sfdev_ida, bdev->minor);

    list_del(&bdev->del_list);

    //Free bdev.
    kfree(bdev);
    //Release module reference.
    module_put(THIS_MODULE);
    TRACE_INFO("Released Module Reference Count %ld", (long)atomic64_dec_return(&module_reference_count));
    return 0;
}

static int sfblkdev_delete_disk(struct delete_disk_param *param)
{
    struct sfblkdisk *bdev;

    if(!volume_exists(param->volume_name, param->remote.ip, 
                        param->remote.port, &bdev)){
        return -EINVAL;
    }
    spin_lock(&g_sfdev_hash_lock);
    _sfblkdev_delete_disk(bdev, 0);
    spin_unlock(&g_sfdev_hash_lock);
    return 0;
}

static int sfblkdev_shutdown_disks(struct shutdown_disk_param *param)
{
    struct sfblkdisk *bdev;
    struct hlist_node *tmp;
    int bkt;

    spin_lock(&g_sfdev_hash_lock);
    hash_for_each_safe(g_sfdev_hash, bkt, tmp, bdev, list) {
        if ((bdev->remote.ip == param->remote.ip &&
            bdev->remote.port == param->remote.port)) {
            if (_sfblkdev_delete_disk(bdev, 1) != 0) {
                //couldn't delete volume??
                TRACE_ERR("couldn't delete volume %s", bdev->volume_name);
            }
        }
    }
    spin_unlock(&g_sfdev_hash_lock);
    return 0;
}

static int sfblkdev_get_disks_local(struct get_disk_param *param)
{
    struct sfblkdisk_info info;
    struct sfblkdisk *bdev;
    int count = 0;
    int err;
    int bkt;
    int req_count = param->count;
    struct sfblkdisk_info *disks = param->disks;

    spin_lock(&g_sfdev_hash_lock);
    hash_for_each(g_sfdev_hash, bkt, bdev, list) {
        if(param->remote.port != -1 && 
            bdev->remote.port != param->remote.port){
            continue;
        }

        strcpy(info.volume_name, bdev->volume_name);
        strcpy(info.device_name, bdev->device_name);
        info.open_count = bdev->open_count;
        info.size = (unsigned long long)bdev->size;
        err = copy_to_user((void *)(disks+count), &info, sizeof(info));
        if (err != 0){
            count = err;
            break;
        }
        count++;
        if (count >= req_count)
            break;
    }
    spin_unlock(&g_sfdev_hash_lock);
    param->valid_count = count;

    return 0;
}

static int sfblkdev_get_disks(struct get_disk_param *param)
{
    int count = 0;
    int err;
    lu_registration_list *lu_list;
    int i;
    uint32_t size;
    disksizeunit unit;
    int req_count = param->count;
    struct sfblkdisk_info *disks = param->disks;
    struct sfblkdisk_info disk;

    lu_list = create_registration_list();
    if (!lu_list)
        return -ENOMEM;

    err = fetch_register_lu_request(lu_list, param->remote.ip, 
                                    param->remote.port);
    if (err < 0)
    {
        TRACE_ERR("fetch_register_lu_request failed with error %d", err);
        goto out_blkstore_service;
    }

    //limit the number of entries to req_count
    count = lu_list->count_entries < req_count ? 
                        lu_list->count_entries:req_count;

    param->valid_count = count;
    for(i=0; i<count; i++)
    {
        unit = lu_list->registration_info[i].unit;
        size = lu_list->registration_info[i].disksize;

        if(unit == SIZE_IN_MB)
            disk.size = size*MB(1);
        else if(unit == SIZE_IN_GB)
            disk.size = size*GB(1);
        else if(unit == SIZE_IN_TB)
            disk.size = size*TB(1UL);
        disk.open_count = 0;

        convert_utf16_to_ascii(disk.volume_name, 
                            lu_list->registration_info[i].luid, 
                            lu_list->registration_info[i].luid_len);
        err = copy_to_user((void *)(disks+i), &disk, sizeof(disk));
        if (err != 0){
            count = err;
            break;
        }
    }

out_blkstore_service:
    free_registration_list(lu_list);
    return 0;
}

struct sfblkdev_ctrl_ops g_nw_mgmt_ops = 
{
    .type = NW_SFBLKDEV_TYPE,
    .create_disk = sfblkdev_create_disk,
    .delete_disk = sfblkdev_delete_disk,
    .get_disks = sfblkdev_get_disks,
    .get_disks_local = sfblkdev_get_disks_local,
    .read_data = sfblkdev_read_data,
    .write_data = sfblkdev_write_data,
    .refresh_disks = sfblkdev_refresh_disks,
    .shutdown_disks = sfblkdev_shutdown_disks,
};


static int sfblkdev_worker(void *data)
{
    struct sfblkdisk *bdev, *t;
    while (!kthread_should_stop()) {
        spin_lock(&g_sfdev_del_list_lock);
        list_for_each_entry_safe(bdev, t, &g_sfdev_del_list, del_list){
            if(bdev->open_count == 0){
                sfblkdev_free_bdev(bdev);
            }
        }
        spin_unlock(&g_sfdev_del_list_lock);
        //yield cpu.
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(5*HZ);
    }    
    return 0;
}

static int sfblkdev_proc_show(struct seq_file *seq_file, void *data)
{
    struct sfblkdisk *bdev;
    struct hlist_node *tmp;
    int bkt;

    seq_puts(seq_file, "**Device Stats**\n");
    spin_lock(&g_sfdev_hash_lock);
    hash_for_each_safe(g_sfdev_hash, bkt, tmp, bdev, list) {
        seq_printf(seq_file, "Volume:[%s], Device:[%s], State:[%d],"
                                " Ip:[%#x], Port:[%d]," 
                                " OpenCount:[%d]\n",
                        bdev->volume_name, bdev->device_name, bdev->state,
                        bdev->remote.ip, bdev->remote.port,
                        bdev->open_count);
    }
    spin_unlock(&g_sfdev_hash_lock);
    return 0;
}

static int sfblkdev_proc_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sfblkdev_proc_show, NULL);
}

const struct file_operations g_sfdev_proc_ops= {
    .open       = sfblkdev_proc_stats_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .owner      = THIS_MODULE,
};

int setup_proc_files(void)
{
    g_sfdev_proc_dir = proc_mkdir("sfblkstore", NULL);
    if(!g_sfdev_proc_dir){
        printk("Couldn't create sfblkstore proc dir!");
        return -1;
    }

    g_sfdev_proc_file = proc_create("devices", S_IFREG|S_IRUGO, g_sfdev_proc_dir, &g_sfdev_proc_ops);
    if(!g_sfdev_proc_file)
    {
        printk("Coudn't create sfblkstore device proc file.\n");
        proc_remove(g_sfdev_proc_dir);
        return -1;
    }
    return 0;
}

void cleanup_proc_files(void)
{
    if(g_sfdev_proc_file)
        proc_remove(g_sfdev_proc_file);
    
    if(g_sfdev_proc_dir)
        proc_remove(g_sfdev_proc_dir);
    return;
}

int initialize_sfblkdev(void)
{
    int err = -1;

    if(setup_proc_files()){
        printk("Couldn't create proc files!\n");
        return -1;
    }

    atomic64_set(&tracking_counter, 0);
    atomic64_set(&module_reference_count, 0);

    hash_init(g_sfdev_hash);
    ida_init(&g_sfdev_ida);

    spin_lock_init(&g_sfdev_hash_lock);

    g_rq_state_slab = kmem_cache_create("sfreq_state", 
                                        sizeof(struct sfreq_state), 
                                        0, 0, NULL);
    if(!g_rq_state_slab)
        goto exit;

    //register mgmt operations.
    err = register_sfblkdev_ctrl_ops(&g_nw_mgmt_ops);
    if (err < 0)
        goto out_ctrl;

    err = g_sfdev_major = register_blkdev(0, SFBLKDEV_NAME);
    if (err < 0) {
        TRACE_ERR("register_blkdev failed with error %d", err);
        goto out_major;
    }

    //start a kthread to delete "shutdown" block devices.
    spin_lock_init(&g_sfdev_del_list_lock);
    INIT_LIST_HEAD(&g_sfdev_del_list);
    g_kworker = kthread_run(sfblkdev_worker, NULL, "sfblkdev-worker-%d", 0);

    TRACE_INFO("Reserved major number %d for sf blockstore device", 
                    g_sfdev_major);
    return 0;
    
out_major:
    unregister_blkdev(g_sfdev_major, SFBLKDEV_NAME);
out_ctrl:
    kmem_cache_destroy(g_rq_state_slab);
exit:
    ida_destroy(&g_sfdev_ida);
    cleanup_proc_files();
    return err;
}

void uninitalize_sfblkdev(void)
{
    kmem_cache_destroy(g_rq_state_slab);
    unregister_sfblkdev_ctrl_ops(&g_nw_mgmt_ops);
    unregister_blkdev(g_sfdev_major, SFBLKDEV_NAME);
    ida_destroy(&g_sfdev_ida);
    kthread_stop(g_kworker);
    cleanup_proc_files();
    TRACE_INFO("Released major number %d reserved for sf blockstore devices", 
                    g_sfdev_major);
    return;
}
