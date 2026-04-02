"""
Two-panel window:
  LEFT:  Live camera feed with detection overlay (only when ENABLE_STREAM 1 in config.h)
  RIGHT: UART text logs, any binary data from the stream is filtered out.

Usage:
    python turret_monitor.py -p COM3
    python turret_monitor.py -p COM3 -br 921600

Press 'q' to quit.
"""

import threading
import time
import sys
import queue
import argparse

import numpy as np
import cv2
import serial

# same image stream constants as in the stm32 code
ROWS        = 144
COLS        = 174
SCALE       = 3
FRAME_BYTES = ROWS * COLS * 2 # YUV422: 2 bytes per pixel

# just the log panel layout
LOG_W         = 520 # the pixel width of the text panel
LOG_MAX_LINES = 200 # the lines kept in memory
FONT          = cv2.FONT_HERSHEY_SIMPLEX
FONT_SCALE    = 0.42
FONT_THICK    = 1
LINE_H        = 17 # the pixels between log lines
LOG_TOP       = 30 # the y-offset after title bar

# Shared state, that one is written by reader thread, but it isread by main thread
_frame_queue   = queue.Queue(maxsize=2)
_log_lines     = []
_log_lock      = threading.Lock()
_latest_det    = None
_stream_active = False


# reader thread
def _is_text(raw: bytes) -> bool:
    """Return True if the byte string looks like human-readable text (not binary)."""
    if not raw:
        return False
    printable = sum(1 for b in raw if 0x20 <= b <= 0x7E or b in (0x09, 0x0A, 0x0D))
    return printable / len(raw) > 0.90


def _add_log(msg: str):
    ts = time.strftime("%H:%M:%S")
    with _log_lock:
        _log_lines.append(f"[{ts}] {msg}")
        if len(_log_lines) > LOG_MAX_LINES:
            del _log_lines[:-LOG_MAX_LINES]


def _parse_det(line: str):
    """Parse '!DET:detected,row,col,px,rmin,rmax,cmin,cmax'"""
    try:
        parts = line[5:].split(',')
        if len(parts) >= 8:
            return {
                'detected':     int(parts[0]) != 0,
                'centroid_row': int(parts[1]),
                'centroid_col': int(parts[2]),
                'pixel_count':  int(parts[3]),
                'bbox_row_min': int(parts[4]),
                'bbox_row_max': int(parts[5]),
                'bbox_col_min': int(parts[6]),
                'bbox_col_max': int(parts[7]),
            }
    except (ValueError, IndexError):
        pass
    return None


def serial_reader(ser):
    """
    Background thread.
    - When it sees 'PREAMBLE!', it reads the raw frame bytes then the !DET line.
    - Everything else that looks like readable text is appended to the log.
    - Binary garbage is silently dropped.
    """
    global _latest_det, _stream_active

    while True:
        try:
            raw_line = ser.readline()
        except Exception:
            break

        if not raw_line:
            continue

        # frame preamble
        if raw_line.strip() == b"PREAMBLE!":
            _stream_active = True

            # Read exact binary frame
            frame_raw = b""
            while len(frame_raw) < FRAME_BYTES:
                try:
                    chunk = ser.read(FRAME_BYTES - len(frame_raw))
                except Exception:
                    break
                if chunk:
                    frame_raw += chunk

            if len(frame_raw) == FRAME_BYTES:
                arr = np.frombuffer(frame_raw, dtype=np.uint8).reshape((ROWS, COLS, 2))
                # Drain stale frame then enqueue latest
                while not _frame_queue.empty():
                    try:
                        _frame_queue.get_nowait()
                    except queue.Empty:
                        break
                _frame_queue.put(arr)

            # Read the !DET: result line that follows the binary data
            try:
                det_raw = ser.readline()
                det_str = det_raw.decode(errors='replace').strip()
                if det_str.startswith("!DET:"):
                    _latest_det = _parse_det(det_str)
            except Exception:
                pass

            continue  # do NOT log preamble or det lines

        # normal text line
        if not _is_text(raw_line):
            continue  # drop binary garbage

        try:
            text = raw_line.decode('utf-8', errors='replace').strip()
        except Exception:
            continue

        if not text:
            continue

        # drop stream protocol lines that might slip through
        if text.startswith("!DET:") or text == "PREAMBLE!":
            continue

        _add_log(text)


# Render helpers
def _make_camera_panel(frame_arr, det, fps):
    """Render the left camera panel with detection overlay."""
    display = cv2.cvtColor(frame_arr, cv2.COLOR_YUV2BGR_UYVY)

    if det and det['detected']:
        cr   = det['centroid_row']
        cc   = det['centroid_col']
        rmin = det['bbox_row_min']
        rmax = det['bbox_row_max']
        cmin = det['bbox_col_min']
        cmax = det['bbox_col_max']

        cv2.rectangle(display, (cmin, rmin), (cmax, rmax), (0, 255, 0), 1)
        cv2.line(display,   (cc - 7, cr),  (cc + 7, cr),  (255, 255, 0), 1)
        cv2.line(display,   (cc, cr - 7),  (cc, cr + 7),  (255, 255, 0), 1)
        cv2.circle(display, (cc, cr), 3,                   (255, 255, 0), -1)
        label       = f"TARGET  ({cr}, {cc})  px={det['pixel_count']}"
        label_color = (0, 255, 0)
    else:
        label       = "NO TARGET"
        label_color = (0, 60, 255)

    # Dim center crosshair (camera's aim reference)
    cx, cy = COLS // 2, ROWS // 2
    cv2.line(display, (cx - 10, cy), (cx + 10, cy), (70, 70, 70), 1)
    cv2.line(display, (cx, cy - 10), (cx, cy + 10), (70, 70, 70), 1)

    display = cv2.resize(display, (COLS * SCALE, ROWS * SCALE), interpolation=cv2.INTER_NEAREST)
    H, W = display.shape[:2]

    cv2.putText(display, label,          (10, 24),      FONT, 0.55, label_color,     2)
    cv2.putText(display, f"{fps:.1f} FPS", (W - 110, 24), FONT, 0.50, (200, 200, 200), 1)

    return display


def _make_no_stream_panel():
    """Left panel shown when streaming is disabled on the STM32 side."""
    H = ROWS * SCALE
    W = COLS * SCALE
    panel = np.zeros((H, W, 3), dtype=np.uint8)
    lines = [
        "Stream disabled",
        "",
        "Set  ENABLE_STREAM 1",
        "in config.h to enable",
        "the live camera feed.",
    ]
    y = H // 2 - (len(lines) * LINE_H) // 2
    for line in lines:
        cv2.putText(panel, line, (20, y), FONT, 0.55, (80, 80, 80), 1)
        y += LINE_H + 4
    return panel


def _make_log_panel(panel_h):
    """Render the right UART log panel."""
    panel = np.zeros((panel_h, LOG_W, 3), dtype=np.uint8)

    # Title bar
    cv2.rectangle(panel, (0, 0), (LOG_W, LOG_TOP - 2), (25, 25, 40), -1)
    cv2.putText(panel, "UART Log", (8, LOG_TOP - 8), FONT, 0.50, (160, 160, 255), 1)
    cv2.line(panel, (0, LOG_TOP - 2), (LOG_W, LOG_TOP - 2), (50, 50, 80), 1)

    # How many lines fit
    max_visible = (panel_h - LOG_TOP) // LINE_H

    with _log_lock:
        visible = _log_lines[-max_visible:]

    y = LOG_TOP + LINE_H - 4
    for line in visible:
        # Color-code by content
        if any(k in line for k in ("ERROR", "FAIL", "failed")):
            color = (60,  60,  255)
        elif any(k in line for k in ("FIRING", "LOCKED")):
            color = (60,  255, 120)
        elif any(k in line for k in ("AIMING", "SEARCHING")):
            color = (50,  200, 255)
        elif "->" in line:
            color = (200, 220, 255)
        elif "press B1" in line:
            color = (180, 180, 60)
        else:
            color = (180, 200, 180)

        cv2.putText(panel, line[:76], (8, y), FONT, FONT_SCALE, color, FONT_THICK)
        y += LINE_H

    return panel

def main():
    parser = argparse.ArgumentParser(description="Turret monitor — camera feed + UART log")
    parser.add_argument('-p',  '--port',     required=True,            help='Serial COM port (e.g. COM3)')
    parser.add_argument('-br', '--baudrate', type=int, default=921600, help='Baudrate (default: 921600)')
    parser.add_argument('--timeout',         type=float, default=1.0,  help='Serial read timeout (s)')
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=args.timeout)
        _add_log(f"Connected: {args.port} @ {args.baudrate} baud")
    except serial.SerialException as e:
        print(f"ERROR: Could not open {args.port}: {e}")
        sys.exit(1)

    # Start background serial reader
    reader = threading.Thread(target=serial_reader, args=(ser,), daemon=True)
    reader.start()

    panel_h    = ROWS * SCALE
    last_frame = _make_no_stream_panel()
    fps        = 0.0
    prev_ts    = None

    print("Monitor running — press 'q' in the window to quit.")

    while True:
        # Pull latest camera frame if one arrived
        try:
            frame_arr  = _frame_queue.get_nowait()
            now        = time.time()
            if prev_ts and (now - prev_ts) > 0:
                fps = 0.8 * fps + 0.2 / (now - prev_ts)
            prev_ts    = now
            last_frame = _make_camera_panel(frame_arr, _latest_det, fps)
        except queue.Empty:
            if not _stream_active:
                last_frame = _make_no_stream_panel()

        log_panel = _make_log_panel(panel_h)
        combined  = np.hstack([last_frame, log_panel])

        cv2.imshow("Turret Monitor", combined)

        key = cv2.waitKey(30) & 0xFF
        if key == ord('q') or key == 27:  # q or ESC
            break
        # Also quit if the window was closed with the X button
        if cv2.getWindowProperty("Turret Monitor", cv2.WND_PROP_VISIBLE) < 1:
            break

    ser.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()