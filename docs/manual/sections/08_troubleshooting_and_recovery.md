# 8. Troubleshooting, Diagnostics & Recovery

This reference provides failure mode diagnostics, recovery procedures, and developer diagnostic tools for Drift Zero hardware and firmware.

## Hardware Diagnostic & Failure Matrix

| Subsystem | Failure Symptom | Underlying Cause | Corrective Action |
|---|---|---|---|
| **Power & Stability** | OLED display flickers, unit suddenly reboots during bass transients | Input voltage brownout below 4.75V due to insufficient USB power supply | Connect to a dedicated 5V / 2.0A (10W minimum) USB-C power source. Avoid unpowered USB hubs. |
| **USB Connectivity** | Zero is unrecognized by host computer | Zero set to Host Mode, or connected via charge-only cable | Go to `Control Panels > USB Setup`, switch role to `USB Gadget (Drive Mode)`, and verify cable supports USB 2.0 High-Speed data. |
| **Mass Storage** | Host OS mounts drive as read-only; cannot copy tracks | Storage unmounted improperly, setting filesystem dirty flag | Go to `Control Panels > Settings > System > File System Check` to repair journal locks. |
| **Audio Engine** | Audio clicks or dropouts during dense looping | DSP CPU load exceeded 85% headroom limit | Open `Settings > Developer > DSP Telemetry`. Reduce simultaneous 1/16-beat loopers during active background track analysis. |
| **Fader Response** | Zipper noise or stepping artifacts when moving faders | Outdated firmware version without slew smoothing | Verify firmware is updated to version 0.6.0+, which incorporates micro-frame slew interpolation on all faders (PR #31). |
| **Database** | Songs missing from playlists or search crashes | SQLite index corruption following sudden power loss | Go to `Control Panels > Settings > System > Reset Library Database`. Existing audio files on disk remain untouched. |

## Audio Pipeline Slew Smoothing (Firmware 0.6.0+)

System Version 0.6.0 incorporates control smoothing across linear faders, crossfaders, and rotary gain stages (`zdj_soundcard_dsp_gain_pan.c`). Control updates are interpolated over micro-frame sample buffers, eliminating zipper artifacts during rapid fader cuts.

If clicking persists:
1. Verify system firmware version under `Control Panels > Settings > System Info`.
2. Inspect the **DSP Performance Telemetry** view. If audio buffer margin drops below 15%, check background process activity.

## Developer Diagnostics Submenu (PR #33)

Accessed via `Control Panels > Settings > Developer`:
* **Crash Notification Dialog:** When enabled, an unexpected software restart halts execution at a diagnostic screen rather than performing a silent restart.
* **Crash Log Inspection:** Reads backtrace dumps from `/var/log/zdj_crash.log`. Displays register states, faulting memory addresses, and function call stacks.
* **DSP Telemetry:** Displays real-time metrics for:
  * Audio thread execution time (target: < 2.5 ms per 512-sample frame).
  * Cortex-M7 shared memory ring buffer margin.
  * System memory usage and CPU core thermals.

## Hardware Recovery & Config-Mode Boot

If firmware corruption or an interrupted system update prevents normal boot:

### Procedure: Entering Emergency Recovery Mode

1. Hold `POWER` for 6 seconds until all LEDs extinguish. Power turns off completely.
2. Press and hold both `DECK 1` and `DECK 2` Select buttons simultaneously.
3. Press and hold `POWER` for 2 seconds while maintaining pressure on `DECK 1` and `DECK 2`.
4. Release `POWER`. Continue holding `DECK 1` and `DECK 2` until the OLED display illuminates:  
   `=== DRIFT ZERO RECOVERY / CONFIG MODE ===`
5. Connect Zero to a host workstation via USB-C. Zero mounts an emergency read-only recovery partition containing `factory_image.pkg` and `recovery.log`.
6. Copy an official release firmware package into the `update/` directory.
7. Reboot the console. The bootloader verifies package cryptographic signatures and executes automated flash recovery.
