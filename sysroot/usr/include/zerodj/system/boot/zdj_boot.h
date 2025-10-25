// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_BOOT_H
#define ZDJ_BOOT_H

#include <zerodj/health/zdj_health_type.h>

#define ZDJ_SHARED_DOMAIN_BOOT_STATE_ADDR 0x55a71000

#define ZDJ_DOMAIN_BOOT_M_POWER_ON 0
#define ZDJ_DOMAIN_BOOT_M_POWER_ON_MASK 1 << ZDJ_DOMAIN_BOOT_M_POWER_ON

#define ZDJ_DOMAIN_BOOT_M_I2C_AVAIL 1
#define ZDJ_DOMAIN_BOOT_M_I2C_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_M_I2C_AVAIL 

#define ZDJ_DOMAIN_BOOT_M_AUDIO_AVAIL 2
#define ZDJ_DOMAIN_BOOT_M_AUDIO_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_M_AUDIO_AVAIL

#define ZDJ_DOMAIN_BOOT_M_HMI_AVAIL 3
#define ZDJ_DOMAIN_BOOT_M_HMI_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_M_HMI_AVAIL

#define ZDJ_DOMAIN_BOOT_M_DISPLAY_AVAIL 4
#define ZDJ_DOMAIN_BOOT_M_DISPLAY_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_M_DISPLAY_AVAIL

#define ZDJ_DOMAIN_BOOT_M_RPMSG_AVAIL 5
#define ZDJ_DOMAIN_BOOT_M_RPMSG_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_M_RPMSG_AVAIL


#define ZDJ_DOMAIN_BOOT_A_LINUX_BOOT 0
#define ZDJ_DOMAIN_BOOT_A_LINUX_BOOT_MASK 1 << ZDJ_DOMAIN_BOOT_A_LINUX_BOOT

#define ZDJ_DOMAIN_BOOT_A_RPMSG_AVAIL 1
#define ZDJ_DOMAIN_BOOT_A_RPMSG_AVAIL_MASK 1 << ZDJ_DOMAIN_BOOT_A_RPMSG_AVAIL

typedef struct {
    int32_t fac_reset;
    int32_t m_avail;
    int32_t m_flags;
    int32_t a_avail;
    int32_t a_flags;
    int32_t update_display_req;
    int32_t flush_hmi_req;
} zdj_domain_boot_state_t;

typedef enum {
    BOOT_MODE_DEVELOPER,
    BOOT_MODE_CONFIG,
    BOOT_MODE_DRIVE,
    BOOT_MODE_NORMAL,
    BOOT_MODE_TEST
} zdj_boot_mode_t;

typedef struct {
    zdj_boot_mode_t mode;
    char buffer[ 128 ];
} zdj_normal_boot_override_t;

void zdj_boot_scan_hmi( void );

zdj_boot_mode_t zdj_boot_get_normal_override( void );
zdj_health_status_t zdj_boot_set_normal_override( zdj_boot_mode_t mode );

#endif