#ifndef __PLUGIN_H
#define __PLUGIN_H
#include "types.h"

struct cpu_interface {
    void (*initialize)(char *, accessor_t);
    void (*loop)();
    void (*finalize)();
};

#ifndef PLUGIN_INTERFACE
struct cpu_interface *(*get_cpu_interface)();
#endif

#endif
