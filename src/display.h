#ifndef _DISPLAY_H
#define _DISPLAY_H

typedef struct _VRackDisplay VRackDisplay;

VRackDisplay *display_new(VRackCtxt *ctxt);
void display_shutdown(VRackDisplay *display);

#endif
