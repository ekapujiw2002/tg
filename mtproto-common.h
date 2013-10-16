#ifndef __MTPROTO_COMMON_H__
#define __MTPROTO_COMMON_H__

#include <string.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/aes.h>
#include <stdio.h>

#include "interface.h"
#include "constants.h"
/* DH key exchange protocol data structures */
#define	CODE_req_pq			0x60469778
#define CODE_resPQ			0x05162463
#define CODE_req_DH_params		0xd712e4be
#define CODE_p_q_inner_data		0x83c95aec
#define CODE_server_DH_inner_data	0xb5890dba
#define CODE_server_DH_params_fail	0x79cb045d
#define CODE_server_DH_params_ok	0xd0e8075c
#define CODE_set_client_DH_params	0xf5045f1f
#define CODE_client_DH_inner_data	0x6643b654
#define CODE_dh_gen_ok			0x3bcbf734
#define CODE_dh_gen_retry		0x46dc1fb9
#define CODE_dh_gen_fail		0xa69dae02 

/* service messages */
#define CODE_rpc_result			0xf35c6d01
#define CODE_rpc_error			0x2144ca19
#define CODE_msg_container		0x73f1f8dc
#define CODE_msg_copy			0xe06046b2
#define CODE_msgs_ack			0x62d6b459
#define CODE_bad_msg_notification	0xa7eff811
#define	CODE_bad_server_salt		0xedab447b
#define CODE_msgs_state_req		0xda69fb52
#define CODE_msgs_state_info		0x04deb57d
#define CODE_msgs_all_info		0x8cc0d131
#define CODE_new_session_created	0x9ec20908
#define CODE_msg_resend_req		0x7d861a08
#define CODE_ping			0x7abe77ec
#define CODE_pong			0x347773c5
#define CODE_destroy_session		0xe7512126
#define CODE_destroy_session_ok		0xe22045fc
#define CODE_destroy_session_none      	0x62d350c9
#define CODE_destroy_sessions		0x9a6face8
#define CODE_destroy_sessions_res	0xa8164668
#define	CODE_get_future_salts		0xb921bd04
#define	CODE_future_salt		0x0949d9dc
#define	CODE_future_salts		0xae500895
#define	CODE_rpc_drop_answer		0x58e4a740
#define CODE_rpc_answer_unknown		0x5e2ad36e
#define CODE_rpc_answer_dropped_running	0xcd78e586
#define CODE_rpc_answer_dropped		0xa43ad8b7
#define	CODE_msg_detailed_info		0x276d3ec6
#define	CODE_msg_new_detailed_info	0x809db6df
#define CODE_ping_delay_disconnect	0xf3427b8c
#define CODE_gzip_packed 0x3072cfa1


/* not really a limit, for struct encrypted_message only */
// #define MAX_MESSAGE_INTS	16384
#define MAX_MESSAGE_INTS	1048576
#define MAX_PROTO_MESSAGE_INTS	1048576

#pragma pack(push,4)
struct encrypted_message {
  // unencrypted header
  long long auth_key_id;
  char msg_key[16];
  // encrypted part, starts with encrypted header
  long long server_salt;
  long long session_id;
  // long long auth_key_id2; // removed
  // first message follows
  long long msg_id;
  int seq_no;
  int msg_len;   // divisible by 4
  int message[MAX_MESSAGE_INTS];
};

struct worker_descr {
  int addr;
  int port;
  int pid;
  int start_time;
  int id;
};

struct rpc_ready_packet {
  int len;
  int seq_num;
  int type;
  struct worker_descr worker;
  int worker_ready_cnt; 
  int crc32;
};


struct front_descr {
  int addr;
  int port;
  int pid;
  int start_time;
  int id;
};

struct rpc_front_packet {
  int len;
  int seq_num;
  int type;
  struct front_descr front;
  long long hash_mult;
  int rem, mod;
  int crc32;
};

struct middle_descr {
  int addr;
  int port;
  int pid;
  int start_time;
  int id;
};

struct rpc_front_ack {
  int len;
  int seq_num;
  int type;
  struct middle_descr middle;
  int crc32;
};

struct rpc_front_err {
  int len;
  int seq_num;
  int type;
  int errcode;
  struct middle_descr middle;
  long long hash_mult;
  int rem, mod;
  int crc32;
};

struct rpc_proxy_req {
  int len;
  int seq_num;
  int type;
  int flags;
  long long ext_conn_id;
  unsigned char remote_ipv6[16];
  int remote_port;
  unsigned char our_ipv6[16];
  int our_port;
  int data[];
};

#define	PROXY_HDR(__x)	((struct rpc_proxy_req *)((__x) - offsetof(struct rpc_proxy_req, data)))

struct rpc_proxy_ans {
  int len;
  int seq_num;
  int type;
  int flags;	// +16 = small error packet, +8 = flush immediately
  long long ext_conn_id;
  int data[];
};

struct rpc_close_conn {
  int len;
  int seq_num;
  int type;
  long long ext_conn_id;
  int crc32;
};

struct rpc_close_ext {
  int len;
  int seq_num;
  int type;
  long long ext_conn_id;
  int crc32;
};

struct rpc_simple_ack {
  int len;
  int seq_num;
  int type;
  long long ext_conn_id;
  int confirm_key;
  int crc32;
};

#pragma pack(pop)

BN_CTX *BN_ctx;

void prng_seed (const char *password_filename, int password_length);
int serialize_bignum (BIGNUM *b, char *buffer, int maxlen);
long long compute_rsa_key_fingerprint (RSA *key);

#define PACKET_BUFFER_SIZE	(16384 * 100) // temp fix
int packet_buffer[PACKET_BUFFER_SIZE], *packet_ptr;

static inline void out_ints (int *what, int len) {
  assert (packet_ptr + len <= packet_buffer + PACKET_BUFFER_SIZE);
  memcpy (packet_ptr, what, len * 4);
  packet_ptr += len;
}


static inline void out_int (int x) {
  assert (packet_ptr + 1 <= packet_buffer + PACKET_BUFFER_SIZE);
  *packet_ptr++ = x;
}


static inline void out_long (long long x) {
  assert (packet_ptr + 2 <= packet_buffer + PACKET_BUFFER_SIZE);
  *(long long *)packet_ptr = x;
  packet_ptr += 2;
}

static inline void clear_packet (void) {
  packet_ptr = packet_buffer;
}

void out_cstring (const char *str, long len);
void out_cstring_careful (const char *str, long len);
void out_data (const char *data, long len);

static inline void out_string (const char *str) {
  out_cstring (str, strlen (str));
}

static inline void out_bignum (BIGNUM *n) {
  int l = serialize_bignum (n, (char *)packet_ptr, (PACKET_BUFFER_SIZE - (packet_ptr - packet_buffer)) * 4);
  assert (l > 0);
  packet_ptr += l >> 2;
}

extern int *in_ptr, *in_end;

static inline int prefetch_strlen (void) {
  if (in_ptr >= in_end) { 
    return -1; 
  }
  unsigned l = *in_ptr;
  if ((l & 0xff) < 0xfe) { 
    l &= 0xff;
    return (in_end >= in_ptr + (l >> 2) + 1) ? (int)l : -1;
  } else if ((l & 0xff) == 0xfe) {
    l >>= 8;
    return (l >= 254 && in_end >= in_ptr + ((l + 7) >> 2)) ? (int)l : -1;
  } else {
    return -1;
  }
}

extern int verbosity;
static inline char *fetch_str (int len) {
  assert (len >= 0);
  if (verbosity > 6) {
    logprintf ("fetch_string: len = %d\n", len);
  }
  if (len < 254) {
    char *str = (char *) in_ptr + 1;
    in_ptr += 1 + (len >> 2);
    return str;
  } else {
    char *str = (char *) in_ptr + 4;
    in_ptr += (len + 7) >> 2;
    return str;
  }
} 

static inline char *fetch_str_dup (void) {
  int l = prefetch_strlen ();
  char *s = malloc (l + 1);
  memcpy (s, fetch_str (l), l);
  s[l] = 0;
  return s;
}

static __inline__ unsigned long long rdtsc(void) {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static inline long have_prefetch_ints (void) {
  return in_end - in_ptr;
}

int fetch_bignum (BIGNUM *x);

static inline int fetch_int (void) {
  if (verbosity > 6) {
    logprintf ("fetch_int: 0x%08x (%d)\n", *in_ptr, *in_ptr);
  }
  return *(in_ptr ++);
}

static inline int prefetch_int (void) {
  return *(in_ptr);
}

static inline long long fetch_long (void) {
  long long r = *(long long *)in_ptr;
  in_ptr += 2;
  return r;
}

static inline double fetch_double (void) {
  double r = *(double *)in_ptr;
  in_ptr += 2;
  return r;
}

static inline void fetch_ints (void *data, int count) {
  memcpy (data, in_ptr, 4 * count);
  in_ptr += count;
}

int get_random_bytes (void *buf, int n);

int pad_rsa_encrypt (char *from, int from_len, char *to, int size, BIGNUM *N, BIGNUM *E);
int pad_rsa_decrypt (char *from, int from_len, char *to, int size, BIGNUM *N, BIGNUM *D);

extern long long rsa_encrypted_chunks, rsa_decrypted_chunks;

extern unsigned char aes_key_raw[32], aes_iv[32];
extern AES_KEY aes_key;

void init_aes_unauth (const char server_nonce[16], const char hidden_client_nonce[32], int encrypt);
void init_aes_auth (char auth_key[192], char msg_key[16], int encrypt);
int pad_aes_encrypt (char *from, int from_len, char *to, int size);
int pad_aes_decrypt (char *from, int from_len, char *to, int size);

static inline void hexdump_in (void) {
  hexdump (in_ptr, in_end);
}
#endif