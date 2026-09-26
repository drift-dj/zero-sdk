# 5. Audio Import Workflows & Rekordbox XML Integration

Drift Zero supports three methods for populating its internal SQLite audio database: direct USB-C drive imports, workstation Mass Storage file transfers (Drive Mode), and Pioneer Rekordbox XML collection imports with cuepoint and beatgrid preservation.

## Workflow 1: Direct USB-C Flash Drive Import

1. Go to `Control Panels > USB Setup` and verify the controller role is set to `USB Host`.
2. Connect a USB-C flash drive or portable SSD to the rear USB-C port. The drive mounts to `/media/usb/`.
3. Press `NAV/SHIFT` to open the Library Menu, scroll to `+ Add Music`, and select `Browse Files`. The file system browser displays.
4. Turn `JOG` to highlight an audio file (`.wav`, `.aif`, `.mp3`, `.flac`) or directory, and press `JOG`. The selected files copy to internal storage.
5. Background analysis commences automatically (`ZDJ_LIBRARY_ANALYSIS_STATE_SCAN`). The background thread decodes audio transients via FFmpeg, calculates BPM, constructs beatgrid anchors, and logs Camelot musical keys to the SQLite database.

## Workflow 2: Workstation File Transfer (Drive Mode)

1. Connect Zero to a host computer using a high-speed USB-C data cable.
2. Go to `Library Menu > + Add Music > Drive Mode` (or `Control Panels > USB Setup > Drive Mode`). Zero enables its USB Mass Storage gadget driver (`zdj_usb_driver.c`).
3. Observe the host computer desktop. The internal storage partition mounts as an external mass storage drive.
4. Drag audio files and directories into the music partition.
5. Eject the volume from the host operating system to flush disk write caches.
6. Press `JOG` on Zero. Drive Mode exits, and the primary performance interface returns.
7. Go to `Library Menu > + Add Music > Scan for New Files`. Zero scans local storage for newly copied files and enqueues them for background audio analysis.

## Workflow 3: Rekordbox XML Collection Import

Rekordbox XML files contain metadata, cuepoint markers, and playlist hierarchies; they do not contain audio data. Both the XML file and the corresponding audio folder must reside on Zero's storage for successful ingestion.

```text
+-------------------------------------------------------------+
| REKORDBOX WORKSTATION                                       |
| 1. Export collection as XML (rekordbox.xml)                 |
| 2. Connect Zero in Drive Mode and copy XML + Audio root     |
+-------------------------------------------------------------+
                             |
                             v
+-------------------------------------------------------------+
| DRIFT ZERO HARDWARE CONSOLE                                 |
| 3. Eject drive on computer and exit Drive Mode on Zero      |
| 4. Go to Library Menu > + Add Music > Rekordbox XML         |
| 5. Dialog 1/3: Select rekordbox.xml file                    |
| 6. Dialog 2/3: Select audio root directory (/Contents)      |
| 7. Dialog 3/3: Set Error Policy (Skip Errors / Abort)       |
| 8. Execute database ingestion and playlist generation       |
+-------------------------------------------------------------+
```

### Verified Import Procedure

1. In Rekordbox on your workstation, go to `File > Export Collection in XML format` and save `rekordbox.xml`.
2. Connect Zero to your computer, select `Drive Mode`, and wait for the volume to mount.
3. Copy `rekordbox.xml` to the root of the Zero drive volume, and copy your Rekordbox audio directory (typically `/Contents`) onto the drive.
4. Eject the drive volume on your computer operating system.
5. Press `JOG` on Zero to terminate Drive Mode. The normal UI displays.
6. Go to `Library Menu > + Add Music > Rekordbox XML`. The XML selection dialog displays.

```text
+=============================================================+
| [IMPORT: REKORDBOX XML]               DIALOG 1/3: LOCATE XML|
| SELECT XML FILE:                                            |
|   ../                                                       |
| > rekordbox.xml                       (14.2 MB) [2,420 TRKS]|
|   backup_collection.xml                (9.8 MB)             |
+=============================================================+
```

7. Turn `JOG` to highlight `rekordbox.xml` and press `JOG`. The audio root selection dialog displays.

```text
+=============================================================+
| [IMPORT: REKORDBOX XML]               DIALOG 2/3: AUDIO ROOT|
|>[ SELECT THIS DIR: /media/internal/Contents ]               |
|   ../                                                       |
|   Artist_A/                                                 |
|   Artist_B/                                                 |
+=============================================================+
```

8. Turn `JOG` to highlight the folder where audio files were placed (e.g. `/media/internal/Contents`) and press `JOG` on `[ Select this dir ]`. The policy configuration dialog displays.

```text
+=============================================================+
| [IMPORT: REKORDBOX XML]               DIALOG 3/3: POLICIES  |
| ERROR POLICY:      >[ SKIP ERRORS ]     [ ABORT ON ERROR ]  |
| CUEPOINTS:          [ IMPORT ALL (8) ]                      |
| BEATGRIDS:          [ OVERWRITE EXISTING ]                  |
| > [ BEGIN INGESTION: 2,420 TRACKS ]                         |
+=============================================================+
```

9. Select the desired error policy:
   * **Skip Errors:** Continues importing remaining tracks if an unreadable audio file or missing path is encountered.
   * **Abort on Error:** Halts execution immediately upon encountering an unresolvable reference.
10. Press `JOG` on `[ BEGIN INGESTION ]`. Zero parses the XML tree using `libxml2`, constructs database entities (`zdj_library_create_rb_xml_import_song_graph`), registers TrackIDs, imports stored `<POSITIONMARK>` hotcues, and generates playlist trees.

## Failure Modes & Troubleshooting

| Condition | Symptom | Root Cause | Corrective Action |
|---|---|---|---|
| Tracks listed but will not play | Error displayed upon loading imported Rekordbox track | XML file was copied without copying the underlying audio files | Copy the audio directory to Zero's storage and repeat the XML import, selecting the audio folder in Dialog 2/3. |
| Zero not recognized over USB | Computer does not detect external drive volume | Zero is set to Host Mode instead of Gadget Mode, or cable is power-only | Go to `Control Panels > USB Setup` and switch to `Gadget Mode`. Use a verified USB-C high-speed data cable. |
| Filesystem mounts read-only | Workstation rejects file write operations | Volume was unplugged without unmounting, setting dirty flag | Run `Control Panels > Settings > System > File System Check` to clear the journal lock. |
