#ifndef _SWITCH_H
#define _SWITCH_H

#include "main.h"

typedef struct _VRackSwitch VRackSwitch;

VRackSwitch *switch_new(VRackCtxt *ctxt, guint32 n_ports);
void switch_shutdown(VRackSwitch *sw);

#endif
