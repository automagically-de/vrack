#ifndef _QMACH_H
#define _QMACH_H

#include "main.h"

typedef struct _VRackQMach VRackQMach;

VRackQMach *qmach_new(VRackCtxt *ctxt, const gchar *description);
void qmach_shutdown(VRackQMach *qm);
VRackKvmSource *qmach_get_kvm_source(VRackQMach *qm);

#endif
