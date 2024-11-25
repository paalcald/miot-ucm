#ifndef FSM_EVENT_H_
#define FSM_EVENT_H_
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(STATE_EVENT);
typedef enum
{
    STATE_EVENT_MONITORING,
    STATE_EVENT_CONSOLE,
} state_event_id_t;

#endif // !FSM_EVENT_H_
