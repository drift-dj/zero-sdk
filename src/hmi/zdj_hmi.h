#ifndef ZDJ_HMI_H
#define ZDJ_HMI_H

typedef enum {
    HMI_EVENT_ENCO_0_
} hmi_event_id_t;

typedef struct {
    hmi_event_id_t id;
    bool blocked;
    int i_val;
    double d_val;
    bool b_val;
} hmi_event_t;

void zdj_hmi_activate( void );
void zdj_hmi_deactivate( void );
void zdj_hmi_update( void );

#endif