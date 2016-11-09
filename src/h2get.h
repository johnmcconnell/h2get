#ifndef H2GET_H_
#define H2GET_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/socket.h>

#define H2GET_HEADERS_SETTINGS_FLAGS_ACK 0x1
#define H2GET_HEADERS_SETTINGS_HEADER_TABLE_SIZE 0x1
#define H2GET_HEADERS_SETTINGS_ENABLE_PUSH 0x2
#define H2GET_HEADERS_SETTINGS_MAX_CONCURRENT_STREAMS 0x3
#define H2GET_HEADERS_SETTINGS_INITIAL_WINDOW_SIZE 0x4
#define H2GET_HEADERS_SETTINGS_MAX_FRAME_SIZE 0x5
#define H2GET_HEADERS_SETTINGS_MAX_HEADER_LIST_SIZE 0x6

#define H2GET_HEADERS_DATA 0x0
#define H2GET_HEADERS_HEADERS 0x1
#define H2GET_HEADERS_PRIORITY 0x2
#define H2GET_HEADERS_RST_STREAM 0x3
#define H2GET_HEADERS_SETTINGS 0x4
#define H2GET_HEADERS_PUSH_PROMISE 0x5
#define H2GET_HEADERS_PING 0x6
#define H2GET_HEADERS_GOAWAY 0x7
#define H2GET_HEADERS_WINDOW_UPDATE 0x8
#define H2GET_HEADERS_CONTINUATION 0x9
#define H2GET_HEADERS_MAX 0xa

#define H2GET_ERR_NO_ERROR 0x0
#define H2GET_ERR_PROTOCOL_ERROR 0x1
#define H2GET_ERR_INTERNAL_ERROR 0x2
#define H2GET_ERR_FLOW_CONTROL_ERROR 0x3
#define H2GET_ERR_SETTINGS_TIMEOUT 0x4
#define H2GET_ERR_STREAM_CLOSED 0x5
#define H2GET_ERR_FRAME_SIZE_ERROR 0x6
#define H2GET_ERR_REFUSED_STREAM 0x7
#define H2GET_ERR_CANCEL 0x8
#define H2GET_ERR_COMPRESSION_ERROR 0x9
#define H2GET_ERR_CONNECT_ERROR 0xa
#define H2GET_ERR_ENHANCE_YOUR_CALM 0xb
#define H2GET_ERR_INADEQUATE_SECURITY 0xc
#define H2GET_ERR_HTTP_1_1_REQUIRED 0xd

#define H2GET_HEADERS_HEADERS_FLAG_END_STREAM 0x1
#define H2GET_HEADERS_HEADERS_FLAG_END_HEADERS 0x4
#define H2GET_HEADERS_HEADERS_FLAG_PADDED 0x8
#define H2GET_HEADERS_HEADERS_FLAG_PRIORITY 0x20

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#include <ctype.h>
#include <stdio.h>
static inline void dump_line(char *data, int offset, int limit)
{
    int i;

    printf("%03x:", offset);
    for (i = 0; i < limit; i++) {
        printf(" %02x", (unsigned char)data[offset + i]);
    }
    for (i = 0; i + limit < 16; i++) {
        printf("   ");
    }
    printf(" ");
    for (i = 0; i < limit; i++) {
        printf("%c", isprint(data[offset + i]) ? data[offset+i]:'.');
    }
    printf("\n");
}

static inline void dump_zone(void *buf, int len)
{
    int i;
    char *data = buf;

    printf("================================================================================\n");
    for (i = 0; i < len; i += 16) {
        int limit;
        limit = 16;
        if (i + limit > len)
            limit = len - i;
        dump_line(data, i, limit);
    }
    printf("================================================================================\n");
}

#define H2GET_DOT_STAR_PRINTF(s) ((int)(s)->len), ((s)->buf)
#define H2GET_STRLIT(s) (s), (sizeof((s)) - 1)
#define H2GET_BUFLIT(s) ((struct h2get_buf){(s), (sizeof((s)) - 1)})
#define H2GET_BUFSTR(s) ((struct h2get_buf){(s), strlen(s)})
#define H2GET_BUF(b, l) ((struct h2get_buf){(void *)b, l})
#define H2GET_BUF_NULL ((struct h2get_buf){})
#define H2GET_TO_STR_ALLOCA(b) ({ \
        char *ret_ = alloca((b).len + 1); \
        memcpy(ret_, (b).buf, (b).len); \
        ret_[(b).len] = '\0'; \
        ret_;})

struct h2get_buf {
    char *buf;
    size_t len;
};

static inline int h2get_buf_printf(struct h2get_buf *out, char *fmt, ...)
{
    int ret;
    size_t newsz;
    char *p;
    va_list a1, a2;
    va_start(a1, fmt);
    va_copy(a2, a1);

    ret = vsnprintf(NULL, 0, fmt, a1);
    if (ret < 0) {
        return ret;
    }
    newsz= out->len + ret + 1;
    p = realloc(out->buf, newsz);
    if (!p) {
        return -1;
    }
    out->buf = p;
    ret = vsprintf(out->buf + out->len, fmt, a2);
    out->len = newsz;
    return ret;
}

enum h2get_conn_state {
    H2GET_CONN_STATE_INIT,
    H2GET_CONN_STATE_CONNECT,
};

struct h2get_conn {
    int fd;
    int protocol;
    int socktype;
    struct {
        struct sockaddr_storage sa_storage;
        socklen_t len;
        struct sockaddr *sa;
    } sa;
    enum h2get_conn_state state;
    struct h2get_buf servername;
    void *priv;
};

enum h2get_transport {
    H2GET_TRANSPORT_UNIX,
    H2GET_TRANSPORT_PLAIN,
    H2GET_TRANSPORT_SSL,
};

struct h2get_ops {
    enum h2get_transport xprt;
    void *(*init)(void);
    int (*connect)(struct h2get_conn *, void *);
    int (*write)(struct h2get_conn *, struct h2get_buf *bufs, size_t nr_bufs);
    int (*read)(struct h2get_conn *, struct h2get_buf *buf, int tout);
    int (*close)(struct h2get_conn *, void *);
    void (*fini)(void *);
};

static inline void *memdup(void *src, size_t len)
{
    void *dst = malloc(len);
    memcpy(dst, src, len);
    return dst;
}

extern struct h2get_ops plain_ops;
extern struct h2get_ops ssl_ops;

struct list {
    struct list *next, *prev;
};

static inline int list_empty(struct list *l)
{
	return l->next == l;
}
static inline void list_init(struct list *n)
{
	n->next = n;
	n->prev = n;
}

static inline void list_del(struct list *n)
{
	n->next->prev = n->prev;
	n->prev->next = n->next;
}

static inline struct list *list_tail(struct list *l)
{
    return l->prev;
}
static inline void list_add(struct list *l, struct list *n)
{
	n->next = l->next;
	n->next->prev = n;
	l->next = n;
	n->prev = l;
}

static inline void list_add_tail(struct list *l, struct list *n)
{
	list_add(l->prev, n);
}

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({			\
		const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) );})

uint8_t *read_bits(uint8_t *buf, uint8_t nr_bits, uint32_t *value, uint8_t *offset);

struct h2get_h2_header {
    unsigned int len : 24;
    unsigned int type : 8;
    unsigned int flags : 8;
    unsigned int r : 1;
    unsigned int stream_id : 31;
    char payload[0];
} __attribute__((packed));

struct h2get_h2_setting {
    uint16_t id;
    uint32_t value;
} __attribute__((packed));

struct h2get_h2_window_update {
    unsigned int reserved : 1;
    unsigned int increment : 31;
} __attribute__((packed));

struct h2get_h2_priority {
    unsigned int exclusive : 1;
    unsigned int dep_stream_id : 31;
    unsigned int weight : 8;
} __attribute__((packed));

struct h2get_h2_goaway {
    unsigned int reserved : 1;
    unsigned int last_stream_id : 31;
    uint32_t error_code;
    char additional_debug_data[];
} __attribute__((packed));


struct h2get_ctx;
void h2get_ctx_init(struct h2get_ctx *ctx);
void h2get_ctx_on_settings_ack(struct h2get_ctx *ctx);
struct h2get_h2_settings;
int h2get_ctx_on_peer_settings(struct h2get_ctx *ctx, struct h2get_h2_header *h, char *payload, int plen);
int h2get_connect(struct h2get_ctx *ctx, struct h2get_buf url_buf, const char **err);
int h2get_close(struct h2get_ctx *ctx);
int h2get_send_priority(struct h2get_ctx *ctx, uint32_t stream_id, struct h2get_h2_priority *prio, const char **err);
int h2get_send_settings(struct h2get_ctx *ctx, const char **err);
int h2get_send_prefix(struct h2get_ctx *ctx, const char **err);
int h2get_send_windows_update(struct h2get_ctx *ctx, uint32_t stream_id, uint32_t increment, const char **err);
int h2get_get(struct h2get_ctx *ctx, const char *path, const char **err);
int h2get_getp(struct h2get_ctx *ctx, const char *path, uint32_t sid, struct h2get_h2_priority prio, const char **err);
const char *h2get_render_error_code(uint32_t err);

#endif /* H2GET_H_ */
/* vim: set expandtab ts=4 sw=4: */
