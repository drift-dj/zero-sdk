#!/usr/bin/env python3
"""
Drift Zero User Manual Builder & Formatter
Converts modular manual source sections into a unified Markdown reference,
a modern, responsive, typography-first HTML manual, and a print-ready PDF.
"""

import os
import sys
import subprocess
import datetime
import re
import shutil
import markdown

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SECTIONS_DIR = os.path.join(SCRIPT_DIR, "sections")
ASSETS_DIR = os.path.join(SCRIPT_DIR, "assets")
OUTPUT_MD = os.path.join(SCRIPT_DIR, "Drift_Zero_User_Manual_v0.6.0.md")
OUTPUT_HTML = os.path.join(SCRIPT_DIR, "Drift_Zero_User_Manual_v0.6.0.html")
DOWNLOADS_DIR = os.path.expanduser(r"C:\Users\justi\Downloads")

SECTION_FILES = [
    "01_system_overview_and_hardware.md",
    "02_dj_playback_mode.md",
    "03_function_knob_pages.md",
    "04_library_and_harmonic_mixing.md",
    "05_music_import_and_rekordbox.md",
    "06_song_editing_cuepoints_and_beatgrid.md",
    "07_control_panels_soundcard_and_usb.md",
    "08_troubleshooting_and_recovery.md",
    "09_glossary_of_dj_and_hardware_terms.md",
]

def get_git_info():
    repo_dir = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
    try:
        sha = subprocess.check_output(["git", "-C", repo_dir, "rev-parse", "--short", "HEAD"], text=True).strip()
        branch = subprocess.check_output(["git", "-C", repo_dir, "rev-parse", "--abbrev-ref", "HEAD"], text=True).strip()
        tag = subprocess.check_output(["git", "-C", repo_dir, "describe", "--tags", "--always"], text=True).strip()
        return sha, branch, tag
    except Exception:
        return "397d19e", "release/0.6.0", "v0.6.0"

def build_markdown():
    sha, branch, tag = get_git_info()
    now_str = datetime.datetime.now().strftime("%Y-%m-%d")

    header = f"""# Drift Zero User Manual
### Official Operational & Technical Reference Manual
**Firmware Version:** `0.6.0` | **Git Commit:** `{sha}` (`{branch}`) | **Revision Date:** `{now_str}`  
**Hardware Platform:** Drift Zero DJ Console (Model ZDJ-1) | **Target SDK:** [`drift-dj/zero-sdk`](https://github.com/drift-dj/zero-sdk)

## Table of Contents
1. [System Overview & Hardware Architecture](#1-system-overview--hardware-architecture)
2. [DJ Playback Engine & Live Workflow](#2-dj-playback-engine--live-workflow)
3. [Function Knob Pages & Audio Processing](#3-function-knob-pages--audio-processing)
4. [Library Navigation & Harmonic Mixing Engine](#4-library-navigation--harmonic-mixing-engine)
5. [Audio Import Workflows & Rekordbox XML Integration](#5-audio-import-workflows--rekordbox-xml-integration)
6. [Track Metadata, Cuepoints & Beatgrid Calibration](#6-track-metadata-cuepoints--beatgrid-calibration)
7. [Control Panels: Recording, Soundcard Routing & USB Subsystems](#7-control-panels-recording-soundcard-routing--usb-subsystems)
8. [Troubleshooting, Diagnostics & Recovery](#8-troubleshooting-diagnostics--recovery)
9. [Glossary of DJ & Hardware Terms](#9-glossary-of-dj--hardware-terms)

"""
    combined_content = [header]

    for fname in SECTION_FILES:
        fpath = os.path.join(SECTIONS_DIR, fname)
        if not os.path.exists(fpath):
            print(f"Error: Missing section file {fpath}", file=sys.stderr)
            sys.exit(1)
        with open(fpath, "r", encoding="utf-8") as f:
            combined_content.append(f.read().strip())
            combined_content.append("\n\n")

    full_md = "\n".join(combined_content)
    with open(OUTPUT_MD, "w", encoding="utf-8") as f:
        f.write(full_md)
    print(f"Generated Markdown: {OUTPUT_MD}")
    return full_md

def transform_callouts(html_content):
    # Transform GitHub-style blockquote alerts: [!NOTE], [!WARNING], [!TIP], [!IMPORTANT]
    callout_icons = {
        "NOTE": '<svg class="callout-icon" viewBox="0 0 16 16" width="16" height="16" fill="currentColor"><path d="M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8Zm8-6.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13ZM6.5 7.75A.75.75 0 0 1 7.25 7h1a.75.75 0 0 1 .75.75v2.75h.25a.75.75 0 0 1 0 1.5h-2a.75.75 0 0 1 0-1.5h.25v-2h-.25a.75.75 0 0 1-.75-.75ZM8 6a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z"/></svg>',
        "TIP": '<svg class="callout-icon" viewBox="0 0 16 16" width="16" height="16" fill="currentColor"><path d="M8 1.5c-2.363 0-4 1.69-4 3.75 0 .984.424 1.625.984 2.304l.214.253c.223.264.47.556.673.848.284.411.537.896.621 1.49a.75.75 0 0 1-1.484.211c-.04-.282-.163-.547-.37-.847a8.456 8.456 0 0 0-.542-.68c-.604-.71-1.346-1.584-1.346-3.279C2.75 2.563 5.068 0 8 0s5.25 2.563 5.25 5.25c0 1.695-.741 2.569-1.346 3.28-.167.195-.341.4-.542.68-.207.3-.33.565-.37.847a.75.75 0 0 1-1.484-.212c.084-.593.337-1.078.621-1.489.203-.292.45-.584.673-.848.075-.088.147-.173.213-.253.561-.679.985-1.32.985-2.304 0-2.06-1.637-3.75-4-3.75ZM6 12.5a.75.75 0 0 1 .75-.75h2.5a.75.75 0 0 1 0 1.5h-2.5a.75.75 0 0 1-.75-.75Zm.75 2.25a.75.75 0 0 0 0 1.5h1.5a.75.75 0 0 0 0-1.5h-1.5Z"/></svg>',
        "IMPORTANT": '<svg class="callout-icon" viewBox="0 0 16 16" width="16" height="16" fill="currentColor"><path d="M0 1.75C0 .784.784 0 1.75 0h12.5C15.216 0 16 .784 16 1.75v9.5A1.75 1.75 0 0 1 14.25 13H8.06l-2.573 2.573A1.458 1.458 0 0 1 3 14.543V13H1.75A1.75 1.75 0 0 1 0 11.25Zm1.75-.25a.25.25 0 0 0-.25.25v9.5c0 .138.112.25.25.25h2a.75.75 0 0 1 .75.75v2.19l2.72-2.72a.749.749 0 0 1 .53-.22h6.5a.25.25 0 0 0 .25-.25v-9.5a.25.25 0 0 0-.25-.25Zm7 2.25v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 9a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z"/></svg>',
        "WARNING": '<svg class="callout-icon" viewBox="0 0 16 16" width="16" height="16" fill="currentColor"><path d="M6.457 1.047c.659-1.234 2.427-1.234 3.086 0l6.082 11.378A1.75 1.75 0 0 1 14.082 15H1.918a1.75 1.75 0 0 1-1.543-2.575Zm1.763.707a.25.25 0 0 0-.44 0L1.698 13.132a.25.25 0 0 0 .22.368h12.164a.25.25 0 0 0 .22-.368Zm.53 3.996v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 11a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z"/></svg>',
    }

    def replace_callout(match):
        ctype = match.group(1).upper()
        content = match.group(2).strip()
        icon = callout_icons.get(ctype, callout_icons["NOTE"])
        css_class = ctype.lower()
        if not content.startswith("<p>") and not content.startswith("<div>"):
            content = f"<p>{content}"
        return f"""<div class="callout callout-{css_class}">
    <div class="callout-header">{icon} <span>{ctype}</span></div>
    <div class="callout-body">{content}</div>
</div>"""

    pattern = re.compile(
        r'<blockquote>\s*<p>\s*\[!(NOTE|TIP|IMPORTANT|WARNING)\][\s\:\-]*\n?(.*?)</blockquote>',
        re.DOTALL | re.IGNORECASE
    )
    return pattern.sub(replace_callout, html_content)

def transform_mockups(html_content):
    # Enhance GIF images into authentic OLED hardware simulator displays
    def replace_img(match):
        alt = match.group(1)
        src = match.group(2)
        if "animated.gif" in src or "assets/" in src:
            return f"""<div class="oled-simulator">
    <div class="oled-bezel">
        <div class="oled-status-bar">
            <span class="oled-dot"></span>
            <span class="oled-label">DRIFT ZERO OLED MONITOR (128x64)</span>
            <span class="oled-freq">60 FPS</span>
        </div>
        <div class="oled-screen-frame">
            <img src="{src}" alt="{alt}" class="oled-display-img">
        </div>
        <div class="oled-caption">{alt}</div>
    </div>
</div>"""
        return match.group(0)

    pattern = re.compile(r'<p><img\s+alt="([^"]*)"\s+src="([^"]*)"\s*/?></p>')
    return pattern.sub(replace_img, html_content)

def transform_ascii_diagrams(html_content):
    # Enhance ascii diagram pre blocks
    def replace_pre(match):
        code_text = match.group(1)
        if "+---" in code_text or "+===" in code_text:
            return f"""<div class="console-diagram">
    <div class="console-titlebar">
        <span class="console-pill red"></span>
        <span class="console-pill yellow"></span>
        <span class="console-pill green"></span>
        <span class="console-title">Hardware Telemetry Display</span>
    </div>
    <pre class="console-screen"><code>{code_text}</code></pre>
</div>"""
        return match.group(0)

    pattern = re.compile(r'<pre><code(?:\s+class="language-[^"]*")?>(.*?)</code></pre>', re.DOTALL)
    return pattern.sub(replace_pre, html_content)

def transform_hardware_keys(html_content):
    # Convert hardware button/knob references to styled kbd elements outside code blocks
    hw_keys = [
        "TONE 1", "TONE 2", "TONE 3", "JOG", "VOL", "PLAY", "CUE", "NAV/SHIFT",
        "DECK 1", "EXT", "DECK 2", "DECK 1 FADER", "DECK 2 FADER", "CROSSFADER",
        "POWER", "OUTPUT 1", "OUTPUT 2", "INPUT 1", "INPUT 2", "USB-C"
    ]
    parts = re.split(r'(<pre.*?</pre>)', html_content, flags=re.DOTALL)
    for i in range(0, len(parts), 2):
        for key in hw_keys:
            parts[i] = parts[i].replace(f'<code>{key}</code>', f'<kbd class="hw-key">{key}</kbd>')
    return "".join(parts)

def build_toc_sidebar(html_content):
    # Extract h1, h2, h3 headers for the interactive sidebar
    toc_items = []
    # Pattern to match headers with IDs generated by python-markdown
    header_pattern = re.compile(r'<h([1-3])\s+id="([^"]+)">([^<]+)</h\1>', re.IGNORECASE)
    
    for match in header_pattern.finditer(html_content):
        level = int(match.group(1))
        hid = match.group(2)
        title = match.group(3).strip()
        if title == "Table of Contents" or title == "Drift Zero User Manual":
            continue
        toc_items.append((level, hid, title))

    sidebar_html = ['<ul class="toc-nav">']
    for level, hid, title in toc_items:
        cls = f"toc-item toc-h{level}"
        sidebar_html.append(f'  <li class="{cls}"><a href="#{hid}" class="toc-link" data-target="{hid}">{title}</a></li>')
    sidebar_html.append('</ul>')
    return "\n".join(sidebar_html)

def build_html(md_text):
    sha, branch, tag = get_git_info()
    now_str = datetime.datetime.now().strftime("%B %d, %Y")

    # Render markdown with robust extensions
    md_parser = markdown.Markdown(
        extensions=[
            'extra',
            'tables',
            'fenced_code',
            'toc',
            'attr_list',
            'def_list'
        ]
    )
    raw_html = md_parser.convert(md_text)

    # Post-process transforms
    raw_html = transform_callouts(raw_html)
    raw_html = transform_mockups(raw_html)
    raw_html = transform_ascii_diagrams(raw_html)
    raw_html = transform_hardware_keys(raw_html)

    sidebar_toc = build_toc_sidebar(raw_html)

    css = """
    :root {
        --bg-main: #090d16;
        --bg-sidebar: #0d131f;
        --bg-card: #111827;
        --bg-elevated: #1a2234;
        --border-subtle: #1e293b;
        --border-strong: #334155;
        --border-focus: #38bdf8;
        --text-primary: #f8fafc;
        --text-secondary: #cbd5e1;
        --text-muted: #94a3b8;
        --accent-primary: #38bdf8;
        --accent-secondary: #818cf8;
        --accent-emerald: #34d399;
        --accent-rose: #f43f5e;
        --accent-amber: #fbbf24;
        --code-bg: #06090f;
        --kbd-bg: #1e293b;
        --font-sans: -apple-system, BlinkMacSystemFont, "Inter", "Segoe UI", Roboto, sans-serif;
        --font-mono: "JetBrains Mono", Consolas, Menlo, Monaco, monospace;
        --sidebar-w: 320px;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }
    html { scroll-behavior: smooth; }
    body {
        font-family: var(--font-sans);
        background: var(--bg-main);
        color: var(--text-secondary);
        line-height: 1.68;
        font-size: 15px;
        letter-spacing: -0.01em;
        overflow-x: hidden;
    }

    /* Page Layout */
    .app-layout {
        display: flex;
        min-height: 100vh;
    }

    /* Sidebar Navigation */
    .sidebar {
        width: var(--sidebar-w);
        background: var(--bg-sidebar);
        border-right: 1px solid var(--border-subtle);
        position: fixed;
        top: 0;
        bottom: 0;
        left: 0;
        display: flex;
        flex-direction: column;
        z-index: 100;
        overflow-y: auto;
    }
    .sidebar-header {
        padding: 24px 20px 16px;
        border-bottom: 1px solid var(--border-subtle);
        background: var(--bg-sidebar);
        position: sticky;
        top: 0;
        z-index: 10;
    }
    .brand-row {
        display: flex;
        align-items: center;
        gap: 12px;
        margin-bottom: 8px;
    }
    .brand-icon {
        width: 32px;
        height: 32px;
        background: linear-gradient(135deg, var(--accent-primary), var(--accent-secondary));
        border-radius: 6px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: 900;
        color: #000;
        font-size: 14px;
        font-family: var(--font-mono);
    }
    .brand-title {
        font-size: 16px;
        font-weight: 700;
        color: var(--text-primary);
        letter-spacing: 0.5px;
    }
    .badge-row {
        display: flex;
        gap: 6px;
        flex-wrap: wrap;
    }
    .badge {
        font-size: 11px;
        font-family: var(--font-mono);
        padding: 2px 7px;
        border-radius: 4px;
        font-weight: 600;
    }
    .badge-firmware {
        background: rgba(56, 189, 248, 0.15);
        color: var(--accent-primary);
        border: 1px solid rgba(56, 189, 248, 0.3);
    }
    .badge-git {
        background: rgba(129, 140, 248, 0.15);
        color: var(--accent-secondary);
        border: 1px solid rgba(129, 140, 248, 0.3);
    }

    .search-box {
        padding: 12px 20px;
        border-bottom: 1px solid var(--border-subtle);
    }
    .search-input {
        width: 100%;
        background: var(--bg-card);
        border: 1px solid var(--border-subtle);
        border-radius: 6px;
        padding: 8px 12px;
        color: var(--text-primary);
        font-size: 13px;
        font-family: var(--font-sans);
        outline: none;
        transition: border-color 0.15s ease;
    }
    .search-input:focus {
        border-color: var(--accent-primary);
    }

    .sidebar-toc {
        padding: 16px 12px;
        flex: 1;
    }
    .toc-nav { list-style: none; }
    .toc-item { margin-bottom: 2px; }
    .toc-h1 {
        margin-top: 14px;
        margin-bottom: 4px;
        font-weight: 700;
        font-size: 13px;
    }
    .toc-h1 a {
        color: var(--text-primary);
        padding: 6px 10px;
        border-radius: 5px;
    }
    .toc-h2 {
        padding-left: 12px;
        font-size: 12.5px;
    }
    .toc-h2 a {
        color: var(--text-muted);
        padding: 4px 8px;
        border-radius: 4px;
    }
    .toc-h3 {
        padding-left: 24px;
        font-size: 12px;
    }
    .toc-h3 a {
        color: #64748b;
        padding: 3px 6px;
        border-radius: 4px;
    }
    .toc-link {
        display: block;
        text-decoration: none;
        transition: all 0.15s ease;
        line-height: 1.4;
    }
    .toc-link:hover {
        color: var(--accent-primary);
        background: rgba(56, 189, 248, 0.08);
    }
    .toc-link.active {
        color: var(--accent-primary);
        background: rgba(56, 189, 248, 0.15);
        font-weight: 600;
        border-left: 3px solid var(--accent-primary);
    }

    .sidebar-footer {
        padding: 14px 20px;
        border-top: 1px solid var(--border-subtle);
        background: var(--bg-sidebar);
        font-size: 11px;
        color: var(--text-muted);
        display: flex;
        justify-content: space-between;
        align-items: center;
    }
    .sidebar-footer a {
        color: var(--accent-primary);
        text-decoration: none;
    }

    /* Main Content Area */
    .main-wrapper {
        margin-left: var(--sidebar-w);
        flex: 1;
        min-width: 0;
        display: flex;
        justify-content: center;
        padding: 48px 40px 96px;
    }
    .content-container {
        max-width: 860px;
        width: 100%;
    }

    /* Header Title Card */
    .doc-hero {
        background: linear-gradient(180deg, var(--bg-card), var(--bg-sidebar));
        border: 1px solid var(--border-subtle);
        border-radius: 12px;
        padding: 36px 40px;
        margin-bottom: 48px;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.4);
    }
    .doc-hero h1 {
        font-size: 2.2rem;
        font-weight: 800;
        color: #fff;
        margin: 0 0 8px 0;
        letter-spacing: -0.02em;
    }
    .doc-hero h3 {
        font-size: 1.15rem;
        font-weight: 500;
        color: var(--accent-primary);
        margin: 0 0 18px 0;
    }
    .doc-meta-grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
        gap: 12px;
        border-top: 1px solid var(--border-subtle);
        padding-top: 16px;
        font-size: 12.5px;
    }
    .doc-meta-item {
        color: var(--text-muted);
    }
    .doc-meta-item strong {
        color: var(--text-primary);
        display: block;
        margin-bottom: 2px;
    }

    /* Typography */
    h1, h2, h3, h4 {
        color: var(--text-primary);
        font-weight: 700;
        letter-spacing: -0.02em;
    }
    h1 {
        font-size: 1.85rem;
        margin-top: 56px;
        margin-bottom: 20px;
        padding-bottom: 12px;
        border-bottom: 1px solid var(--border-subtle);
        scroll-margin-top: 32px;
    }
    h2 {
        font-size: 1.4rem;
        margin-top: 36px;
        margin-bottom: 14px;
        color: #e2e8f0;
        scroll-margin-top: 32px;
    }
    h3 {
        font-size: 1.15rem;
        margin-top: 28px;
        margin-bottom: 10px;
        color: #cbd5e1;
        scroll-margin-top: 32px;
    }
    h4 {
        font-size: 1.0rem;
        margin-top: 20px;
        margin-bottom: 8px;
        color: var(--accent-primary);
    }
    p { margin-bottom: 1.25em; line-height: 1.7; }
    ul, ol {
        margin-bottom: 1.3em;
        padding-left: 24px;
    }
    li {
        margin-bottom: 0.45em;
        line-height: 1.65;
    }
    li::marker { color: var(--accent-primary); }

    strong { color: var(--text-primary); font-weight: 600; }
    a {
        color: var(--accent-primary);
        text-decoration: none;
        border-bottom: 1px solid transparent;
        transition: border-color 0.15s ease;
    }
    a:hover { border-bottom-color: var(--accent-primary); }

    /* Hardware Keys Styling */
    .hw-key {
        display: inline-block;
        font-family: var(--font-mono);
        font-size: 11.5px;
        font-weight: 700;
        background: #1e293b;
        color: #38bdf8;
        border: 1px solid #475569;
        border-bottom-width: 2px;
        border-radius: 4px;
        padding: 1px 6px;
        margin: 0 2px;
        box-shadow: 0 1px 2px rgba(0,0,0,0.3);
        vertical-align: baseline;
        text-transform: uppercase;
    }

    /* Inline Code */
    code:not([class]) {
        font-family: var(--font-mono);
        background: var(--code-bg);
        color: #7dd3fc;
        padding: 2px 6px;
        border-radius: 4px;
        font-size: 0.88em;
        border: 1px solid rgba(255,255,255,0.08);
    }

    /* Pre & Code Blocks */
    pre {
        background: var(--code-bg);
        border: 1px solid var(--border-subtle);
        border-radius: 8px;
        padding: 18px 20px;
        overflow-x: auto;
        margin: 20px 0 24px;
        font-family: var(--font-mono);
        font-size: 13px;
        line-height: 1.55;
    }
    pre code {
        background: none !important;
        border: none !important;
        padding: 0 !important;
        color: #e2e8f0;
    }

    /* Tables */
    table {
        width: 100%;
        border-collapse: separate;
        border-spacing: 0;
        margin: 24px 0 32px;
        border: 1px solid var(--border-subtle);
        border-radius: 8px;
        overflow: hidden;
        font-size: 13.5px;
    }
    th, td {
        padding: 11px 16px;
        text-align: left;
        border-bottom: 1px solid var(--border-subtle);
    }
    th {
        background: var(--bg-card);
        color: #f8fafc;
        font-weight: 600;
        font-size: 13px;
        letter-spacing: 0.2px;
        border-bottom: 2px solid var(--border-strong);
    }
    tr:last-child td { border-bottom: none; }
    tr:nth-child(even) td { background: rgba(255, 255, 255, 0.015); }
    tr:hover td { background: rgba(56, 189, 248, 0.04); }

    /* Callout Alert Boxes */
    .callout {
        border-radius: 8px;
        padding: 16px 20px;
        margin: 24px 0;
        border-left: 4px solid;
        background: var(--bg-card);
        box-shadow: 0 4px 12px rgba(0,0,0,0.2);
    }
    .callout-header {
        display: flex;
        align-items: center;
        gap: 8px;
        font-size: 12px;
        font-weight: 700;
        letter-spacing: 0.8px;
        text-transform: uppercase;
        margin-bottom: 8px;
    }
    .callout-body {
        font-size: 14px;
        line-height: 1.6;
        color: #cbd5e1;
    }
    .callout-body p:last-child { margin-bottom: 0; }

    .callout-note {
        border-left-color: var(--accent-primary);
        background: rgba(56, 189, 248, 0.06);
    }
    .callout-note .callout-header { color: var(--accent-primary); }

    .callout-tip {
        border-left-color: var(--accent-emerald);
        background: rgba(52, 211, 153, 0.06);
    }
    .callout-tip .callout-header { color: var(--accent-emerald); }

    .callout-important {
        border-left-color: var(--accent-secondary);
        background: rgba(129, 140, 248, 0.06);
    }
    .callout-important .callout-header { color: var(--accent-secondary); }

    .callout-warning {
        border-left-color: var(--accent-rose);
        background: rgba(244, 63, 94, 0.06);
    }
    .callout-warning .callout-header { color: var(--accent-rose); }

    /* OLED Simulator Frame */
    .oled-simulator {
        margin: 32px 0;
        display: flex;
        justify-content: center;
    }
    .oled-bezel {
        background: #05080f;
        border: 2px solid #1e293b;
        border-radius: 10px;
        padding: 14px 18px 12px;
        box-shadow: 0 8px 30px rgba(0, 0, 0, 0.7), 0 0 1px 1px rgba(255,255,255,0.05);
        max-width: 640px;
        width: 100%;
    }
    .oled-status-bar {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 10px;
        font-family: var(--font-mono);
        font-size: 10.5px;
        color: #64748b;
        letter-spacing: 0.5px;
    }
    .oled-dot {
        width: 7px;
        height: 7px;
        background: #38bdf8;
        border-radius: 50%;
        box-shadow: 0 0 8px #38bdf8;
        display: inline-block;
    }
    .oled-screen-frame {
        background: #000;
        border: 1px solid #1e293b;
        border-radius: 4px;
        padding: 6px;
        display: flex;
        justify-content: center;
        box-shadow: inset 0 0 20px rgba(0,0,0,0.9);
    }
    .oled-display-img {
        max-width: 100%;
        height: auto;
        border-radius: 2px;
        image-rendering: pixelated;
        box-shadow: 0 0 16px rgba(56, 189, 248, 0.2);
    }
    .oled-caption {
        font-family: var(--font-mono);
        font-size: 11px;
        color: #94a3b8;
        text-align: center;
        margin-top: 10px;
        letter-spacing: 0.3px;
    }

    /* Console Diagram Frame */
    .console-diagram {
        margin: 28px 0;
        background: var(--code-bg);
        border: 1px solid var(--border-strong);
        border-radius: 8px;
        overflow: hidden;
        box-shadow: 0 6px 20px rgba(0,0,0,0.5);
    }
    .console-titlebar {
        background: #0f172a;
        padding: 8px 14px;
        border-bottom: 1px solid var(--border-subtle);
        display: flex;
        align-items: center;
        gap: 6px;
    }
    .console-pill {
        width: 10px;
        height: 10px;
        border-radius: 50%;
        display: inline-block;
    }
    .console-pill.red { background: #f43f5e; }
    .console-pill.yellow { background: #fbbf24; }
    .console-pill.green { background: #34d399; }
    .console-title {
        font-family: var(--font-mono);
        font-size: 11px;
        color: #94a3b8;
        margin-left: 8px;
        letter-spacing: 0.5px;
    }
    .console-screen {
        margin: 0 !important;
        border: none !important;
        border-radius: 0 !important;
        background: transparent !important;
        padding: 16px 20px !important;
    }

    /* Print Stylesheet */
    @media print {
        body {
            background: #fff !important;
            color: #111 !important;
            font-size: 11pt;
        }
        .sidebar { display: none !important; }
        .main-wrapper {
            margin: 0 !important;
            padding: 0 !important;
            width: 100% !important;
        }
        .content-container { max-width: 100% !important; }
        .doc-hero {
            background: none !important;
            border: 1px solid #ccc !important;
            box-shadow: none !important;
            padding: 20px !important;
            page-break-after: avoid;
        }
        .doc-hero h1 { color: #000 !important; }
        .doc-hero h3 { color: #333 !important; }
        h1 {
            break-before: page;
            color: #000 !important;
            border-bottom: 2px solid #000 !important;
        }
        h2, h3, h4 { color: #111 !important; }
        pre, .console-diagram {
            border: 1px solid #bbb !important;
            background: #f8f8f8 !important;
            page-break-inside: avoid;
        }
        pre code { color: #222 !important; }
        .callout {
            border: 1px solid #aaa !important;
            border-left-width: 4px !important;
            background: #fbfbfb !important;
            page-break-inside: avoid;
        }
        .callout-body { color: #222 !important; }
        .oled-bezel {
            border: 1px solid #ccc !important;
            background: #f9f9f9 !important;
            box-shadow: none !important;
            page-break-inside: avoid;
        }
        table {
            border: 1px solid #aaa !important;
            page-break-inside: avoid;
        }
        th { background: #eee !important; color: #000 !important; }
        td { color: #222 !important; }
        .hw-key {
            background: #eee !important;
            color: #111 !important;
            border-color: #999 !important;
        }
    }
    """

    js = """
    document.addEventListener("DOMContentLoaded", () => {
        // Scrollspy for sidebar TOC links
        const links = Array.from(document.querySelectorAll('.toc-link'));
        const headers = Array.from(document.querySelectorAll('h1[id], h2[id], h3[id]'));
        
        function updateActiveLink() {
            const scrollPos = window.scrollY + 100;
            let currentHeader = null;
            
            for (let i = 0; i < headers.length; i++) {
                if (headers[i].offsetTop <= scrollPos) {
                    currentHeader = headers[i];
                } else {
                    break;
                }
            }
            
            if (currentHeader) {
                const targetId = currentHeader.id;
                links.forEach(link => {
                    if (link.getAttribute('data-target') === targetId) {
                        link.classList.add('active');
                        // Auto-scroll sidebar if active item is out of view
                        const linkRect = link.getBoundingClientRect();
                        const sidebarRect = document.querySelector('.sidebar').getBoundingClientRect();
                        if (linkRect.top < sidebarRect.top || linkRect.bottom > sidebarRect.bottom) {
                            link.scrollIntoView({ block: 'nearest' });
                        }
                    } else {
                        link.classList.remove('active');
                    }
                });
            }
        }
        
        window.addEventListener('scroll', updateActiveLink, { passive: true });
        updateActiveLink();

        // Search / Filter
        const searchInput = document.getElementById('search-input');
        if (searchInput) {
            searchInput.addEventListener('input', (e) => {
                const query = e.target.value.toLowerCase().trim();
                const tocItems = document.querySelectorAll('.toc-item');
                tocItems.forEach(item => {
                    const text = item.textContent.toLowerCase();
                    if (!query || text.includes(query)) {
                        item.style.display = '';
                    } else {
                        item.style.display = 'none';
                    }
                });
            });
        }
    });
    """

    full_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Drift Zero User Manual (v0.6.0)</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&family=JetBrains+Mono:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>{css}</style>
</head>
<body>
<div class="app-layout">
    <aside class="sidebar">
        <div class="sidebar-header">
            <div class="brand-row">
                <div class="brand-icon">Z0</div>
                <div class="brand-title">DRIFT ZERO</div>
            </div>
            <div class="badge-row">
                <span class="badge badge-firmware">FIRMWARE v0.6.0</span>
                <span class="badge badge-git">{sha}</span>
            </div>
        </div>
        <div class="search-box">
            <input type="text" id="search-input" class="search-input" placeholder="Filter sections... (/)">
        </div>
        <div class="sidebar-toc">
            {sidebar_toc}
        </div>
        <div class="sidebar-footer">
            <span>Model ZDJ-1</span>
            <a href="javascript:window.print()">Print / PDF</a>
        </div>
    </aside>

    <main class="main-wrapper">
        <div class="content-container">
            <header class="doc-hero">
                <h1>Drift Zero User Manual</h1>
                <h3>Official Operational & Technical Reference Manual</h3>
                <div class="doc-meta-grid">
                    <div class="doc-meta-item">
                        <strong>Firmware Version</strong>
                        <span>0.6.0 Production</span>
                    </div>
                    <div class="doc-meta-item">
                        <strong>Hardware Platform</strong>
                        <span>Model ZDJ-1 Dual Deck</span>
                    </div>
                    <div class="doc-meta-item">
                        <strong>Git Reference</strong>
                        <span>{sha} ({branch})</span>
                    </div>
                    <div class="doc-meta-item">
                        <strong>Revision Date</strong>
                        <span>{now_str}</span>
                    </div>
                </div>
            </header>

            <article class="manual-body">
                {raw_html}
            </article>
        </div>
    </main>
</div>
<script>{js}</script>
</body>
</html>"""

    with open(OUTPUT_HTML, "w", encoding="utf-8") as f:
        f.write(full_html)
    print(f"Generated HTML: {OUTPUT_HTML}")

    # Copy to Downloads
    dl_assets = os.path.join(DOWNLOADS_DIR, "assets")
    shutil.copytree(ASSETS_DIR, dl_assets, dirs_exist_ok=True)
    print(f"Synced assets to Downloads: {dl_assets}")

    dl_html = os.path.join(DOWNLOADS_DIR, "Drift_Zero_User_Manual_v0.6.0.html")
    with open(dl_html, "w", encoding="utf-8") as f:
        f.write(full_html)
    print(f"Copied HTML to Downloads: {dl_html}")

def build_pdf():
    edge_paths = [
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"
    ]
    edge_exe = None
    for p in edge_paths:
        if os.path.exists(p):
            edge_exe = p
            break

    if not edge_exe:
        print("Note: Microsoft Edge not found in standard paths; skipping PDF generation.")
        return

    dl_html = os.path.join(DOWNLOADS_DIR, "Drift_Zero_User_Manual_v0.6.0.html")
    dl_pdf = os.path.join(DOWNLOADS_DIR, "Drift_Zero_User_Manual_v0.6.0.pdf")

    cmd = [
        edge_exe,
        "--headless",
        "--disable-gpu",
        "--no-pdf-header-footer",
        f"--print-to-pdf={dl_pdf}",
        f"file:///{dl_html.replace(os.sep, '/')}"
    ]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"Generated PDF: {dl_pdf}")
    except Exception as e:
        print(f"Failed to compile PDF: {e}")

if __name__ == "__main__":
    md_content = build_markdown()
    build_html(md_content)
    build_pdf()
    print("Build completed successfully.")
