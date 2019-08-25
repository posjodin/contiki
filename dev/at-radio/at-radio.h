#ifndef AT_RADIO_A6_H
#define AT_RADIO_A6_H


#define AT_RADIO_MAX_SEND_LEN 1024
//#define AT_RADIO_MAX_SEND_LEN 512
#define AT_RADIO_MAX_RECV_LEN 1024

/* How long to wait for APN registration (sec) */
#define AT_RADIO_APN_REGISTER_TIMEOUT 180
/* How long to wait between each attempt (sec) */
#define AT_RADIO_APN_REGISTER_REATTEMPT 10

#define AT_RADIO_DEBUG

typedef enum {
  AT_RADIO_CONN_CONNECTED,
  AT_RADIO_CONN_SOCKET_CLOSED,
  AT_RADIO_CONN_SOCKET_TIMEDOUT,
  AT_RADIO_CONN_ABORTED,
  AT_RADIO_CONN_DATA_SENT
} at_radio_conn_event_t;

#define AT_RADIO_MAX_APN_LEN 32
struct at_radio_context {
  uint8_t active; /* Context active? */
  char *pdptype; /* allowed values: "IP" and "IPV6" */
  char apn[AT_RADIO_MAX_APN_LEN+1];
};

struct at_radio_connection; /* Forward declaration */
typedef int (* at_radio_callback_t)(void *, int);
typedef void (* at_radio_data_callback_t)(struct at_radio_connection *at_radioconn,
                                      void *callback_arg,
                                      const uint8_t *input_data_ptr,
                                      int input_data_len);
typedef void (* at_radio_event_callback_t)(void *callback_arg,
                                       at_radio_conn_event_t event);

#define AT_RADIO_MAX_CONNECTION 1
struct at_radio_connection {
  struct at_radio_context *context;
  uint8_t reserved;
  uint8_t connectionid;
  const char *proto;
  const char *host;
  uip_ipaddr_t ipaddr;
  uint16_t port;
  void *callback_arg;
  at_radio_data_callback_t input_callback;
  at_radio_event_callback_t event_callback;
  uint8_t *input_data_ptr;
  uint8_t *output_data_ptr;

  uint16_t input_data_len;
  uint16_t output_data_len;
};
#define AT_RADIO_CONNECTION_RESERVED(GC) ((GC)->reserved)
#define AT_RADIO_CONNECTION_RELEASE(GC) (GC)->reserved = 0
#define AT_RADIO_CONNECTION_RESERVE(GC) (GC)->reserved = 1    

struct at_radio_status {
  enum {
    AT_RADIO_STATE_NONE,
    AT_RADIO_STATE_IDLE,
    AT_RADIO_STATE_REGISTERED,
    AT_RADIO_STATE_ACTIVE
  } state;

#if NETSTACK_CONF_WITH_IPV6
  char ipaddr[sizeof("::ffff:255.255.255.255")];
#else
  char ipaddr[sizeof("255.255.255.255")];  
#endif /* NETSTACK_CONF_WITH_IPV6 */
  uip_ip4addr_t ip4addr;
  uip_ip4addr_t ip4mask;  
  uip_ipaddr_t uipaddr;
  int8_t rssi;

  enum int8_t {
    AT_RADIO_MODULE_UNNKOWN = -1,
    AT_RADIO_MODULE_A6 = 6,
    AT_RADIO_MODULE_A7,
    AT_RADIO_MODULE_SIM7020E
  } module;

  double longi;
  double lat;
  double speed;
  double course;
};

struct at_radio_status status;

struct at_radio_statistics {
  unsigned int at_timeouts;
  unsigned int at_errors;
  unsigned int at_retries;
  unsigned int resets;
  unsigned int connections;     /* Connections succeeded */
  unsigned int connfailed;      /* Connections failed */
} at_radio_statistics;

process_event_t at_radio_ev_init;
process_event_t at_radio_ev_connection;
process_event_t at_radio_ev_send;
process_event_t at_radio_ev_close;
process_event_t at_radio_ev_datamode;

void at_radio_enqueue_event(process_event_t ev, void *data);
struct at_radio_event * at_radio_dequeue_event();
void at_radio_init();
struct at_radio_connection *alloc_at_radio_connection();
struct at_radio_connection *find_at_radio_connection(char connectionid);
int at_radio_set_context(struct at_radio_context *gcontext, char *pdptype, char *apn);
int at_radio_register(struct at_radio_connection *gconn,
                      void *callback_arg,
                      at_radio_data_callback_t input_callback,
                      at_radio_event_callback_t event_callback);
int at_radio_unregister(struct at_radio_connection *gconn);
struct at_radio_connection * at_radio_connection(struct at_radio_connection *at_radioconn,
                                                 const char *proto, const uip_ipaddr_t *ipaddr,
                                                 uint16_t port);
void at_radio_send(struct at_radio_connection *at_radioconn);
void at_radio_close(struct at_radio_connection *gconn);
void at_radio_datamode(struct at_radio_connection *gconn);
void at_radio_call_event(struct at_radio_connection *at_radioconn, at_radio_conn_event_t event);
struct at_radio_status *at_radio_status();

/* 
 * Radio module functions 
 */
void
at_radio_module_init();
size_t
at_radio_sendbuf(uint8_t *buf, size_t len);
PT_THREAD(read_csq(struct pt *pt));
PT_THREAD(init_module(struct pt *pt));
PT_THREAD(apn_register(struct pt *pt));
PT_THREAD(apn_activate(struct pt *pt));
PT_THREAD(get_moduleinfo(struct pt *pt));
PT_THREAD(get_ipconfig(struct pt *pt));
PT_THREAD(at_radio_connect_pt(struct pt *pt, struct at_radio_connection *at_radioconn));
PT_THREAD(at_radio_send_pt(struct pt *pt, struct at_radio_connection * at_radioconn));
PT_THREAD(at_radio_close_pt(struct pt *pt, struct at_radio_connection * at_radioconn));
PT_THREAD(at_radio_datamode_pt(struct pt *pt, struct at_radio_connection * at_radioconn));
#endif /* AT_RADIO_A6_H */
