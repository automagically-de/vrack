#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "kvm.h"

typedef struct _VRackDisplay VRackDisplay;

VRackDisplay *display_new(VRackCtxt *ctxt);
void display_shutdown(VRackDisplay *display);
gboolean display_set_kvm_source(VRackDisplay *dpy, VRackKvmSource *source);

#endif
