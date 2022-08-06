#ifndef V4L2_EVENTS
#define V4L2_EVENTS

#include "headers.h"

void uvc_events_subscribe(struct processing *processing);

void uvc_events_unsubscribe(struct processing *processing);

void uvc_events_process(struct processing *processing);

#endif // end V4L2_EVENTS
