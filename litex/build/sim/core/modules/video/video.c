#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include <unistd.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <termios.h>

#include "modules.h"

struct session_s {
  char *hsync;
  char *vsync;
  short *hcount;
  short *vcount;
  char *de;
  char *sys_clk;
  struct event *ev;
  int x, y;
};

struct event_base *base;
static int litex_sim_module_pads_get(struct pad_s *pads, char *name, void **signal)
{
  int ret = RC_OK;
  void *sig = NULL;
  int i;

  if(!pads || !name || !signal) {
    ret=RC_INVARG;
    goto out;
  }

  i = 0;
  while(pads[i].name) {
    if(!strcmp(pads[i].name, name)) {
      sig = (void*)pads[i].signal;
      break;
    }
    i++;
  }

out:
  *signal = sig;
  return ret;
}


static int videosim_start(void *b)
{
  base = (struct event_base *)b;
  printf("[video] loaded (%p)\n", base);
  return RC_OK;
}

static int videosim_new(void **sess, char *args)
{
  int ret = RC_OK;
  struct session_s *s = NULL;

  if(!sess) {
    ret = RC_INVARG;
    goto out;
  }

  s = (struct session_s*) malloc(sizeof(struct session_s));
  if(!s) {
    ret=RC_NOENMEM;
    goto out;
  }
  memset(s, 0, sizeof(struct session_s));

out:
  *sess = (void*) s;
  return ret;
}

static int videosim_add_pads(void *sess, struct pad_list_s *plist)
{
  int ret = RC_OK;
  struct session_s *s = (struct session_s*) sess;
  struct pad_s *pads;

  if(!sess || !plist) {
    ret = RC_INVARG;
    goto out;
  }
  pads = plist->pads;
  if(!strcmp(plist->name, "vga")) {
    litex_sim_module_pads_get(pads, "hsync", (void**)&s->hsync);
    litex_sim_module_pads_get(pads, "vsync", (void**)&s->vsync);
    litex_sim_module_pads_get(pads, "hcount", (void**)&s->hcount);
    litex_sim_module_pads_get(pads, "vcount", (void**)&s->vcount);
    litex_sim_module_pads_get(pads, "de", (void**)&s->de);
  }


  if(!strcmp(plist->name, "sys_clk"))
    litex_sim_module_pads_get(pads, "sys_clk", (void**) &s->sys_clk);
out:
  return ret;
}

static int videosim_tick(void *sess, uint64_t time_ps) {
  static clk_edge_state_t edge;
  struct session_s *s = (struct session_s*)sess;

  if(!clk_pos_edge(&edge, *s->sys_clk)) {
    return RC_OK;
  }

  if(*s->hsync)
  {
    if(s->x != 0)
    {
      if(*s->vsync)
      {
        //if(s->y != 0) printf("max x, y %d, %d\n", s->x, s->y); //nre frame condition
        s->y = 0;
      }
      else
        s->y = s->y + 1;
    }
    s->x = 0;
  }
  else
    s->x = s->x + 1;

  //printf("x, y %d %d, %d %d\n", s->x, *s->hcount, s->y, *s->vcount); //not equal as cause of sync


  return RC_OK;
}

static struct ext_module_s ext_mod = {
  "video",
  videosim_start,
  videosim_new,
  videosim_add_pads,
  NULL,
  videosim_tick
};

int litex_sim_ext_module_init(int (*register_module) (struct ext_module_s *))
{
  int ret = RC_OK;
  ret = register_module(&ext_mod);
  return ret;
}
