#ifndef GEL_TRACING_H
#define GEL_TRACING_H

#ifdef GEL_TRACING

#define TRACY_ENABLE 1
#include <tracy/Tracy.hpp>

#define TRACE_BEGIN ZoneScoped
#define TRACE_SECTION(Name) ZoneScopedN(#Name)
#define TRACE_TAG(Value) (ZoneText((Value), strlen((Value))))
#define TRACE_END FrameMark

#else

#define TRACE_BEGIN
#define TRACE_SECTION(Name)
#define TRACE_TAG(Value)
#define TRACE_END

#endif  // GEL_TRACING

#endif  // GEL_TRACING_H
