#!/usr/bin/env python3
"""
Generate animated GIF mockups of the Drift Zero 128x64 OLED display.
Renders pixel-styled 384x192 animated GIFs for manual documentation.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ASSETS_DIR = os.path.join(SCRIPT_DIR, "assets")
DOWNLOADS_DIR = os.path.expanduser(r"C:\Users\justi\Downloads")

W, H = 384, 192

# Palette
BG_COLOR = (7, 10, 15)       # Deep OLED black
BEZEL_COLOR = (24, 34, 52)   # Dark frame
CYAN_MAIN = (56, 189, 248)   # Primary OLED cyan
CYAN_DIM = (14, 116, 144)    # Inactive/tick cyan
CYAN_BRIGHT = (186, 230, 253)# Glowing white/cyan
AMBER = (245, 158, 11)       # Warning / Hotcue amber
WHITE = (255, 255, 255)
GREEN = (52, 211, 153)

try:
    FONT_MAIN = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 13)
    FONT_SMALL = ImageFont.truetype(r"C:\Windows\Fonts\consola.ttf", 11)
    FONT_BOLD = ImageFont.truetype(r"C:\Windows\Fonts\consolab.ttf", 13)
except Exception:
    FONT_MAIN = ImageFont.load_default()
    FONT_SMALL = ImageFont.load_default()
    FONT_BOLD = ImageFont.load_default()

def draw_bezel(draw):
    draw.rectangle([0, 0, W - 1, H - 1], outline=BEZEL_COLOR, width=3)
    draw.rectangle([3, 3, W - 4, H - 4], outline=(15, 23, 42), width=1)

def create_playback_gif():
    frames = []
    total_frames = 16
    for f in range(total_frames):
        img = Image.new("RGB", (W, H), BG_COLOR)
        draw = ImageDraw.Draw(img)
        draw_bezel(draw)

        # Header D1
        draw.text((12, 10), "[D1] > PLAY", fill=GREEN, font=FONT_BOLD)
        draw.text((115, 10), "124.00 BPM", fill=CYAN_BRIGHT, font=FONT_MAIN)
        draw.text((215, 10), "[=] 11B", fill=CYAN_MAIN, font=FONT_BOLD)
        draw.text((290, 10), "PAGE: [EQ]", fill=WHITE, font=FONT_SMALL)

        # Waveform D1 (scrolling)
        offset1 = (f * 6) % 360
        wf_y1 = 38
        draw.line([12, wf_y1, 372, wf_y1], fill=CYAN_DIM, width=1)
        for x in range(12, 372, 4):
            val = math.sin((x + offset1) * 0.1) * 10 + math.cos((x + offset1) * 0.25) * 5
            h = abs(val) + 3
            draw.line([x, wf_y1 - h, x, wf_y1 + h], fill=CYAN_MAIN, width=2)
            # Beatgrid ticks every 48 pixels
            if (x + offset1) % 48 < 4:
                draw.line([x, wf_y1 + 10, x, wf_y1 + 15], fill=CYAN_BRIGHT, width=2)

        # Cuepoint flag on D1
        cue_x = 120 - (f * 2) % 100
        if cue_x > 20:
            draw.rectangle([cue_x, wf_y1 - 18, cue_x + 16, wf_y1 - 8], fill=AMBER)
            draw.text((cue_x + 4, wf_y1 - 20), "1", fill=(0, 0, 0), font=FONT_SMALL)
            draw.line([cue_x, wf_y1 - 8, cue_x, wf_y1], fill=AMBER, width=1)

        # Playhead hairline
        draw.line([140, 26, 140, 56], fill=WHITE, width=1)

        # Center: EXT DECK
        draw.line([12, 68, 372, 68], fill=BEZEL_COLOR, width=1)
        draw.text((12, 72), "[EXT] LINE IN (1/2)", fill=CYAN_DIM, font=FONT_SMALL)
        draw.text((170, 72), "CLK: 124.0", fill=CYAN_MAIN, font=FONT_SMALL)
        draw.text((280, 72), "GAIN: +0.0dB", fill=CYAN_DIM, font=FONT_SMALL)

        # D2 Section
        draw.line([12, 94, 372, 94], fill=BEZEL_COLOR, width=1)
        draw.text((12, 98), "[D2] > PLAY", fill=GREEN, font=FONT_BOLD)
        draw.text((115, 98), "124.00 BPM", fill=CYAN_BRIGHT, font=FONT_MAIN)
        draw.text((215, 98), "[+] 12B", fill=AMBER, font=FONT_BOLD)
        draw.text((280, 98), "SYNC: LOCKED", fill=GREEN, font=FONT_SMALL)

        # Waveform D2 (scrolling synced)
        offset2 = (f * 6 + 18) % 360
        wf_y2 = 135
        draw.line([12, wf_y2, 372, wf_y2], fill=CYAN_DIM, width=1)
        for x in range(12, 372, 4):
            val = math.sin((x + offset2) * 0.12) * 9 + math.cos((x + offset2) * 0.3) * 6
            h = abs(val) + 3
            draw.line([x, wf_y2 - h, x, wf_y2 + h], fill=(6, 182, 212), width=2)
            if (x + offset2) % 48 < 4:
                draw.line([x, wf_y2 + 10, x, wf_y2 + 15], fill=CYAN_BRIGHT, width=2)

        # Playhead hairline D2
        draw.line([140, 122, 140, 152], fill=WHITE, width=1)

        # Footer telemetry
        draw.line([12, 168, 372, 168], fill=BEZEL_COLOR, width=1)
        bar_num = 16 + (f // 4)
        beat_num = (f % 4) + 1
        draw.text((12, 172), f"PHRASE: BAR {bar_num}.{beat_num}", fill=CYAN_MAIN, font=FONT_SMALL)
        draw.text((260, 172), "MASTER MIX: L/R OK", fill=WHITE, font=FONT_SMALL)

        frames.append(img)

    out_path = os.path.join(ASSETS_DIR, "dj_playback_animated.gif")
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=120, loop=0)
    print(f"Saved: {out_path}")
    return out_path

def create_bass_swap_gif():
    frames = []
    total_frames = 20
    for f in range(total_frames):
        img = Image.new("RGB", (W, H), BG_COLOR)
        draw = ImageDraw.Draw(img)
        draw_bezel(draw)

        # Header
        draw.text((12, 10), "[D1] 3-BAND ISOLATOR EQ", fill=CYAN_BRIGHT, font=FONT_BOLD)
        draw.text((280, 10), "PAGE: [1/5]", fill=CYAN_DIM, font=FONT_SMALL)
        draw.line([12, 28, 372, 28], fill=BEZEL_COLOR, width=1)

        is_swap = f >= 8 and f <= 16
        is_flash = (f >= 7 and f <= 9)

        if is_swap:
            swap_bg = (30, 58, 138) if is_flash else (15, 23, 42)
            draw.rectangle([12, 34, 372, 54], fill=swap_bg, outline=AMBER if is_flash else CYAN_MAIN)
            draw.text((60, 36), ">>> BASS SWAP: D1 <-> D2 ACTIVE <<<", fill=WHITE if is_flash else AMBER, font=FONT_BOLD)
        else:
            draw.rectangle([12, 34, 372, 54], fill=(15, 23, 42), outline=CYAN_DIM)
            draw.text((70, 36), "HOLD TONE 1 + TURN: EXECUTE SWAP", fill=CYAN_MAIN, font=FONT_SMALL)

        # Deck 1 Low
        draw.text((16, 68), "D1 LOW BASS:", fill=WHITE, font=FONT_MAIN)
        d1_low = 180 if f < 8 else 10  # Drops from full to -inf
        draw.rectangle([130, 68, 130 + d1_low, 80], fill=(239, 68, 68) if f < 8 else CYAN_DIM)
        d1_txt = "  0.0 dB" if f < 8 else "-inf dB (KILL)"
        draw.text((320, 68), d1_txt, fill=CYAN_BRIGHT if f < 8 else CYAN_DIM, font=FONT_SMALL)

        # Deck 2 Low
        draw.text((16, 94), "D2 LOW BASS:", fill=WHITE, font=FONT_MAIN)
        d2_low = 10 if f < 8 else 180  # Jumps from -inf to full
        draw.rectangle([130, 94, 130 + d2_low, 106], fill=CYAN_DIM if f < 8 else GREEN)
        d2_txt = "-inf dB (KILL)" if f < 8 else "  0.0 dB (DROP!)"
        draw.text((320, 94), d2_txt, fill=CYAN_DIM if f < 8 else GREEN, font=FONT_SMALL)

        # Mid and High bands (stable)
        draw.text((16, 122), "D1 MID RANGE:", fill=CYAN_DIM, font=FONT_MAIN)
        draw.rectangle([130, 122, 260, 132], fill=CYAN_MAIN)
        draw.text((320, 122), "  0.0 dB", fill=CYAN_DIM, font=FONT_SMALL)

        draw.text((16, 144), "D1 HIGH TREBLE:", fill=CYAN_DIM, font=FONT_MAIN)
        draw.rectangle([130, 144, 280, 154], fill=CYAN_MAIN)
        draw.text((320, 144), " +1.5 dB", fill=CYAN_DIM, font=FONT_SMALL)

        # Bottom status
        draw.line([12, 168, 372, 168], fill=BEZEL_COLOR, width=1)
        sub_state = "SUB-BASS ON DECK 1" if f < 8 else "SUB-BASS ON DECK 2 (ZERO MUD)"
        draw.text((16, 172), f"STATUS: {sub_state}", fill=WHITE, font=FONT_SMALL)

        frames.append(img)

    out_path = os.path.join(ASSETS_DIR, "bass_swap_animated.gif")
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=150, loop=0)
    print(f"Saved: {out_path}")
    return out_path

def create_looper_gif():
    frames = []
    loop_sizes = ["4 BEATS", "4 BEATS", "2 BEATS", "2 BEATS", "1 BEAT", "1/2 BEAT", "1/4 BEAT (STUTTER)", "DROP!"]
    
    for i, size in enumerate(loop_sizes):
        for rep in range(2):
            img = Image.new("RGB", (W, H), BG_COLOR)
            draw = ImageDraw.Draw(img)
            draw_bezel(draw)

            is_drop = (size == "DROP!")
            is_stutter = ("STUTTER" in size)

            # Header
            draw.text((12, 10), "[D1] PERFORMANCE LOOPER", fill=CYAN_BRIGHT, font=FONT_BOLD)
            draw.text((280, 10), "PAGE: [5/5]", fill=CYAN_DIM, font=FONT_SMALL)
            draw.line([12, 28, 372, 28], fill=BEZEL_COLOR, width=1)

            # Center Loop Badge
            box_col = GREEN if not is_drop else WHITE
            draw.rectangle([12, 36, 372, 64], fill=(15, 23, 42), outline=box_col, width=2)
            draw.text((24, 42), "LOOP SIZE:", fill=CYAN_DIM, font=FONT_MAIN)
            draw.text((120, 40), f"[ {size} ]", fill=AMBER if is_stutter else GREEN, font=FONT_BOLD)
            snap_txt = "SNAP: 100%" if not is_drop else "RELEASED"
            draw.text((290, 42), snap_txt, fill=WHITE, font=FONT_SMALL)

            # Waveform visualizer with contracting loop bracket
            wf_y = 110
            draw.line([12, wf_y, 372, wf_y], fill=CYAN_DIM, width=1)
            for x in range(12, 372, 4):
                val = math.sin(x * 0.15 + (rep * 4)) * 12
                h = abs(val) + 4
                draw.line([x, wf_y - h, x, wf_y + h], fill=CYAN_DIM, width=2)

            # Active loop bracket
            bracket_widths = {
                "4 BEATS": 160,
                "2 BEATS": 80,
                "1 BEAT": 40,
                "1/2 BEAT": 20,
                "1/4 BEAT (STUTTER)": 10,
                "DROP!": 0
            }
            bw = bracket_widths[size]
            lx = 140
            if bw > 0:
                draw.rectangle([lx, wf_y - 20, lx + bw, wf_y + 20], outline=AMBER if is_stutter else CYAN_BRIGHT, width=2)
                # Playhead oscillating inside bracket
                ph_x = lx + ((rep * 30) % bw)
                draw.line([ph_x, wf_y - 24, ph_x, wf_y + 24], fill=WHITE, width=2)

            # Telemetry footer
            draw.line([12, 164, 372, 164], fill=BEZEL_COLOR, width=1)
            action_txt = "TURN TONE 1: DIVIDE LOOP" if not is_drop else "TONE 1 PRESSED: RESUME LINEAR PLAY"
            draw.text((16, 170), f"ACTION: {action_txt}", fill=CYAN_MAIN if not is_drop else GREEN, font=FONT_SMALL)

            frames.append(img)

    out_path = os.path.join(ASSETS_DIR, "looper_stutter_animated.gif")
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=180, loop=0)
    print(f"Saved: {out_path}")
    return out_path

def create_library_gif():
    frames = []
    songs = [
        ("01. Solar Fields - Sol", "124.0", "[=] 11B", "PERFECT MATCH", CYAN_MAIN),
        ("02. Tycho - Awake (Edit)", "124.0", "[+] 12B", "ENERGY BOOST", GREEN),
        ("03. Jon Hopkins - Open Eye", "124.0", "[~] 11A", "MODE SWAP", AMBER),
        ("04. Bicep - Glue", "124.0", "[Δ]  8A", "MOOD SHIFT", (244, 63, 94)),
        ("05. Floating Points - LesAlp", "128.0", "[-] 10B", "ENERGY DROP", CYAN_DIM),
    ]

    for sel_idx in range(len(songs)):
        for step in range(2):
            img = Image.new("RGB", (W, H), BG_COLOR)
            draw = ImageDraw.Draw(img)
            draw_bezel(draw)

            # Header
            draw.text((12, 10), "[LIBRARY: SONGS]", fill=CYAN_BRIGHT, font=FONT_BOLD)
            draw.text((260, 10), "TARGET: [DECK 2]", fill=AMBER, font=FONT_SMALL)
            draw.line([12, 28, 372, 28], fill=BEZEL_COLOR, width=1)

            # Song items
            y = 36
            for idx, (title, bpm, key, badge_txt, col) in enumerate(songs):
                is_selected = (idx == sel_idx)
                if is_selected:
                    draw.rectangle([12, y - 2, 372, y + 20], fill=(22, 36, 60), outline=CYAN_MAIN)
                    cursor = ">"
                    txt_col = WHITE
                else:
                    cursor = " "
                    txt_col = (148, 163, 184)

                draw.text((16, y), f"{cursor} {title[:20]}", fill=txt_col, font=FONT_SMALL)
                draw.text((215, y), bpm, fill=CYAN_DIM if not is_selected else CYAN_BRIGHT, font=FONT_SMALL)
                draw.text((265, y), key, fill=col if is_selected else CYAN_DIM, font=FONT_BOLD)
                y += 24

            # Footer harmonic details
            draw.line([12, 164, 372, 164], fill=BEZEL_COLOR, width=1)
            _, _, cur_key, cur_desc, cur_col = songs[sel_idx]
            draw.text((16, 170), f"SELECTED: {cur_key} -> {cur_desc}", fill=cur_col, font=FONT_BOLD)

            frames.append(img)

    out_path = os.path.join(ASSETS_DIR, "harmonic_library_animated.gif")
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=260, loop=0)
    print(f"Saved: {out_path}")
    return out_path

def create_beatgrid_gif():
    frames = []
    total_frames = 16
    for f in range(total_frames):
        img = Image.new("RGB", (W, H), BG_COLOR)
        draw = ImageDraw.Draw(img)
        draw_bezel(draw)

        # Header
        draw.text((12, 10), "[D1: BEATGRID CALIBRATION]", fill=CYAN_BRIGHT, font=FONT_BOLD)
        draw.text((280, 10), "BPM: [124.00]", fill=WHITE, font=FONT_SMALL)
        draw.line([12, 28, 372, 28], fill=BEZEL_COLOR, width=1)

        is_anchored = f >= 6
        is_flash = (f >= 5 and f <= 7)

        # Waveform zoomed 400%
        wf_y = 95
        draw.line([12, wf_y, 372, wf_y], fill=CYAN_DIM, width=1)

        # Spikes (Kick transients)
        peaks = [60, 140, 220, 300]
        for p in peaks:
            # Draw realistic kick drum transient spike
            draw.polygon([(p - 12, wf_y), (p, wf_y - 45), (p + 12, wf_y)], fill=(30, 58, 138), outline=CYAN_MAIN)
            draw.polygon([(p - 12, wf_y), (p, wf_y + 45), (p + 12, wf_y)], fill=(30, 58, 138), outline=CYAN_MAIN)

        # Downbeat Anchor at first peak (x=60)
        if is_anchored:
            draw.line([60, wf_y - 50, 60, wf_y + 50], fill=AMBER, width=2)
            draw.rectangle([60, wf_y - 62, 140, wf_y - 48], fill=AMBER)
            draw.text((64, wf_y - 63), "ANCHOR: BAR 1.1", fill=(0, 0, 0), font=FONT_SMALL)
        else:
            # Scrub cursor moving to peak 60
            scrub_x = 30 + (f * 5)
            draw.line([scrub_x, wf_y - 50, scrub_x, wf_y + 50], fill=WHITE, width=1)

        # Beatgrid lines (stretching into alignment after anchor)
        grid_spacing = 80 if is_anchored else 74 + f  # stretching to match 80px peaks
        start_x = 60 if is_anchored else 40
        for gx in range(start_x, 370, int(grid_spacing)):
            draw.line([gx, wf_y - 25, gx, wf_y + 25], fill=GREEN if is_anchored else (239, 68, 68), width=1)
            draw.text((gx + 2, wf_y + 28), "|", fill=GREEN if is_anchored else (239, 68, 68), font=FONT_SMALL)

        # Bottom telemetry
        draw.line([12, 164, 372, 164], fill=BEZEL_COLOR, width=1)
        state_txt = "TONE 1: PRESS TO LOCK ANCHOR" if not is_anchored else "GRID LOCKED: SAMPLE-ACCURATE PHASE"
        draw.text((16, 170), f"STATUS: {state_txt}", fill=CYAN_BRIGHT if not is_anchored else GREEN, font=FONT_SMALL)

        frames.append(img)

    out_path = os.path.join(ASSETS_DIR, "beatgrid_calibrate_animated.gif")
    frames[0].save(out_path, save_all=True, append_images=frames[1:], duration=160, loop=0)
    print(f"Saved: {out_path}")
    return out_path

if __name__ == "__main__":
    print("Generating animated OLED mockup GIFs...")
    f1 = create_playback_gif()
    f2 = create_bass_swap_gif()
    f3 = create_looper_gif()
    f4 = create_library_gif()
    f5 = create_beatgrid_gif()
    
    # Also copy to Downloads so user can preview directly
    for f in [f1, f2, f3, f4, f5]:
        target = os.path.join(DOWNLOADS_DIR, os.path.basename(f))
        import shutil
        shutil.copy2(f, target)
        print(f"Copied to Downloads: {target}")

    print("All animated GIFs generated successfully.")
