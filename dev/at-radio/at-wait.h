#ifndef AT_WAIT_H
#define AT_WAIT_H

struct at_wait; /* forward declaration */
typedef char (* at_callback_t)(struct pt *, struct at_wait *, int c);

struct at_wait {
  char *str;   /* String we are matching against */
  at_callback_t callback; /* Callback function when match is complete */
  //int (* match)(struct at_wait *, int c);  /* Match function */
  uint8_t pos; /* How far into the string we have matched so far */ 
};

#define MAXWAIT 6
struct at_wait *at_waitlist2[MAXWAIT];

struct at_wait *at_wait_match; 	/* Latest match */

#define AT_LINE_SIZE 200
char at_line[AT_LINE_SIZE]; /* Where generic callbacks put result */

uint8_t atwait_matching; /* set to true during callback after match */
process_event_t at_match_event;


void
start_at(struct at_wait *at);
void
restart_at(struct at_wait *at);
void
stop_at(struct at_wait *at);

void
atwait_start_atlist(uint8_t permanent, ...);

void
atwait_init();

/* Generic callbacks */
//PT_THREAD(wait_simple_callback(struct pt *pt, struct at_wait *at, int c));
//PT_THREAD(wait_readline_callback(struct pt *pt, struct at_wait *at, int c));
//PT_THREAD(wait_readlines_callback(struct pt *pt, struct at_wait *at, int c));
PT_THREAD(wait_readline_pt(struct pt *pt, struct at_wait *at, int c));
PT_THREAD(wait_readlines_pt(struct pt *pt, struct at_wait *at, int c));

/* Match functions */
int
at_match_byte(struct at_wait *at, int c);
int
at_match_null(struct at_wait *at, int c);

void
atwait_record_on();
void
atwait_record_off();

PT_THREAD(wait_fsm_pt(struct pt *pt, int c));

//PT_THREAD(atwait(struct pt *pt, process_event_t ev,
//                 process_data_t data, struct at_wait **atp, int seconds, ...));
PT_THREAD(atwait(int lineno, struct pt *pt, struct at_wait **atp, int seconds, ...));
PT_THREAD(at_sendbuf(struct pt *pt, unsigned char *buf, size_t len));

#define ATWAIT2(SEC, ...)  {                                                      \
    static struct pt pt;                                                          \
    PT_INIT(&pt);                                                                 \
while (atwait(__LINE__, &pt, &at, SEC, __VA_ARGS__, NULL) < PT_EXITED) { \
      PROCESS_PAUSE();                                                       \
  }                                                                               \
}  

#define PT_ATWAIT2(SEC, ...)  {                                                      \
    static struct pt atpt;                                                          \
    PT_INIT(&atpt);                                                                 \
    while (atwait(__LINE__, &atpt, &at, SEC, __VA_ARGS__, NULL) < PT_EXITED) { \
      PT_YIELD(pt);                                                       \
  }                                                                               \
}  

#define DELAY(SEC)  {                                                      \
    static struct pt pt;                                                          \
    PT_INIT(&pt);                                                                 \
    while (atwait(__LINE__, &pt, &at, SEC, NULL) < PT_EXITED) { \
      PROCESS_PAUSE();                                                       \
  }                                                                               \
}  

#define PT_DELAY(SEC)  {                                                      \
    static struct pt atpt;                                                          \
    PT_INIT(&atpt);                                                                 \
    while (atwait(__LINE__, &atpt, &at, SEC, NULL) < PT_EXITED) { \
      PT_YIELD(pt);                                                       \
  }                                                                               \
}  

#define ATSPAWN(fun, ...)   { \
  static struct pt _pt;       \
  PT_INIT(&_pt); \
  while (fun(&_pt, ##__VA_ARGS__) < PT_EXITED) { \
      PROCESS_PAUSE(); \
    } \
  }

#define PT_ATSPAWN(fun, ...)   { \
  static struct pt _pt;       \
  PT_INIT(&_pt); \
  while (fun(&_pt, ##__VA_ARGS__) < PT_EXITED) { \
      PT_YIELD(pt); \
    } \
  }


#define ATSTR2(str) { \
    printf("-->%s\n", str); \
    ATSPAWN(at_sendbuf, (unsigned char *) str, strlen(str));    \
}

#define PT_ATSTR2(str) {\
    printf("-->%s\n", str); \
    PT_ATSPAWN(at_sendbuf, (unsigned char *) str, strlen(str));    \
  }

#define ATBUF2(buf, len)     ATSPAWN(at_sendbuf, buf, len)
#define PT_ATBUF2(buf, len) PT_ATSPAWN(at_sendbuf, buf, len)

#endif /* AT_WAIT_H */
