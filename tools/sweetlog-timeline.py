#!/usr/bin/env python3
"""Boot timeline from a SANTONILOG1 ring with 0202 uptime stamps.

  python work/sweetlog-timeline.py artifacts/logs/sweetlog-XXX.bin [--gaps N] [--text OUT]

Prints when the boot milestones happened and the N largest silent gaps
(with the line before and after each gap), so a slow boot says where the
time went. Logs from kernels before 0202 have no stamps: then the
launchd/SpringBoard os_log times (RTC unset, 1969-12-31 16:00 PST = boot)
bracket each kernel line.
"""
import argparse
import re
import sys

STAMP = re.compile(r'^\[\s*(\d+)\.(\d{3})\] ?(.*)$')
MILESTONES = [
    ('kernel console', r'pmap_startup'),
    ('4 CPUs up', r'SWEET CPU: \d+/\d+ CPUs initialized'),
    ('log flusher', r'LOG: console log flusher active'),
    ('BSD root mounted', r'BSD root: |root device'),
    ('launchd', r'exec launchd|launchd\['),
    ('first datamigrator', r'exec com\.apple\.datamigrator'),
    ('SpringBoard exec', r'exec SpringBoard'),
    ('backboardd exec', r'exec backboardd'),
    ('display on', r'SWEET FB: display on'),
    ('first swap', r'SWEET FB: swap '),
    ('SpringBoard opens AKS', r'IOK SpringBoard: open AppleKeyStore'),
    ('lock screen surfaces', r'SWEET SURF: SpringBoard create'),
    ('display off (ignored)', r'display off ignored'),
    ('crash', r'^CRASH \S+\[\d+\]'),
]


def ring_text(path):
    d = open(path, 'rb').read()
    if d[:11] != b'SANTONILOG1':
        sys.exit(f'{path}: no SANTONILOG1 header')
    size = int.from_bytes(d[16:20], 'little')
    total = int.from_bytes(d[20:28], 'little')
    seq = int.from_bytes(d[28:36], 'little')
    ring = d[512:512 + size]
    if total <= size:
        body = ring[:total]
    else:  # wrapped: oldest byte sits at total % size
        at = total % size
        body = ring[at:] + ring[:at]
    return body.decode('latin1'), total, seq


USERCLOCK = re.compile(r'1969-12-31 (1[6-9]|2[0-3]):(\d\d):(\d\d)')


def userland_clock(lines):
    """Times from launchd/SpringBoard os_log lines on a boot with no RTC.

    The wall clock starts at the epoch, printed in the iOS default zone
    (PST), so 1969-12-31 16:00:00 is uptime 0. Each row keeps the text of
    the line, or of the next line for SpringBoard's two-line records.
    """
    rows = []
    for i, ln in enumerate(lines):
        m = USERCLOCK.search(ln)
        if not m:
            continue
        t = (int(m[1]) - 16) * 3600 + int(m[2]) * 60 + int(m[3])
        text = ln[m.end():].strip(' .-0123456789')
        if len(text) < 30 and i + 1 < len(lines):
            text = (text + ' ' + lines[i + 1]).strip()
        rows.append((float(t), f'{text}  (line {i + 1})', i))
    return rows


def bracket(lines, rows):
    """(earliest, latest) time for every line from the os_log times around it."""
    lo, hi = [None] * len(lines), [None] * len(lines)
    marks = {i: t for t, _, i in rows}
    cur = None
    for i in range(len(lines)):
        cur = marks.get(i, cur)
        lo[i] = cur
    cur = None
    for i in reversed(range(len(lines))):
        cur = marks.get(i, cur)
        hi[i] = cur
    return lo, hi


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log')
    ap.add_argument('--gaps', type=int, default=12)
    ap.add_argument('--text', help='also write the decoded text here')
    a = ap.parse_args()

    text, total, seq = ring_text(a.log)
    if a.text:
        open(a.text, 'w', encoding='utf-8').write(text)
    lines = text.splitlines()
    rows = []
    for ln in lines:
        m = STAMP.match(ln)
        if m:
            rows.append((int(m[1]) + int(m[2]) / 1000, m[3]))
    print(f'{a.log}: {total} bytes, flush #{seq}, {len(lines)} lines, '
          f'{len(rows)} stamped')
    span = None
    if not rows:
        rows = userland_clock(lines)
        if rows:
            span = bracket(lines, rows)
            rows = [(t, s) for t, s, _ in rows]
        if not rows:
            print('no uptime stamps (kernel predates patch 0202) and no '
                  'launchd/os_log times either')
            return
        print(f'no 0202 stamps: using the {len(rows)} userland os_log times '
              '(RTC unset, so 1969-12-31 16:00:00 PST = boot); kernel lines '
              'in between have no time')
    end = rows[-1][0]
    print(f'last stamp: {end:.3f} s ({end / 60:.1f} min)\n')

    print('milestones (first occurrence; crashes and display-off: all)')
    if span:
        fmt = lambda v: '?' if v is None else f'{v:.0f}'
        for name, pat in MILESTONES:
            rx = re.compile(pat)
            idx = [i for i, ln in enumerate(lines) if rx.search(ln)]
            every = name.startswith(('crash', 'display off'))
            if not idx:
                print(f'  {"-":>11}  {name}')
            for i in (idx if every else idx[:1]):
                when = f'{fmt(span[0][i])}-{fmt(span[1][i])}'
                print(f'  {when:>11}  {name}: line {i + 1}: {lines[i][:70]}')
        rows_for_milestones = []
    else:
        rows_for_milestones = MILESTONES
    for name, pat in rows_for_milestones:
        rx = re.compile(pat)
        hits = [(t, s) for t, s in rows if rx.search(s)]
        if not hits:
            print(f'  {"-":>9}  {name}')
            continue
        every = name.startswith(('crash', 'display off'))
        for t, s in (hits if every else hits[:1]):
            print(f'  {t:9.3f}  {name}: {s[:90]}')

    print(f'\nlargest gaps (top {a.gaps})')
    gaps = sorted(((rows[i + 1][0] - rows[i][0], i) for i in range(len(rows) - 1)),
                  reverse=True)[:a.gaps]
    for g, i in sorted(gaps, key=lambda x: x[1]):
        print(f'  {g:8.3f} s  at {rows[i][0]:9.3f}')
        print(f'      before: {rows[i][1][:110]}')
        print(f'      after:  {rows[i + 1][1][:110]}')

    if span:  # only os_log lines carry a time: a histogram would mislead
        return
    print('\nper-minute line counts')
    buckets = {}
    for t, _ in rows:
        buckets[int(t // 60)] = buckets.get(int(t // 60), 0) + 1
    for mnt in range(int(end // 60) + 1):
        n = buckets.get(mnt, 0)
        print(f'  {mnt:3d} min  {n:5d}  {"#" * min(60, n // 10)}')


if __name__ == '__main__':
    main()
