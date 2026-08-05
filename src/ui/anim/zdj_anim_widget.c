#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>

void zdj_anim_init_record_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W - 34;
    anim->start_point.y = -12;
    anim->end_point.x = ZDJ_SCREEN_W - 34;
    anim->end_point.y = 1;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_record_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W - 34;
    anim->start_point.y = 1;
    anim->end_point.x = ZDJ_SCREEN_W - 34;
    anim->end_point.y = -12;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_record_widget( zdj_anim_t * anim, zdj_view_t * view ) {
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}



void zdj_anim_init_debug_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W - 40;
    anim->start_point.y = ZDJ_SCREEN_H + 2;
    anim->end_point.x = ZDJ_SCREEN_W - 40;
    anim->end_point.y = ZDJ_SCREEN_H - 8;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_debug_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W - 40;
    anim->start_point.y = ZDJ_SCREEN_H - 8;
    anim->end_point.x = ZDJ_SCREEN_W - 40;
    anim->end_point.y = ZDJ_SCREEN_H + 2;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_debug_widget( zdj_anim_t * anim, zdj_view_t * view ) {
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}


void zdj_anim_init_perf_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = ZDJ_SCREEN_H + 2;
    anim->end_point.x = 0;
    anim->end_point.y = ZDJ_SCREEN_H - 8;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_perf_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = ZDJ_SCREEN_H - 8;
    anim->end_point.x = 0;
    anim->end_point.y = ZDJ_SCREEN_H + 2;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_perf_widget( zdj_anim_t * anim, zdj_view_t * view ) {
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}



void zdj_anim_init_notify_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // printf( "zdj_anim_init_notify_widget_show\n" );
    anim->start_point.x = 0;
    anim->start_point.y = ZDJ_SCREEN_H + 2;
    anim->end_point.x = 0;
    anim->end_point.y = ZDJ_SCREEN_H - 8;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_notify_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // printf( "zdj_anim_init_notify_widget_hide\n" );
    anim->start_point.x = 0;
    anim->start_point.y = ZDJ_SCREEN_H - 8;
    anim->end_point.x = 0;
    anim->end_point.y = ZDJ_SCREEN_H + 2;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_notify_widget( zdj_anim_t * anim, zdj_view_t * view ) {
    // printf( "zdj_anim_update_notify_widget\n" );
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}

void zdj_anim_init_crash_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // printf( "zdj_anim_init_notify_widget_show\n" );
    anim->start_point.x = 10;
    anim->start_point.y = ZDJ_SCREEN_H + 2;
    anim->end_point.x = 10;
    anim->end_point.y = 7;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_crash_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // printf( "zdj_anim_init_notify_widget_hide\n" );
    anim->start_point.x = 10;
    anim->start_point.y = 7;
    anim->end_point.x = 10;
    anim->end_point.y = ZDJ_SCREEN_H + 2;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_crash_widget( zdj_anim_t * anim, zdj_view_t * view ) {
    // printf( "zdj_anim_update_notify_widget\n" );
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}