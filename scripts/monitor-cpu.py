#!/usr/bin/env python3
"""Monitor CPU usage of the JQGunk VST plugin process via /proc.

Finds the Reaper VST bridge process hosting JQGunk (or the standalone),
polls /proc/[pid]/stat every 200ms, and writes timestamped CSV to cpu-log.csv.

Usage:
    python scripts/monitor-cpu.py              # auto-detect process
    python scripts/monitor-cpu.py --pid 12345  # monitor specific PID
    python scripts/monitor-cpu.py -o out.csv   # custom output file
"""

import argparse
import os
import signal
import subprocess
import sys
import time

POLL_INTERVAL = 0.2  # seconds
CSV_HEADER = "timestamp_ms,cpu_pct,rss_kb\n"


def find_jqgunk_pid():
    """Find the PID of the process hosting JQGunk.

    Searches for:
    1. reaper_host_x86_64 processes with JQGunk in cmdline (VST bridge)
    2. JQGunk standalone process
    """
    for pid_dir in os.listdir("/proc"):
        if not pid_dir.isdigit():
            continue
        try:
            cmdline_path = f"/proc/{pid_dir}/cmdline"
            with open(cmdline_path, "rb") as f:
                cmdline = f.read().decode("utf-8", errors="replace")
            if "JQGunk" in cmdline or "jqgunk" in cmdline.lower():
                return int(pid_dir)
        except (PermissionError, FileNotFoundError, ProcessLookupError):
            continue

    # Fallback: try pgrep
    for pattern in ["JQGunk", "reaper_host.*JQGunk"]:
        try:
            result = subprocess.run(
                ["pgrep", "-f", pattern],
                capture_output=True, text=True, timeout=5
            )
            if result.returncode == 0:
                pids = result.stdout.strip().split("\n")
                if pids and pids[0]:
                    return int(pids[0])
        except (subprocess.TimeoutExpired, FileNotFoundError):
            continue

    return None


def read_proc_stat(pid):
    """Read CPU ticks and RSS from /proc/[pid]/stat and /proc/[pid]/statm."""
    stat_path = f"/proc/{pid}/stat"
    statm_path = f"/proc/{pid}/statm"

    with open(stat_path) as f:
        fields = f.read().split()

    # Fields 13 (utime) and 14 (stime) are user and kernel CPU ticks
    utime = int(fields[13])
    stime = int(fields[14])

    rss_pages = 0
    try:
        with open(statm_path) as f:
            statm_fields = f.read().split()
            rss_pages = int(statm_fields[1])  # resident set size in pages
    except (FileNotFoundError, IndexError):
        pass

    page_size_kb = os.sysconf("SC_PAGE_SIZE") // 1024
    rss_kb = rss_pages * page_size_kb

    return utime + stime, rss_kb


def monitor(pid, output_file):
    """Poll CPU and RSS, write CSV."""
    clk_tck = os.sysconf("SC_CLK_TCK")

    print(f"Monitoring PID {pid} (poll every {POLL_INTERVAL}s)")
    print(f"Writing to {output_file}")
    print("Press Ctrl+C to stop.\n")

    with open(output_file, "w") as csv:
        csv.write(CSV_HEADER)
        csv.flush()

        prev_ticks, _ = read_proc_stat(pid)
        prev_time = time.monotonic()

        while True:
            time.sleep(POLL_INTERVAL)
            try:
                curr_ticks, rss_kb = read_proc_stat(pid)
            except (FileNotFoundError, ProcessLookupError):
                print(f"\nProcess {pid} exited.")
                break

            curr_time = time.monotonic()
            dt = curr_time - prev_time
            dticks = curr_ticks - prev_ticks

            cpu_pct = (dticks / clk_tck) / dt * 100.0 if dt > 0 else 0.0

            ts_ms = int(time.time() * 1000)
            line = f"{ts_ms},{cpu_pct:.1f},{rss_kb}\n"
            csv.write(line)
            csv.flush()

            print(f"  t={ts_ms}  cpu={cpu_pct:5.1f}%  rss={rss_kb}KB", end="\r")

            prev_ticks = curr_ticks
            prev_time = curr_time

    print("\nDone.")


def main():
    parser = argparse.ArgumentParser(description="Monitor JQGunk process CPU usage")
    parser.add_argument("--pid", type=int, help="PID to monitor (auto-detect if omitted)")
    parser.add_argument("-o", "--output", default="cpu-log.csv", help="Output CSV file")
    args = parser.parse_args()

    pid = args.pid
    if pid is None:
        print("Searching for JQGunk process...")
        pid = find_jqgunk_pid()
        if pid is None:
            print("Could not find JQGunk process. Use --pid to specify manually.",
                  file=sys.stderr)
            sys.exit(1)
        print(f"Found JQGunk at PID {pid}")

    # Verify PID exists
    if not os.path.exists(f"/proc/{pid}"):
        print(f"PID {pid} does not exist.", file=sys.stderr)
        sys.exit(1)

    # Graceful shutdown on Ctrl+C
    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))

    try:
        monitor(pid, args.output)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
