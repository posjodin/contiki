/*
 * Copyright (c) 2001, Adam Dunkels.
 * Copyright (c) 2009, 2010 Joakim Eriksson, Niclas Finne, Dogan Yazar.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

 /* Below define allows importing saved output into Wireshark as "Raw IP" packet type */
#define WIRESHARK_IMPORT_FORMAT 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <err.h>

#include "tools-utils.h"

#ifndef BAUDRATE
#define BAUDRATE B115200
#endif
speed_t b_rate = BAUDRATE;

int verbose = 1;
const char *ipaddr;
const char *netmask;
int slipfd = 0;
int tcpfd = 0;
int clientfd = 0;

uint16_t basedelay=0,delaymsec=0;
uint32_t startsec,startmsec,delaystartsec,delaystartmsec;
int timestamp = 0, flowcontrol=0, showprogress=0, flowcontrol_xonxoff=0;

int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));
void write_to_serial(int outfd, void *inbuf, int len);

void slip_send(int fd, unsigned char c);
void slip_send_char(int fd, unsigned char c);

#define PROGRESS(s) if(showprogress) fprintf(stderr, s)

char tundev[1024] = { "" };

/* IPv6 required minimum MTU */
#define MIN_DEVMTU 1500
int devmtu = MIN_DEVMTU;

int
ssystem(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

int
ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  printf("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

#define SLIP_END      0300
#define SLIP_ESC      0333
#define SLIP_ESC_END  0334
#define SLIP_ESC_ESC  0335

#define SLIP_ESC_XON  0336
#define SLIP_ESC_XOFF 0337
#define XON           17
#define XOFF          19

/* get sockaddr, IPv4 or IPv6: */
void *
get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void
stamptime(void)
{
  static long startsecs=0,startmsecs=0;
  long secs,msecs;
  struct timeval tv;
  time_t t;
  struct tm *tmp;
  char timec[20];

  gettimeofday(&tv, NULL) ;
  msecs=tv.tv_usec/1000;
  secs=tv.tv_sec;
  if (startsecs) {
    secs -=startsecs;
    msecs-=startmsecs;
    if (msecs<0) {secs--;msecs+=1000;}
    fprintf(stderr,"%04lu.%03lu ", secs, msecs);
  } else {
    startsecs=secs;
    startmsecs=msecs;
    t=time(NULL);
    tmp=localtime(&t);
    strftime(timec,sizeof(timec),"%T",tmp);
//    fprintf(stderr,"\n%s.%03lu ",timec,msecs);
    fprintf(stderr,"\n%s ",timec);
  }
}

int
is_sensible_string(const unsigned char *s, int len)
{
  int i;
  for(i = 1; i < len; i++) {
    if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
      continue;
    } else if(s[i] < ' ' || '~' < s[i]) {
      return 0;
    }
  }
  return 1;
}

/*
 * Read from serial, when we have a packet write it to tun. No output
 * buffering, input buffered by stdio.
 */
void
tcp_to_tun(int intcp, int outfd)
{
  static union {
    unsigned char inbuf[2000];
  } uip;
  static int inbufptr = 0;
  int ret,i;
  unsigned char c;

  ret = read(intcp, uip.inbuf, sizeof(uip.inbuf));
  if(ret == -1 || ret == 0) err(1, "tcp_to_tun: read");
  if(ret == -1) {
    err(1, "tcp_to_tun: read");
  }
  if(ret == 0) {
    return;
  }
  {
    int i;
    for (i = 0; i < ret; i++) {
      printf(" %02x", uip.inbuf[i]);
    }
    printf("\n");
  }
  printf("Write %d bytes to tun\n", ret);
  if(write(outfd, uip.inbuf, ret) != ret) {
    err(1, "tcp_to_tun: write");
  }
}

unsigned char slip_buf[2000];
int slip_end, slip_begin;

void
slip_send_char(int fd, unsigned char c)
{
  switch(c) {
  case SLIP_END:
    slip_send(fd, SLIP_ESC);
    slip_send(fd, SLIP_ESC_END);
    break;
  case SLIP_ESC:
    slip_send(fd, SLIP_ESC);
    slip_send(fd, SLIP_ESC_ESC);
    break;
  case XON:
    if(flowcontrol_xonxoff) {
      slip_send(fd, SLIP_ESC);
      slip_send(fd, SLIP_ESC_XON);
    } else {
      slip_send(fd, c);
    }
    break;
  case XOFF:
    if(flowcontrol_xonxoff) {
      slip_send(fd, SLIP_ESC);
      slip_send(fd, SLIP_ESC_XOFF);
    } else {
      slip_send(fd, c);
    }
    break;
  default:
    slip_send(fd, c);
    break;
  }
}

void
slip_send(int fd, unsigned char c)
{
  if(slip_end >= sizeof(slip_buf)) {
    err(1, "slip_send overflow");
  }
  slip_buf[slip_end] = c;
  slip_end++;
}

int
slip_empty()
{
  return slip_end == 0;
}

void
slip_flushbuf(int fd)
{
  int n;

  if(slip_empty()) {
    return;
  }

  n = write(fd, slip_buf + slip_begin, (slip_end - slip_begin));

  if(n == -1 && errno != EAGAIN) {
    err(1, "slip_flushbuf write failed");
  } else if(n == -1) {
    PROGRESS("Q");		/* Outqueueis full! */
  } else {
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
    }
  }
}

void
write_to_serial(int outfd, void *inbuf, int len)
{
  u_int8_t *p = inbuf;
  int i;

  if(verbose>2) {
    if (timestamp) stamptime();
    printf("Packet from TUN of length %d - write SLIP\n", len);
    if (verbose>4) {
#if WIRESHARK_IMPORT_FORMAT
      printf("0000");
	  for(i = 0; i < len; i++) printf(" %02x", p[i]);
#else
      printf("         ");
      for(i = 0; i < len; i++) {
        printf("%02x", p[i]);
        if((i & 3) == 3) printf(" ");
        if((i & 15) == 15) printf("\n         ");
      }
#endif
      printf("\n");
    }
  }

  /* It would be ``nice'' to send a SLIP_END here but it's not
   * really necessary.
   */
  /* slip_send(outfd, SLIP_END); */

  for(i = 0; i < len; i++) {
    switch(p[i]) {
    case SLIP_END:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_END);
      break;
    case SLIP_ESC:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_ESC);
      break;
    case XON:
      if(flowcontrol_xonxoff) {
        slip_send(outfd, SLIP_ESC);
        slip_send(outfd, SLIP_ESC_XON);
      } else {
        slip_send(outfd, p[i]);
      }
      break;
    case XOFF:
      if(flowcontrol_xonxoff) {
        slip_send(outfd, SLIP_ESC);
        slip_send(outfd, SLIP_ESC_XOFF);
      } else {
        slip_send(outfd, p[i]);
      }
      break;
    default:
      slip_send(outfd, p[i]);
      break;
    }
  }
  slip_send(outfd, SLIP_END);
  PROGRESS("t");
}


/*
 * Read from tun, write to slip.
 */
int
tun_to_tcp(int infd, int outfd)
{
  struct {
    unsigned char inbuf[2000];
  } uip;
  int size;
  int i;
  
  if((size = read(infd, uip.inbuf, 2000)) == -1) err(1, "tun_to_tcp: read");
  printf("Read %d bytes frmo tun\n", size);
  for (i = 0; i < size; i++)
    printf(" %02x", uip.inbuf[i]);
  printf("\n");
  if (size != write(outfd, uip.inbuf, size)) err(1, "to_to_tcp: write");
  return size;
}

void
stty_telos(int fd)
{
  struct termios tty;
  speed_t speed = b_rate;
  int i;

  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");

  if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

  cfmakeraw(&tty);

  /* Nonblocking read. */
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  if (flowcontrol)
    tty.c_cflag |= CRTSCTS;
  else
    tty.c_cflag &= ~CRTSCTS;
  tty.c_iflag &= ~IXON;
  if(flowcontrol_xonxoff) {
    tty.c_iflag |= IXOFF | IXANY;
  } else {
    tty.c_iflag &= ~IXOFF & ~IXANY;
  }
  tty.c_cflag &= ~HUPCL;
  tty.c_cflag &= ~CLOCAL;

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

#if 1
  /* Nonblocking read and write. */
  /* if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) err(1, "fcntl"); */

  tty.c_cflag |= CLOCAL;
  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

  i = TIOCM_DTR;
  if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

  usleep(10*1000);		/* Wait for hardware 10ms. */

  /* Flush input and output buffers. */
  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}

int
devopen(const char *dev, int flags)
{
  char t[1024];
  strcpy(t, "/dev/");
  strncat(t, dev, sizeof(t) - 5);
  return open(t, flags);
}

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>

int
tun_alloc(char *dev, int tap)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    perror("can not open /dev/net/tun");
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = (tap ? IFF_TAP : IFF_TUN) | IFF_NO_PI;
  if(*dev != 0)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
    close(fd);
    fprintf(stderr, "can not tunsetiff to %s (flags=%08x): %s\n", dev, ifr.ifr_flags,
            strerror(errno));
    return err;
  }

  /* get resulting tunnel name */
  strcpy(dev, ifr.ifr_name);
  return fd;
}
#else
int
tun_alloc(char *dev, int tap)
{
  return devopen(dev, O_RDWR);
}
#endif

void
cleanup(void)
{
#ifndef __APPLE__
  if (timestamp) stamptime();
  ssystem("ifconfig %s down", tundev);
#ifndef linux
  ssystem("sysctl -w net.ipv6.conf.all.forwarding=1");
#endif
  /* ssystem("arp -d %s", ipaddr); */
  if (timestamp) stamptime();
  ssystem("netstat -nr"
	  " | awk '{ if ($2 == \"%s\") print \"route delete -net \"$1; }'"
	  " | sh",
	  tundev);
#else
  {
    char *  itfaddr = strdup(ipaddr);
    char *  prefix = index(itfaddr, '/');
    if (timestamp) stamptime();
    ssystem("ifconfig %s inet6 %s remove", tundev, ipaddr);
    if (timestamp) stamptime();
    ssystem("ifconfig %s down", tundev);
    if ( prefix != NULL ) *prefix = '\0';
    ssystem("route delete -inet6 %s", itfaddr);
    free(itfaddr);
  }
#endif
}

void
sigcleanup(int signo)
{
  fprintf(stderr, "signal %d\n", signo);
  exit(0);			/* exit(0) will call cleanup() */
}

static int got_sigalarm;

void
sigalarm(int signo)
{
  got_sigalarm = 1;
  return;
}

void
sigalarm_reset()
{
#ifdef linux
#define TIMEOUT (997*1000)
#else
#define TIMEOUT (2451*1000)
#endif
  ualarm(TIMEOUT, TIMEOUT);
  got_sigalarm = 0;
}

void
ifconf(const char *tundev, const char *ipaddr)
{
#ifdef linux
  if (timestamp) stamptime();
  ssystem("ifconfig %s inet `hostname` mtu %d up", tundev, devmtu);
  if (timestamp) stamptime();
  ssystem("ifconfig %s add %s", tundev, ipaddr);

/* radvd needs a link local address for routing */
#if 0
/* fe80::1/64 is good enough */
  ssystem("ifconfig %s add fe80::1/64", tundev);
#elif 1
/* Generate a link local address a la sixxs/aiccu */
/* First a full parse, stripping off the prefix length */
  {
    char lladdr[40];
    char c, *ptr=(char *)ipaddr;
    uint16_t digit,ai,a[8],cc,scc,i;
    for(ai=0; ai<8; ai++) {
      a[ai]=0;
    }
    ai=0;
    cc=scc=0;
    while(c=*ptr++) {
      if(c=='/') break;
      if(c==':') {
	if(cc)
	  scc = ai;
	cc = 1;
	if(++ai>7) break;
      } else {
	cc=0;
	digit = c-'0';
	if (digit > 9)
	  digit = 10 + (c & 0xdf) - 'A';
	a[ai] = (a[ai] << 4) + digit;
      }
    }
    /* Get # elided and shift what's after to the end */
    cc=8-ai;
    for(i=0;i<cc;i++) {
      if ((8-i-cc) <= scc) {
	a[7-i] = 0;
      } else {
	a[7-i] = a[8-i-cc];
	a[8-i-cc]=0;
      }
    }
    sprintf(lladdr,"fe80::%x:%x:%x:%x",a[1]&0xfefd,a[2],a[3],a[7]);
    if (timestamp) stamptime();
    ssystem("ifconfig %s add %s/64", tundev, lladdr);
  }
#endif /* link local */
#elif defined(__APPLE__)
  {
	char * itfaddr = strdup(ipaddr);
	char * prefix = index(itfaddr, '/');
	if ( prefix != NULL ) {
		*prefix = '\0';
		prefix++;
	} else {
		prefix = "64";
	}
    if (timestamp) stamptime();
    ssystem("ifconfig %s inet6 mtu %d up", tundev, devmtu);
    if (timestamp) stamptime();
    ssystem("ifconfig %s inet6 %s add", tundev, ipaddr );
    if (timestamp) stamptime();
    ssystem("sysctl -w net.inet6.ip6.forwarding=1");
    free(itfaddr);
  }
#else
  if (timestamp) stamptime();
  ssystem("ifconfig %s inet `hostname` %s mtu %d up", tundev, ipaddr, devmtu);
  if (timestamp) stamptime();
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  if (timestamp) stamptime();
  ssystem("ifconfig %s\n", tundev);
}

int
main(int argc, char **argv)
{
  int c;
  int tunfd, maxfd;
  int ret;
  fd_set rset, wset;
  FILE *inslip;
  const char *siodev = NULL;
  const char *host = NULL;
  const char *port = NULL;
  int portno = 0;
  const char *prog;
  int baudrate = -2;
  int ipa_enable = 0;
  int tap = 0;
  slipfd = 0;
  struct sockaddr_in listenaddr;
  
  prog = argv[0];
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  while((c = getopt(argc, argv, "B:HILPhXM:s:t:v::d::a:p:T")) != -1) {
    switch(c) {
    case 'B':
      baudrate = atoi(optarg);
      break;

    case 'H':
      flowcontrol=1;
      break;

    case 'X':
      flowcontrol_xonxoff=1;
      break;

    case 'L':
      timestamp=1;
      break;

    case 'M':
      devmtu=atoi(optarg);
      if(devmtu < MIN_DEVMTU) {
        devmtu = MIN_DEVMTU;
      }

    case 'P':
      showprogress=1;
      break;

    case 's':
      if(strncmp("/dev/", optarg, 5) == 0) {
	siodev = optarg + 5;
      } else {
	siodev = optarg;
      }
      break;

    case 'I':
      ipa_enable = 1;
      fprintf(stderr, "Will inquire about IP address using IPA=\n");
      break;

    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
	strncpy(tundev, optarg + 5, sizeof(tundev));
      } else {
	strncpy(tundev, optarg, sizeof(tundev));
      }
      break;

    case 'a':
      host = optarg;
      break;

    case 'p':
      portno = atoi(optarg);
      break;

    case 'd':
      basedelay = 10;
      if (optarg) basedelay = atoi(optarg);
      break;

    case 'v':
      verbose = 2;
      if (optarg) verbose = atoi(optarg);
      break;

    case 'T':
      tap = 1;
      break;

    case '?':
    case 'h':
    default:
      fprintf(stderr,"usage:  %s [options] ipaddress\n", prog);
      fprintf(stderr,"example: tunslip6 -L -v2 -s ttyUSB1 fd00::1/64\n");
      fprintf(stderr,"Options are:\n");
#ifndef __APPLE__
      fprintf(stderr," -B baudrate    9600,19200,38400,57600,115200 (default),230400,460800,921600\n");
#else
      fprintf(stderr," -B baudrate    9600,19200,38400,57600,115200 (default),230400\n");
#endif
      fprintf(stderr," -H             Hardware CTS/RTS flow control (default disabled)\n");
      fprintf(stderr," -I             Inquire IP address\n");
      fprintf(stderr," -X             Software XON/XOFF flow control (default disabled)\n");
      fprintf(stderr," -L             Log output format (adds time stamps)\n");
      fprintf(stderr," -s siodev      Serial device (default /dev/ttyUSB0)\n");
      fprintf(stderr," -M             Interface MTU (default and min: 1280)\n");
      fprintf(stderr," -T             Make tap interface (default is tun interface)\n");
      fprintf(stderr," -t tundev      Name of interface (default tap0 or tun0)\n");
      fprintf(stderr," -v[level]      Verbosity level\n");
      fprintf(stderr,"    -v0         No messages\n");
      fprintf(stderr,"    -v1         Encapsulated SLIP debug messages (default)\n");
      fprintf(stderr,"    -v2         Printable strings after they are received\n");
      fprintf(stderr,"    -v3         Printable strings and SLIP packet notifications\n");
      fprintf(stderr,"    -v4         All printable characters as they are received\n");
      fprintf(stderr,"    -v5         All SLIP packets in hex\n");
      fprintf(stderr,"    -v          Equivalent to -v3\n");
      fprintf(stderr," -d[basedelay]  Minimum delay between outgoing SLIP packets.\n");
      fprintf(stderr,"                Actual delay is basedelay*(#6LowPAN fragments) milliseconds.\n");
      fprintf(stderr,"                -d is equivalent to -d10.\n");
      fprintf(stderr," -a serveraddr  \n");
      fprintf(stderr," -p serverport  \n");
      exit(1);
      break;
    }
  }
  argc -= (optind - 1);
  argv += (optind - 1);

  if(argc != 1 && argc != 2) {
    err(1, "usage: %s [-B baudrate] [-H] [-L] [-s siodev] [-t tundev] [-T] [-v verbosity] [-d delay] [-a serveraddress] [-p serverport] ipaddress", prog);
  }
  ipaddr = "10.20.30.40";

  if(port == NULL) {
    port = "7020";
  }

  if (portno == 0) {
    portno = 7020;
  }
    
  if((tcpfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("client: tcp socket");
    exit(1);
  }
    
    if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
      perror("setsockopt(SO_REUSEADDR) failed");
  
  bzero((char *) &listenaddr, sizeof(listenaddr));
  listenaddr.sin_family = AF_INET;
  listenaddr.sin_port = htons(portno);
  listenaddr.sin_addr.s_addr = INADDR_ANY;
  if (bind(tcpfd, (struct sockaddr *) &listenaddr,
           sizeof(listenaddr)) < 0)
    perror("bind");
    
  printf("Listen on port %d\n", portno);

  //inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
  //s, sizeof(s));
  //fprintf(stderr, "slip connected to ``%s:%s''\n", s, port);

  tunfd = tun_alloc(tundev, tap);
  if(tunfd == -1) err(1, "main: open /dev/tun");
  if (timestamp) stamptime();
  fprintf(stderr, "opened %s device ``/dev/%s''\n",
          tap ? "tap" : "tun", tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  signal(SIGALRM, sigalarm);
  ifconf(tundev, ipaddr);

  if (listen(tcpfd, 5) < 0) {
    perror("listen");
  }
  { struct sockaddr_in clientaddr;
    socklen_t clientlen;
    clientlen = sizeof(clientaddr);
    char s[80];
    clientfd = accept(tcpfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (clientfd < 0) {
      perror("accept");
    }
    printf("Accepted\n");
    inet_ntop(clientaddr.sin_family, get_in_addr((struct sockaddr *) &clientaddr),
              s, sizeof(s));
    printf("Accepted connection from %s:%d\n", s, ntohs(clientaddr.sin_port));
  //fprintf(stderr, "slip connected to ``%s:%s''\n", s, port);

  }
  while(1) {
    maxfd = 0;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    FD_SET(clientfd, &rset);	/* Read from slip ASAP! */
    FD_SET(tcpfd, &rset);	/* Read from slip ASAP! */
    if(clientfd > maxfd) maxfd = clientfd;

    FD_SET(tunfd, &rset);
    if(tunfd > maxfd) maxfd = tunfd;

    ret = select(maxfd + 1, &rset, &wset, NULL, NULL);
    if(ret == -1 && errno != EINTR) {
      err(1, "select");
    } else if(ret > 0) {
      if(FD_ISSET(clientfd, &rset)) {
        printf("DAya from client!\n");
        tcp_to_tun(clientfd, tunfd);

      }

      /* Optional delay between outgoing packets */
      /* Base delay times number of 6lowpan fragments to be sent */
      if(FD_ISSET(tunfd, &rset)) {
        int size;
        size=tun_to_tcp(tunfd, clientfd);
        slip_flushbuf(slipfd);
      }
    }
  }
}
