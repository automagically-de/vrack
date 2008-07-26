#include <gtk/gtk.h>

#include "main.h"
#include "kvm.h"
#include "rack.h"

struct _VRackKvmSwitch {
	VRackItem *rackitem;

	guint32 n_ports;
	VRackKvmSource *source;
	VRackKvmSink **sinks;
	GtkWidget *widget;
	GtkWidget **w_ports;
};

VRackKvmSwitch *kvm_switch_new(VRackCtxt *ctxt, guint32 n_ports)
{
	VRackKvmSwitch *kvm;

	kvm = g_new0(VRackKvmSwitch, 1);

	return kvm;
}
