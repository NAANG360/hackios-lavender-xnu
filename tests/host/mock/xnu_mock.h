/*
 * Host mock of the few XNU kernel interfaces used by bsd/dev/sweet_keys.c and
 * bsd/dev/sweet_usb.c. Every stub header under mock/ includes only this
 * file, so the real driver sources compile unchanged on the host. MMIO is
 * simulated by the test: ml_io_map() returns host addresses of fake
 * register files, and delay() is the hook where fake hardware reacts.
 */
#ifndef XNU_MOCK_H
#define XNU_MOCK_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* Darwin LP64: uint64_t is unsigned long long (matters for %llu/%llx). */
#define uint64_t unsigned long long

typedef uintptr_t vm_offset_t;
typedef uintptr_t vm_size_t;
typedef uint64_t addr64_t;
typedef uint32_t ppnum_t;
typedef int kern_return_t;
typedef int wait_result_t;
typedef int errno_t;
typedef void *thread_t;
typedef void *pmap_t;
typedef void *vm_map_t;
typedef void (*thread_continue_t)(void *, wait_result_t);
typedef unsigned int boolean_t;

#define KERN_SUCCESS            0
#define THREAD_UNINT            0
#define THREAD_CONTINUE_NULL    ((thread_continue_t)0)
#define NSEC_PER_MSEC           1000000ull
#define TRUE                    1
#define FALSE                   0
#ifndef MIN
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif
#define PAGE_MASK               0x3fffu
#define ptoa_64(x)              ((uint64_t)(x) << 14)
#define KMA_DATA                1
#define KMA_ZERO                2
#define KMA_PERMANENT           4
#define VM_KERN_MEMORY_IOKIT    0
#define __unused                __attribute__((unused))
#define os_atomic_load(p, m)    (*(p))
#define os_atomic_add_orig(p, v, m) __atomic_fetch_add((p), (v), __ATOMIC_RELAXED)

extern vm_map_t kernel_map;
extern pmap_t kernel_pmap;

void delay(int usec);
void clock_interval_to_deadline(uint32_t interval, uint64_t scale, uint64_t *deadline);
int assert_wait_deadline(void *event, int interruptible, uint64_t deadline);
wait_result_t thread_block(thread_continue_t continuation);
kern_return_t kernel_thread_start(thread_continue_t fn, void *arg, thread_t *thread);
void thread_deallocate(thread_t thread);
int PE_parse_boot_argn(const char *name, void *value, int size);
void flush_dcache64(addr64_t pa, unsigned len, boolean_t phys);
kern_return_t kmem_alloc_contig(vm_map_t map, vm_offset_t *addr, vm_size_t size,
    vm_offset_t mask, ppnum_t max_pnum, ppnum_t pnum_mask, int flags, int tag);

/* ---- locks ---- */
typedef struct { int unused; } lck_grp_t, lck_mtx_t;
#define LCK_GRP_ATTR_NULL       NULL
#define LCK_ATTR_NULL           NULL
lck_grp_t *lck_grp_alloc_init(const char *name, void *attr);
lck_mtx_t *lck_mtx_alloc_init(lck_grp_t *grp, void *attr);
void lck_mtx_lock(lck_mtx_t *m);
void lck_mtx_unlock(lck_mtx_t *m);

/* ---- networking KPI (only what sweet_usb.c names) ---- */
typedef struct mock_mbuf *mbuf_t;
typedef struct mock_ifnet *ifnet_t;
typedef struct mock_socket *socket_t;
#define ETHER_ADDR_LEN          6
#define ETHER_HDR_LEN           14
#define ETHER_CRC_LEN           4
#define ETHER_MAX_LEN           1518
#define ETHERMTU                1500
#define MBUF_DONTWAIT           1
#define MBUF_TYPE_DATA          1
#define MBUF_EXT                1
#define AF_INET                 2
#define PF_INET                 2
#define SOCK_DGRAM              2
#define IPPROTO_UDP             17
#define IFF_UP                  0x1
#define IFF_BROADCAST           0x2
#define IFF_RUNNING             0x40
#define IFF_SIMPLEX             0x800
#define IFF_MULTICAST           0x8000
#define IFT_ETHER               6
#define IFNET_FAMILY_ETHERNET   2
#define IFNET_INIT_CURRENT_VERSION 2
#define IFNET_INIT_LEGACY       1
#define SIOCSIFADDR             1
#define SIOCAIFADDR             2
#define SIOCSIFFLAGS            3
#define SIOCADDMULTI            4
#define SIOCDELMULTI            5
#define htonl(x)                __builtin_bswap32(x)
#define htons(x)                __builtin_bswap16(x)

struct ether_header { uint8_t d[6], s[6]; uint16_t t; };
struct in_addr { uint32_t s_addr; };
struct sockaddr_in { uint8_t sin_len, sin_family; uint16_t sin_port; struct in_addr sin_addr; char z[8]; };
struct sockaddr { uint8_t sa_len, sa_family; char sa_data[14]; };
struct ifaliasreq { char ifra_name[16]; struct sockaddr ifra_addr, ifra_broadaddr, ifra_mask; };
struct iovec { void *iov_base; size_t iov_len; };
struct msghdr { void *msg_name; unsigned msg_namelen; struct iovec *msg_iov; int msg_iovlen; };
struct ifnet_stat_increment_param { uint32_t packets_in, bytes_in, errors_in; };
struct ifnet_init_eparams {
	uint32_t ver, len, flags;
	const void *uniqueid; uint32_t uniqueid_len;
	const char *name; uint32_t unit, family, type;
	void *output, *demux, *add_proto, *del_proto, *check_multi, *framer_extended, *ioctl, *detach;
	const uint8_t *broadcast_addr; uint32_t broadcast_len;
};
extern const uint8_t etherbroadcastaddr[ETHER_ADDR_LEN];
int ether_demux(void), ether_add_proto(void), ether_del_proto(void),
    ether_check_multi(void), ether_frameout_extended(void);
errno_t ifnet_allocate_extended(const struct ifnet_init_eparams *init, ifnet_t *ifp);
errno_t ifnet_attach(ifnet_t ifp, const void *ll);
errno_t ifnet_set_addrlen(ifnet_t ifp, int len);
errno_t ifnet_set_hdrlen(ifnet_t ifp, int len);
errno_t ifnet_set_mtu(ifnet_t ifp, int mtu);
errno_t ifnet_set_baudrate(ifnet_t ifp, uint64_t baud);
errno_t ifnet_set_flags(ifnet_t ifp, uint32_t flags, uint32_t mask);
errno_t ifnet_set_lladdr(ifnet_t ifp, const void *ll, size_t len);
errno_t ifnet_input(ifnet_t ifp, mbuf_t m, const struct ifnet_stat_increment_param *s);
errno_t ifnet_stat_increment_out(ifnet_t ifp, uint32_t p, uint32_t b, uint32_t e);
errno_t proto_plumb(int pf, ifnet_t ifp);
errno_t sock_socket(int d, int t, int p, void *cb, void *c, socket_t *so);
errno_t sock_ioctl(socket_t so, unsigned long req, void *arg);
errno_t sock_send(socket_t so, const struct msghdr *msg, int flags, size_t *sent);
void sock_close(socket_t so);
errno_t mbuf_gethdr(int how, int type, mbuf_t *m);
errno_t mbuf_mclget(int how, int type, mbuf_t *m);
int mbuf_flags(mbuf_t m);
size_t mbuf_maxlen(mbuf_t m);
void *mbuf_data(mbuf_t m);
size_t mbuf_len(mbuf_t m);
errno_t mbuf_setlen(mbuf_t m, size_t len);
errno_t mbuf_pkthdr_setlen(mbuf_t m, size_t len);
size_t mbuf_pkthdr_len(mbuf_t m);
errno_t mbuf_pkthdr_setrcvif(mbuf_t m, ifnet_t ifp);
errno_t mbuf_pkthdr_setheader(mbuf_t m, void *h);
void mbuf_pkthdr_adjustlen(mbuf_t m, int amount);
errno_t mbuf_setdata(mbuf_t m, void *data, size_t len);
errno_t mbuf_copydata(mbuf_t m, size_t off, size_t len, void *out);
void mbuf_freem(mbuf_t m);
#endif /* XNU_MOCK_H */
