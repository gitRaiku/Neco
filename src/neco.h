#ifndef NECO_H
#define NECO_H

#define NAME "Neco"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <wchar.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <linux/input.h>

#include <wayland-client.h>
#include <wayland-cursor.h>

#include "../protocol/xdgsh-protocol.h"
#include "../protocol/xout-protocol.h"
#include "../protocol/zwlr-protocol.h"

#include "config.h"
#include "vecs.h"

/// Versions
#define COMPV 4
#define SHMV 1
#define ZWLRV 4
#define XWMBASEV 2
#define XOUTMGRV 3
#define SEATV 7
#define WOUTV 1


#define CMON state.mons[i]
#define SLEN(x) (sizeof(x)/sizeof(x[0]))
#define FUNNIERCSTRING(x) #x
#define FUNNYCSTRING(x) FUNNIERCSTRING(x)
#define LOG(imp,...) {if (log_level <= imp) {fprintf(imp <= 5 ? stdout : stderr, "Neco: " __VA_ARGS__);}}
#define WLCHECK(x,e) {if(!(x)){LOG(10, NAME ": Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e "\n"); exit(1);}}
#define WLCHECKE(x,e) {if(!(x)){LOG(10, NAME "Error running " #x " on " __FILE__ ":" FUNNYCSTRING(__LINE__) ": " e " [%m]\n"); exit(1);}}

struct sbuf {
  uint32_t height, width;
  uint32_t size;
  uint32_t fd;
  uint32_t *data;
  struct wl_buffer *__restrict b[2];
  uint8_t csel;
};

struct cmon {
	uint32_t n;
  char *xdgname;
  struct wl_output *out;
  struct zxdg_output_v1 *xout;
  struct zwlr_layer_surface_v1 *lsurf;
  struct wl_surface *surf;
  struct sbuf sb;
};

struct cseatp {
  struct wl_pointer *p;
  struct cmon *fmon;
  int32_t x, y;
  uint8_t lpres, rpres;
};

struct cseat {
  uint32_t n;
  struct wl_seat *s;
  struct cseatp p;
};

struct coutp {
  uint32_t n;
  struct wl_output* o;
};
DEF_VECTOR_SUITE(seat, struct cseat)

#endif


