#ifndef GEL_TRACING_H
#define GEL_TRACING_H

#ifdef GEL_TRACING

#define TRACY_ENABLE 1
#include <tracy/Tracy.hpp>

#define TRACE_MARK FrameMark
#define TRACE_ZONE ZoneScoped
#define TRACE_ZONE_NAMED(Name) ZoneScopedN((Name))
#define TRACE_TAG(Value) (ZoneText((Value), strlen((Value))))

#else

#define TRACE_MARK
#define TRACE_ZONE
#define TRACE_ZONE_NAMED(Name)
#define TRACE_TAG(Value)

#endif  // GEL_TRACING

#endif  // GEL_TRACING_H
