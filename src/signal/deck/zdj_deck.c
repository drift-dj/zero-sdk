#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/clock/zdj_deck_clock.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/deck/io/zdj_deck_io.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/settings/zdj_settings.h>