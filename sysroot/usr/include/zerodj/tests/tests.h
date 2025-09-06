#ifndef TESTS_H
#define TESTS_H

typedef enum {
    TEST_UI_VIEW,
    TEST_UI_ASSET,
    TEST_UI_ASSET_VIEW,
    TEST_COUNT
} test_type_t;

typedef enum {
    TEST_STATUS_INIT,
    TEST_STATUS_READY,
    TEST_STATUS_WAIT,
    TEST_STATUS_RUN,
    TEST_STATUS_DONE
} test_status_t;

typedef struct {
    test_type_t type;
    test_status_t status;
    double progress;
    long start_rss;
    long end_rss;
} test_t;

typedef void ( *zdj_test_func_t )( test_t * );

void zdj_init_tests( void );

// UI View Tests
void zdj_view_standup_test( test_t * test );
void zdj_view_shuffle_test( test_t * test );
void zdj_view_nesting_test( test_t * test );

#endif