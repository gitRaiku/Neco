#include "neco.h"

#include "gif_load.h"

VECTOR_SUITE(seat, struct cseat)

uint32_t log_level;
FILE *__restrict errf;

struct cstate {
  struct wl_display *dpy;
  struct wl_registry *reg;
  struct wl_shm *shm;
  struct wl_compositor *comp;
  struct xdg_wm_base *xwmBase;
  struct wl_region *oreg;
  struct xdg_surface *xsurf;
  struct xdg_toplevel *xtlev;
  struct zwlr_layer_shell_v1 *zwlr;
  struct zxdg_output_manager_v1 *xoutmgr;
  struct seatv seats;
  //struct outpv outs;
  struct wl_cursor_image *pImg;
  struct wl_surface *pSurf;
  struct cmon *__restrict mons;
  uint32_t monsl;

  uint32_t width, height;
  uint8_t closed;
  uint32_t tlen[9]; // Length in px of every tag icon
};
struct cstate state;
void render(struct cmon *mon);

struct cmon *mon_from_surf(struct wl_surface *surf) { int32_t i; for(i = 0; i < state.monsl; ++i) { if (CMON.surf == surf) { return &CMON; } } LOG(10, "Could not find monitor from surface!\n"); return NULL; }
void xwmb_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); }
const struct xdg_wm_base_listener xwmb_listener = { .ping = xwmb_ping };
void p_axis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v) { }
void p_axis_source(void *d, struct wl_pointer *p, uint32_t s) { }
void p_axis_stop(void *d, struct wl_pointer *p, uint32_t t, uint32_t s) { }
void p_axis_discrete(void *d, struct wl_pointer *p, uint32_t t, int s) { }
void p_enter(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
  struct cseat *seat = data;
  seat->p.fmon = mon_from_surf(surface); seat->p.lpres = 0; seat->p.rpres = 0;
  if (!state.pImg) {
    struct wl_cursor_theme *cth = wl_cursor_theme_load(NULL, 32, state.shm);
    state.pImg = wl_cursor_theme_get_cursor(cth, "left_ptr")->images[0];
    state.pSurf = wl_compositor_create_surface(state.comp);
    wl_surface_attach(state.pSurf, wl_cursor_image_get_buffer(state.pImg), 0, 0);
    wl_surface_commit(state.pSurf);
  }
  wl_pointer_set_cursor(ptr, serial, state.pSurf, state.pImg->hotspot_x, state.pImg->hotspot_y);
}
void p_leave(void *data, struct wl_pointer *ptr, uint32_t serial, struct wl_surface *surf) { struct cseat *seat = data; seat->p.x = seat->p.y = seat->p.lpres = seat->p.rpres = 0; seat->p.fmon = NULL; }
void p_motion(void *data, struct wl_pointer *ptr, uint32_t time, wl_fixed_t x, wl_fixed_t y) { struct cseat *seat = data; seat->p.x = wl_fixed_to_int(x); seat->p.y = wl_fixed_to_int(y); }
void gname(char *__restrict s) {uint32_t fnl=strlen(s);int32_t i,r;for(i=1;i<=6;++i){r=random();s[fnl-i]='A'+(r&15)+((r&16)<<1);}}
int32_t cshmf(uint32_t size) {
  char fnm[] = NAME "-000000";

  int32_t fd = 0;
  uint32_t retries = 100;
  do { gname(fnm); fd = shm_open(fnm, O_RDWR | O_CREAT | O_EXCL, 0600); if (fd >= 0) { break; } } while (--retries); if (retries == 0) { LOG(0, "Could not create the shm file! [%m]\n"); exit(1); }
  WLCHECKE(!shm_unlink(fnm),"Could not unlink the shm file!");
  WLCHECKE(!ftruncate(fd, size),"Could not truncate the shm file!");
  return fd;
}

void cbufs(struct cmon *mon, uint32_t width, uint32_t height, enum wl_shm_format form) {
  uint32_t stride = width * 4;
  uint32_t size = stride * height;
  uint32_t fd = cshmf(size * 2);
  struct wl_shm_pool *pool = wl_shm_create_pool(state.shm, fd, size * 2);
  uint32_t *data = mmap(NULL, size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  WLCHECK(data!=MAP_FAILED,"Could not mmap the shm data!");
  mon->sb.b[0] = wl_shm_pool_create_buffer(pool,    0, width, height, stride, form);
  WLCHECK(mon->sb.b[0],"Could not create the first shm buffer!");
  mon->sb.b[1] = wl_shm_pool_create_buffer(pool, size, width, height, stride, form);
  WLCHECK(mon->sb.b[1],"Could not create the second shm buffer!");
  wl_shm_pool_destroy(pool);

  mon->sb.fd = fd;
  mon->sb.data = data;
  mon->sb.size = size;
  mon->sb.width = width;
  mon->sb.height = height;
}

void zwlr_configure(void *data, struct zwlr_layer_surface_v1 *l, uint32_t serial, uint32_t width, uint32_t height) {  /// TODO
  struct cmon *mon = data;
  zwlr_layer_surface_v1_ack_configure(l, serial);
  if (mon->sb.b[0]) { return; }
  cbufs(mon, width, height, WL_SHM_FORMAT_ARGB8888);
  render(mon);
}
void zwlr_closed(void *data, struct zwlr_layer_surface_v1 *l) { }
struct zwlr_layer_surface_v1_listener zwlr_listener = { .configure = zwlr_configure, .closed = zwlr_closed };

struct gif {
  uint32_t w, h, framec;
  uint8_t *__restrict data; /// frames * w * h
  uint32_t *__restrict times; /// frames
  uint32_t ncol;
  uint32_t *__restrict cols;
  uint8_t curf;
};
struct gif gif;
#define GIFC(g, f, y, x) ((g).data[(f) * (g).w * (g).h + (y) * (g).w + (x)])

uint64_t ntu;
uint64_t getcurtu() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * 100 + ts.tv_nsec / 10000000;
}
void render(struct cmon *mon) { /// TODO:
  if (!mon->sb.b[0]) { return; }

  uint64_t ctu = getcurtu();
  if (ctu >= ntu) {
    ++gif.curf;
    /*
    int32_t i, j;
    for (i = 0; i < mon->sb.height; ++i) {
      for (j = 0; j < mon->sb.width; ++j) {
        fprintf(stdout, "%2X ", GIFC(gif, gif.curf, i, j));
      }
      fputc('\n', stdout);
    }
    */
    gif.curf %= gif.framec;
    ntu += gif.times[gif.curf];
  }

  uint32_t i, j;
  uint32_t co = mon->sb.size * mon->sb.csel / 4;
  for (i = 0; i < mon->sb.height; ++i) {
    for (j = 0; j < mon->sb.width; ++j) {
      mon->sb.data[co + (i + 0) * mon->sb.width + j + 0] = gif.cols[GIFC(gif, gif.curf, i, j)];
    }
  }

  wl_surface_attach(mon->surf, mon->sb.b[mon->sb.csel], 0, 0);
  wl_surface_damage(mon->surf, 0, 0, mon->sb.width, mon->sb.height);
  wl_surface_commit(mon->surf);
  mon->sb.csel = 1 - mon->sb.csel;
}

void zxout_logical_position(void *d, struct zxdg_output_v1 *z, int x, int y) { }
void zxout_logical_size(void *d, struct zxdg_output_v1 *z, int x, int y) { }
void zxout_done(void *d, struct zxdg_output_v1 *z) { }
void zxout_description(void *d, struct zxdg_output_v1 *z, const char *c) { }
void zxout_name(void* data, struct zxdg_output_v1* xout, const char* name) {
  struct cmon *mon = data;
  mon->xdgname = strdup(name);
  LOG(0, "Got monitor name %s[Added to mon %u]\n", name, mon->n);
  zxdg_output_v1_destroy(xout);
}
const struct zxdg_output_v1_listener zxout_listener = { .name = zxout_name, .logical_position = zxout_logical_position, .logical_size = zxout_logical_size, .done = zxout_done, .description = zxout_description };

void p_frame(void *data, struct wl_pointer *ptr) { /// TODO
  struct cseat *seat = data;
  if (!seat->p.fmon) { return; }
  if (seat->p.lpres == 1) {
    /// Handle clicks
  }
}
void p_button(void *data, struct wl_pointer *ptr, uint32_t serial, uint32_t time, uint32_t button, uint32_t pressed) {
  struct cseat *seat = data;
  uint8_t cp = pressed == WL_POINTER_BUTTON_STATE_PRESSED;
  if (button == BTN_LEFT) { if (seat->p.lpres && cp) { seat->p.lpres = 2; } else { seat->p.lpres = cp; } }
  if (button == BTN_RIGHT) { if (seat->p.rpres && cp) { seat->p.rpres = 2; } else { seat->p.rpres = cp; } 
  }
}
const struct wl_pointer_listener pointer_listener = { .enter = p_enter, .leave = p_leave, .motion = p_motion, .button = p_button, .frame = p_frame, .axis = p_axis, .axis_source = p_axis_source, .axis_stop = p_axis_stop, .axis_discrete = p_axis_discrete };

void seat_name(void *d, struct wl_seat *s, const char *n) { }
void seat_caps(void *data, struct wl_seat *s, uint32_t caps) {
  struct cseat *seat = data;
  uint8_t hasPointer = caps & WL_SEAT_CAPABILITY_POINTER;
  if (!seat->p.p && hasPointer) {
    seat->p.p = wl_seat_get_pointer(seat->s);
    wl_pointer_add_listener(seat->p.p, &pointer_listener, seat);
  }
}
const struct wl_seat_listener seat_listener = { .capabilities = seat_caps, .name = seat_name };

void reg_global_remove(void *data, struct wl_registry *reg, uint32_t name) { }
void reg_global(void *data, struct wl_registry *reg, uint32_t name, const char *iface, uint32_t ver) {
#define CHI(x,y,z,w) {if(!strcmp(iface,y .name)) {state. x=wl_registry_bind(reg, name, &y, z);w;return;}}
#define CHV(x,y,z,w) {if(!strcmp(iface,y .name)) {x cbind=wl_registry_bind(reg, name, &y, z);w;return;}}

  CHI(comp           , wl_compositor_interface         , COMPV,);
  CHI(shm            , wl_shm_interface                , SHMV,);
  CHI(zwlr           , zwlr_layer_shell_v1_interface   , ZWLRV,);
  CHI(xwmBase        , xdg_wm_base_interface           , XWMBASEV,
      xdg_wm_base_add_listener(state.xwmBase, &xwmb_listener, NULL));
  CHI(xoutmgr        , zxdg_output_manager_v1_interface, XOUTMGRV,);
  CHV(struct wl_seat*, wl_seat_interface               , SEATV,
      struct cseat cs = {0};
      cs.n = name;
      cs.s = cbind;
      seatvp(&state.seats, cs);
      wl_seat_add_listener(cbind, &seat_listener, state.seats.v + state.seats.l - 1););
  CHV(struct wl_output*, wl_output_interface           , WOUTV,
      state.mons = realloc(state.mons, (state.monsl + 1) * sizeof(state.mons[0]));
      struct cmon mon = {0};
      mon.n = name;
      LOG(0, "Added monitor: %u\n", name);
      mon.out = cbind;
      state.mons[state.monsl] = mon;
      ++state.monsl;
      );
#undef CHI
#undef CHV
}
const struct wl_registry_listener reg_listener = { .global = reg_global, .global_remove = reg_global_remove};

void init_wayland() {
  WLCHECK(state.dpy=wl_display_connect(NULL),"Could not connect to the wayland display!");
  WLCHECK(state.reg=wl_display_get_registry(state.dpy),"Could not fetch the wayland registry!");
  seatvi(&state.seats);
  wl_registry_add_listener(state.reg, &reg_listener, NULL);
  wl_display_roundtrip(state.dpy);

#define CHECK_INIT(x, e, v) {if (!state. x) { LOG(10, "Your wayland compositor does not support " #e " version " #v "which is required for me to work :(\n"); exit(1); }}
  CHECK_INIT(comp      , wl_compositor         , COMPV   );
  CHECK_INIT(shm       , wl_shm                , SHMV    );
  CHECK_INIT(zwlr      , zwlr_layer_shell_v1   , ZWLRV   );
  CHECK_INIT(xoutmgr   , zxdg_output_manager_v1, XOUTMGRV);
#undef CHECK_INIT

  seatvt(&state.seats);

  int32_t i;
  for(i = 0; i < state.monsl; ++i) {
    CMON.xout = zxdg_output_manager_v1_get_xdg_output(state.xoutmgr, CMON.out);
    zxdg_output_v1_add_listener(CMON.xout, &zxout_listener, &CMON);
    wl_display_roundtrip(state.dpy);
  }

  for(i = 0; i < state.monsl; ++i) {
    CMON.surf = wl_compositor_create_surface(state.comp); WLCHECK(CMON.surf,"Cannot create wayland surface!");
    CMON.lsurf = zwlr_layer_shell_v1_get_layer_surface(state.zwlr, CMON.surf, CMON.out, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, NAME); WLCHECK(CMON.lsurf,"Cannot create zwlr surface!");
    zwlr_layer_surface_v1_add_listener(CMON.lsurf, &zwlr_listener, &CMON);

    zwlr_layer_surface_v1_set_size(CMON.lsurf, gif.w, gif.h);
    zwlr_layer_surface_v1_set_keyboard_interactivity(CMON.lsurf, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
    wl_surface_commit(CMON.surf);
  }
  wl_display_dispatch(state.dpy);
}

void render_mons() { int32_t i; for(i = 0; i < state.monsl; ++i) { render(&CMON); } }

void getframe(void* data, struct GIF_WHDR* whdr) {
  struct gif *__restrict g = data;
  if (!whdr->ifrm) { 
    g->w = whdr->xdim;
    g->h = whdr->ydim;
    g->framec = whdr->nfrm;
    g->data = malloc(g->w * g->h * g->framec);
    g->times = malloc(g->framec * sizeof(*g->times));
    g->ncol = whdr->clrs;
    g->cols = malloc(g->ncol * sizeof(*g->cols));
    int32_t i;
    for(i = 0; i < g->ncol; ++i) {
      if (i == whdr->tran) {
        g->cols[i] = 0;
      } else {
        g->cols[i] = 0xFF000000 | (whdr->cpal[i].R << 16) | (whdr->cpal[i].G << 8) | whdr->cpal[i].B;
      }
    }
  }

  g->times[whdr->ifrm] = whdr->time;

  {
    int32_t i, j;
    for (i = 0; i < g->h; ++i) {
      for (j = 0; j < g->w; ++j) {
        if ((i < whdr->fryo) || (i >= whdr->fryo + whdr->fryd) ||
            (j < whdr->frxo) || (j >= whdr->frxo + whdr->frxd)) {
          GIFC(*g, whdr->ifrm, i, j) = whdr->tran;
        } else {
          GIFC(*g, whdr->ifrm, i, j) = whdr->bptr[(i - whdr->fryo) * whdr->frxd + j - whdr->frxo];
        }
      }
    }
  }
}

void loadgif(char *__restrict fname, struct gif *__restrict g) {
  uint32_t op = open(fname, O_RDONLY);
  if (op == -1) { fprintf(stderr, "Could not open %s! [%m]\n", fname); exit(1); }
  uint32_t size = (unsigned long)lseek(op, 0UL, 2 /** SEEK_END **/);
  lseek(op, 0UL, 0 /** SEEK_SET **/);

  uint8_t *__restrict mem = malloc(size);
  if (read(op, mem, size));
  close(op);
  GIF_Load(mem, size, getframe, 0, g, 0L);
}

int main(void) {
  setbuf(stderr, NULL);
  setlocale(LC_ALL, "");
  log_level = 0;
  errf = stderr;
  
  char fname[] = "/home/arch/Misc/Downloads/neco.gif";
  loadgif(fname, &gif);
  ntu = getcurtu() + gif.times[0];
  init_wayland();

  while (1) {
    wl_display_dispatch(state.dpy);
    render_mons();
  }

  return 0;
}

