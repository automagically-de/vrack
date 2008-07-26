#ifndef _KVM_H
#define _KVM_H

#include <gtk/gtk.h>

#include "main.h"

typedef struct _VRackKvmSwitch VRackKvmSwitch;
typedef struct _VRackKvmSource VRackKvmSource;
typedef struct _VRackKvmSink VRackKvmSink;

VRackKvmSwitch *kvm_switch_new(VRackCtxt *ctxt, guint32 n_ports);
void kvm_switch_shutdown(VRackKvmSwitch *kvm);
VRackKvmSink *kvm_switch_attach(VRackKvmSwitch *kvm, VRackKvmSource *source,
	guint32 port);
VRackKvmSource *kvm_switch_get_kvm_source(VRackKvmSwitch *kvm);

typedef GdkPixbuf *(* VRackKvmGetPixbufFunc)(gpointer opaque);
typedef gboolean (*VRackKvmGetSizeFunc)(gpointer opaque, guint32 *w,
	guint32 *h);

typedef gboolean (* VRackKvmSendMouseMoveFunc)(gpointer opaque,
	guint32 x, guint32 y);
typedef gboolean (* VRackKvmSendMouseButtonFunc)(gpointer opaque,
	guint32 button_mask);

struct _VRackKvmSource {
	VRackKvmGetPixbufFunc get_pixbuf;
	VRackKvmGetSizeFunc get_size;
	VRackKvmSendMouseMoveFunc send_mouse_move;
	VRackKvmSendMouseButtonFunc send_mouse_button;
	gpointer opaque;
};


struct _VRackKvmSink {
	VRackKvmSource *source;
	gpointer opaque;
};

#endif /* _KVM_H */
