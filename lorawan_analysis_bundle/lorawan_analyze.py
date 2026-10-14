#!/usr/bin/env python3
"""
LoRaWAN CSV analyzer for GhostBLE.

Usage:
    python3 lorawan_analyze.py lorawan_packets.csv
    python3 lorawan_analyze.py lorawan_packets.csv --out-dir report

Uses only the Python standard library. Creates:
    report.md                  Human-readable analysis
    report.html                HTML report with simple bar charts
    device_summary.csv         Per-DevAddr history and signal summary
    scan_slot_summary.csv      Counts grouped by frequency / SF / bandwidth
    packet_rows_enriched.csv   Original rows plus calculated FCnt delta

Important:
- Counts per frequency/SF are packet counts, NOT detection rates. To calculate
  detection rates, the CSV must also record dwell/scan time per radio slot.
- DevAddr is not a permanent globally unique hardware identity.
- GPS coordinates in this schema are the receiver's position at reception,
  unless your firmware explicitly stores transmitter coordinates.
"""

from __future__ import annotations

import argparse
import csv
import html
import math
import statistics
import sys
from collections import Counter, defaultdict
from datetime import datetime
from pathlib import Path
from typing import Any


NUMERIC_FIELDS = {
    "ms", "fcnt", "fcnt_delta", "fport", "adr", "ack", "fopts_len",
    "payload_len", "freq_mhz", "sf", "bw_khz", "rssi_dbm", "snr_db",
    "len", "toa_us", "lat", "lon", "alt", "sats",
}
SUCCESS_TYPES = {"OK", "SUCCESS", "VALID", "DECODED"}
ERROR_TYPES = {"CRC", "CRC_ERROR", "CRC FAIL", "CRC_FAIL"}


def parse_value(value: str | None) -> Any:
    if value is None:
        return None
    value = value.strip()
    if value == "":
        return None
    try:
        if value.lower() in {"nan", "none", "null"}:
            return None
        number = float(value)
        if not math.isfinite(number):
            return None
        return number
    except (TypeError, ValueError):
        return value


def fmt_num(value: Any, digits: int = 1) -> str:
    if value is None or value == "":
        return "—"
    try:
        x = float(value)
        if not math.isfinite(x):
            return "—"
        return f"{x:.{digits}f}"
    except (TypeError, ValueError):
        return str(value)


def fmt_time(value: Any) -> str:
    if not value:
        return "—"
    return str(value)


def csv_write(path: Path, rows: list[dict[str, Any]], fieldnames: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            writer.writerow({k: ("" if row.get(k) is None else row.get(k)) for k in fieldnames})


def read_csv(path: Path) -> list[dict[str, Any]]:
    with path.open("r", newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames:
            raise ValueError("CSV has no header row.")
        rows = []
        for raw in reader:
            row: dict[str, Any] = {}
            for key, value in raw.items():
                if key in NUMERIC_FIELDS:
                    row[key] = parse_value(value)
                else:
                    row[key] = value.strip() if isinstance(value, str) and value.strip() else None
            rows.append(row)
        return rows


def numeric_values(rows: list[dict[str, Any]], key: str) -> list[float]:
    out = []
    for row in rows:
        v = row.get(key)
        if isinstance(v, (int, float)) and math.isfinite(float(v)):
            out.append(float(v))
    return out


def mean_or_none(values: list[float]) -> float | None:
    return statistics.mean(values) if values else None


def unique_join(values: list[Any]) -> str:
    cleaned = sorted({str(v) for v in values if v not in (None, "")})
    return ";".join(cleaned)


def classify_type(row: dict[str, Any]) -> str:
    t = str(row.get("type") or "").strip().upper()
    if t in ERROR_TYPES or "CRC" in t:
        return "error"
    if t in SUCCESS_TYPES:
        return "success"
    # If the logger uses a different status label, do not silently assume OK.
    return "other"


def sort_time_key(row: dict[str, Any]) -> tuple[int, str]:
    value = str(row.get("gps_time") or "")
    try:
        return (0, datetime.fromisoformat(value).isoformat())
    except ValueError:
        return (1, value)


def enrich_fcnt(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Calculate FCnt difference between consecutive successful rows per DevAddr."""
    last_counter: dict[str, float] = {}
    enriched = []
    for row in sorted(rows, key=sort_time_key):
        copy = dict(row)
        devaddr = str(copy.get("devaddr") or "").strip().upper()
        fcnt = copy.get("fcnt")
        if classify_type(copy) == "success" and devaddr and isinstance(fcnt, (int, float)):
            previous = last_counter.get(devaddr)
            if previous is not None:
                copy["fcnt_delta_calculated"] = fcnt - previous
            else:
                copy["fcnt_delta_calculated"] = None
            last_counter[devaddr] = fcnt
        else:
            copy["fcnt_delta_calculated"] = None
        enriched.append(copy)
    return enriched


def build_device_summary(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        devaddr = str(row.get("devaddr") or "").strip().upper()
        if devaddr and classify_type(row) == "success":
            groups[devaddr].append(row)

    output = []
    for devaddr, items in groups.items():
        items = sorted(items, key=sort_time_key)
        rssis = numeric_values(items, "rssi_dbm")
        snrs = numeric_values(items, "snr_db")
        fc = numeric_values(items, "fcnt")
        deltas = []
        last = None
        for item in items:
            val = item.get("fcnt")
            if isinstance(val, (int, float)):
                if last is not None:
                    deltas.append(val - last)
                last = val
        output.append({
            "devaddr": devaddr,
            "packets": len(items),
            "first_seen": items[0].get("gps_time"),
            "last_seen": items[-1].get("gps_time"),
            "best_rssi_dbm": max(rssis) if rssis else None,
            "worst_rssi_dbm": min(rssis) if rssis else None,
            "avg_rssi_dbm": mean_or_none(rssis),
            "best_snr_db": max(snrs) if snrs else None,
            "avg_snr_db": mean_or_none(snrs),
            "first_fcnt": fc[0] if fc else None,
            "last_fcnt": fc[-1] if fc else None,
            "last_fcnt_delta": deltas[-1] if deltas else None,
            "frequencies_mhz": unique_join([fmt_num(x, 3) for x in numeric_values(items, "freq_mhz")]),
            "spreading_factors": unique_join([int(x) if x.is_integer() else x for x in numeric_values(items, "sf")]),
            "fports": unique_join([int(x) if x.is_integer() else x for x in numeric_values(items, "fport")]),
            "last_lat": items[-1].get("lat"),
            "last_lon": items[-1].get("lon"),
        })
    return sorted(output, key=lambda r: (-int(r["packets"]), str(r["devaddr"])))


def build_slot_summary(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[Any, Any, Any], list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        key = (row.get("freq_mhz"), row.get("sf"), row.get("bw_khz"))
        groups[key].append(row)

    output = []
    for (freq, sf, bw), items in groups.items():
        ok = sum(classify_type(r) == "success" for r in items)
        crc = sum(classify_type(r) == "error" for r in items)
        other = len(items) - ok - crc
        rssis = numeric_values([r for r in items if classify_type(r) == "success"], "rssi_dbm")
        snrs = numeric_values([r for r in items if classify_type(r) == "success"], "snr_db")
        output.append({
            "freq_mhz": freq,
            "sf": sf,
            "bw_khz": bw,
            "total_packets": len(items),
            "decoded_ok": ok,
            "crc_errors": crc,
            "other_status": other,
            "avg_rssi_ok_dbm": mean_or_none(rssis),
            "best_rssi_ok_dbm": max(rssis) if rssis else None,
            "avg_snr_ok_db": mean_or_none(snrs),
            "first_seen": min((str(r.get("gps_time") or "") for r in items), default=""),
            "last_seen": max((str(r.get("gps_time") or "") for r in items), default=""),
        })
    return sorted(output, key=lambda r: (
        float(r["freq_mhz"]) if isinstance(r["freq_mhz"], (int, float)) else 0,
        float(r["sf"]) if isinstance(r["sf"], (int, float)) else 0,
        float(r["bw_khz"]) if isinstance(r["bw_khz"], (int, float)) else 0,
    ))


def bar_chart(title: str, labels: list[str], values: list[int], color: str = "#3b82f6") -> str:
    max_value = max(values, default=0) or 1
    pieces = [f'<section class="chart"><h3>{html.escape(title)}</h3>']
    for label, value in zip(labels, values):
        width = max(1, round(100 * value / max_value)) if value else 0
        pieces.append(
            '<div class="barrow"><div class="barlabel">' + html.escape(label) +
            '</div><div class="bartrack"><div class="bar" style="width:' + str(width) +
            '%;background:' + color + '"></div></div><div class="barvalue">' +
            str(value) + '</div></div>'
        )
    pieces.append("</section>")
    return "".join(pieces)


def write_reports(input_path: Path, out_dir: Path, rows: list[dict[str, Any]]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    enriched = enrich_fcnt(rows)
    devices = build_device_summary(rows)
    slots = build_slot_summary(rows)

    csv_write(out_dir / "device_summary.csv", devices, [
        "devaddr", "packets", "first_seen", "last_seen", "best_rssi_dbm",
        "worst_rssi_dbm", "avg_rssi_dbm", "best_snr_db", "avg_snr_db",
        "first_fcnt", "last_fcnt", "last_fcnt_delta", "frequencies_mhz",
        "spreading_factors", "fports", "last_lat", "last_lon",
    ])
    csv_write(out_dir / "scan_slot_summary.csv", slots, [
        "freq_mhz", "sf", "bw_khz", "total_packets", "decoded_ok",
        "crc_errors", "other_status", "avg_rssi_ok_dbm", "best_rssi_ok_dbm",
        "avg_snr_ok_db", "first_seen", "last_seen",
    ])
    all_keys = list(dict.fromkeys([k for row in rows for k in row.keys()] + ["fcnt_delta_calculated"]))
    csv_write(out_dir / "packet_rows_enriched.csv", enriched, all_keys)

    statuses = Counter(classify_type(r) for r in rows)
    successful = [r for r in rows if classify_type(r) == "success"]
    errors = [r for r in rows if classify_type(r) == "error"]
    other = [r for r in rows if classify_type(r) == "other"]
    unique_addrs = {str(r.get("devaddr")).upper() for r in successful if r.get("devaddr")}
    times = sorted([str(r.get("gps_time")) for r in rows if r.get("gps_time")])
    first_time = times[0] if times else "—"
    last_time = times[-1] if times else "—"
    try:
        duration = datetime.fromisoformat(last_time) - datetime.fromisoformat(first_time)
        duration_text = str(duration)
    except (ValueError, TypeError):
        duration_text = "Not available"

    freq_counts = Counter(fmt_num(r.get("freq_mhz"), 3) for r in rows if r.get("freq_mhz") is not None)
    sf_counts = Counter(str(int(r["sf"]) if isinstance(r.get("sf"), (int, float)) and float(r["sf"]).is_integer() else r.get("sf"))
                        for r in rows if r.get("sf") is not None)
    rssi = numeric_values(successful, "rssi_dbm")
    snr = numeric_values(successful, "snr_db")
    total = len(rows)
    error_pct = (100 * len(errors) / total) if total else 0
    top_devices = devices[:15]

    md = [
        f"# LoRaWAN CSV analysis: {input_path.name}", "",
        "## Overview", "",
        f"- Total CSV rows: **{total}**",
        f"- Successfully decoded (`type`=OK/success): **{len(successful)}**",
        f"- CRC/error rows: **{len(errors)}** ({error_pct:.1f}%)",
        f"- Other/unclassified status rows: **{len(other)}**",
        f"- Distinct DevAddr among successful rows: **{len(unique_addrs)}**",
        f"- Time range: **{first_time}** to **{last_time}**",
        f"- Duration between first and last timestamp: **{duration_text}**", "",
        "## Reception summary", "",
        f"- Best RSSI among successful packets: **{max(rssi):.1f} dBm**" if rssi else "- Best RSSI: not available",
        f"- Weakest RSSI among successful packets: **{min(rssi):.1f} dBm**" if rssi else "- Weakest RSSI: not available",
        f"- Average RSSI among successful packets: **{statistics.mean(rssi):.1f} dBm**" if rssi else "- Average RSSI: not available",
        f"- Best SNR among successful packets: **{max(snr):.1f} dB**" if snr else "- Best SNR: not available",
        f"- Weakest SNR among successful packets: **{min(snr):.1f} dB**" if snr else "- Weakest SNR: not available",
        "", "## Packets by frequency", "",
        "| Frequency | Packets |", "|---:|---:|",
    ]
    md.extend(f"| {freq} MHz | {count} |" for freq, count in sorted(freq_counts.items(), key=lambda x: float(x[0])))
    md += ["", "## Packets by spreading factor", "", "| SF | Packets |", "|---:|---:|"]
    md.extend(f"| {sf} | {count} |" for sf, count in sorted(sf_counts.items(), key=lambda x: int(x[0]) if x[0].isdigit() else 999))
    md += ["", "## Most frequently seen DevAddr", "",
           "| DevAddr | Packets | First seen | Last seen | Best RSSI | Avg RSSI | Avg SNR | Last FCnt | Last FCnt delta | Frequencies | SFs |",
           "|---|---:|---|---|---:|---:|---:|---:|---:|---|---|"]
    for d in top_devices:
        md.append(
            f"| {d['devaddr']} | {d['packets']} | {d['first_seen'] or '—'} | {d['last_seen'] or '—'} | "
            f"{fmt_num(d['best_rssi_dbm'])} | {fmt_num(d['avg_rssi_dbm'])} | {fmt_num(d['avg_snr_db'])} | "
            f"{fmt_num(d['last_fcnt'], 0)} | {fmt_num(d['last_fcnt_delta'], 0)} | "
            f"{d['frequencies_mhz']} | {d['spreading_factors']} |"
        )
    md += ["", "## Frequency / SF / bandwidth slot counts", "",
           "| Frequency MHz | SF | BW kHz | Total | Decoded OK | CRC errors | Avg RSSI OK | Avg SNR OK |",
           "|---:|---:|---:|---:|---:|---:|---:|---:|"]
    for s in slots:
        md.append(
            f"| {fmt_num(s['freq_mhz'], 3)} | {fmt_num(s['sf'], 0)} | {fmt_num(s['bw_khz'], 0)} | "
            f"{s['total_packets']} | {s['decoded_ok']} | {s['crc_errors']} | "
            f"{fmt_num(s['avg_rssi_ok_dbm'])} | {fmt_num(s['avg_snr_ok_db'])} |"
        )
    md += [
        "", "## Interpretation notes", "",
        "- Packet counts by frequency/SF are **not detection rates** unless dwell time or scan-attempt counts are also logged.",
        "- A DevAddr is not a permanent globally unique hardware identifier; treat it as a network/session address.",
        "- `fcnt_delta_calculated` is computed from consecutive successful rows with the same DevAddr, sorted by `gps_time`. It can be misleading across session resets, counter resets, missed packets, or out-of-order timestamps.",
        "- GPS fields are treated as receiver position, unless the firmware explicitly records transmitter location.",
        "- `type` values not recognized as OK/success or CRC/error are counted as unclassified instead of being silently treated as valid.",
        "", "## Generated files", "",
        "- `report.html`: visual report",
        "- `device_summary.csv`: per-address history and signal summary",
        "- `scan_slot_summary.csv`: counts by frequency/SF/bandwidth",
        "- `packet_rows_enriched.csv`: input rows plus calculated FCnt delta",
        "",
    ]
    (out_dir / "report.md").write_text("\n".join(md), encoding="utf-8")

    freq_labels = [x for x, _ in sorted(freq_counts.items(), key=lambda x: float(x[0]))]
    freq_values = [freq_counts[x] for x in freq_labels]
    sf_labels = [x for x, _ in sorted(sf_counts.items(), key=lambda x: int(x[0]) if x[0].isdigit() else 999)]
    sf_values = [sf_counts[x] for x in sf_labels]
    status_labels = ["Decoded OK", "CRC/error", "Other"]
    status_values = [len(successful), len(errors), len(other)]

    device_table = "".join(
        "<tr>" + "".join(f"<td>{html.escape(str(v if v is not None else '—'))}</td>" for v in [
            d["devaddr"], d["packets"], d["first_seen"], d["last_seen"],
            fmt_num(d["best_rssi_dbm"]), fmt_num(d["avg_rssi_dbm"]),
            fmt_num(d["avg_snr_db"]), fmt_num(d["last_fcnt"], 0),
            fmt_num(d["last_fcnt_delta"], 0),
        ]) + "</tr>" for d in top_devices
    )
    slot_table = "".join(
        "<tr>" + "".join(f"<td>{html.escape(str(v if v is not None else '—'))}</td>" for v in [
            fmt_num(s["freq_mhz"], 3), fmt_num(s["sf"], 0), fmt_num(s["bw_khz"], 0),
            s["total_packets"], s["decoded_ok"], s["crc_errors"],
            fmt_num(s["avg_rssi_ok_dbm"]), fmt_num(s["avg_snr_ok_db"]),
        ]) + "</tr>" for s in slots
    )
    html_report = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LoRaWAN CSV Analysis</title>
<style>
body{{font:16px/1.5 system-ui,-apple-system,Segoe UI,sans-serif;max-width:1200px;margin:2rem auto;padding:0 1rem;color:#202124;background:#fff}}
h1,h2{{line-height:1.2}} .cards{{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px}}
.card{{border:1px solid #ddd;border-radius:12px;padding:14px}} .big{{font-size:1.8rem;font-weight:700}}
.chart{{border:1px solid #ddd;border-radius:12px;padding:14px;margin:12px 0;min-width:0}}
.barrow{{display:grid;grid-template-columns:110px minmax(80px,1fr) 35px;gap:8px;align-items:center;margin:8px 0}}
.bartrack{{height:14px;background:#eee;border-radius:4px;overflow:hidden}} .bar{{height:100%;border-radius:4px}}
.barlabel{{font-size:.9rem}} .barvalue{{text-align:right;font-variant-numeric:tabular-nums}}
.tablewrap{{overflow-x:auto}} table{{border-collapse:collapse;width:100%;font-size:.9rem}} th,td{{border-bottom:1px solid #ddd;text-align:left;padding:8px;white-space:nowrap}} th{{background:#f5f5f5}}
.note{{background:#f7f7f7;border-radius:8px;padding:12px}} footer{{color:#666;font-size:.85rem;margin-top:2rem}}
</style></head><body>
<h1>LoRaWAN CSV Analysis</h1><p>Source: <code>{html.escape(input_path.name)}</code></p>
<div class="cards">
<div class="card"><div>Total rows</div><div class="big">{total}</div></div>
<div class="card"><div>Decoded OK</div><div class="big">{len(successful)}</div></div>
<div class="card"><div>CRC/error</div><div class="big">{len(errors)} ({error_pct:.1f}%)</div></div>
<div class="card"><div>Distinct DevAddr</div><div class="big">{len(unique_addrs)}</div></div>
<div class="card"><div>Duration</div><div class="big" style="font-size:1.2rem">{html.escape(duration_text)}</div></div>
</div>
<p><b>Time range:</b> {html.escape(first_time)} — {html.escape(last_time)}</p>
{bar_chart("Packets by frequency", freq_labels, freq_values)}
{bar_chart("Packets by spreading factor", sf_labels, sf_values, "#0f9d58")}
{bar_chart("Status counts", status_labels, status_values, "#d97706")}
<h2>Reception quality (decoded packets only)</h2>
<ul><li>Best RSSI: <b>{fmt_num(max(rssi) if rssi else None)} dBm</b></li>
<li>Weakest RSSI: <b>{fmt_num(min(rssi) if rssi else None)} dBm</b></li>
<li>Average RSSI: <b>{fmt_num(mean_or_none(rssi))} dBm</b></li>
<li>Best SNR: <b>{fmt_num(max(snr) if snr else None)} dB</b></li>
<li>Weakest SNR: <b>{fmt_num(min(snr) if snr else None)} dB</b></li></ul>
<h2>Most frequently seen DevAddr (top 15)</h2><div class="tablewrap"><table><thead><tr>
<th>DevAddr</th><th>Packets</th><th>First seen</th><th>Last seen</th><th>Best RSSI</th><th>Avg RSSI</th><th>Avg SNR</th><th>Last FCnt</th><th>Last FCnt delta</th>
</tr></thead><tbody>{device_table}</tbody></table></div>
<h2>Frequency / SF / bandwidth breakdown</h2><div class="tablewrap"><table><thead><tr>
<th>MHz</th><th>SF</th><th>BW kHz</th><th>Total</th><th>Decoded OK</th><th>CRC errors</th><th>Avg RSSI OK</th><th>Avg SNR OK</th>
</tr></thead><tbody>{slot_table}</tbody></table></div>
<h2>How to interpret this report</h2><div class="note"><ul>
<li>Packet counts are not detection rates unless scan dwell time or attempts per slot are logged.</li>
<li>DevAddr is a network/session address, not a permanent globally unique hardware ID.</li>
<li>FCnt deltas are calculated from adjacent successful rows for the same DevAddr. Counter resets, missing frames or session changes can affect interpretation.</li>
<li>GPS values are assumed to be receiver position, unless firmware records transmitter coordinates.</li>
</ul></div>
<footer>Generated by lorawan_analyze.py. No third-party Python packages required.</footer>
</body></html>"""
    (out_dir / "report.html").write_text(html_report, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze a GhostBLE LoRaWAN packet CSV.")
    parser.add_argument("csv_file", type=Path, help="Input lorawan_packets.csv file")
    parser.add_argument("--out-dir", type=Path, default=None, help="Output directory (default: <csvname>_report)")
    args = parser.parse_args()

    if not args.csv_file.is_file():
        print(f"Error: input file not found: {args.csv_file}", file=sys.stderr)
        return 2
    out_dir = args.out_dir or args.csv_file.with_name(args.csv_file.stem + "_report")
    try:
        rows = read_csv(args.csv_file)
        if not rows:
            print("Error: CSV contains a header but no data rows.", file=sys.stderr)
            return 2
        write_reports(args.csv_file, out_dir, rows)
    except (OSError, csv.Error, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 2

    print(f"Analyzed {len(rows)} rows from {args.csv_file}")
    print(f"Report folder: {out_dir}")
    print(f"  - {out_dir / 'report.html'}")
    print(f"  - {out_dir / 'report.md'}")
    print(f"  - {out_dir / 'device_summary.csv'}")
    print(f"  - {out_dir / 'scan_slot_summary.csv'}")
    print(f"  - {out_dir / 'packet_rows_enriched.csv'}")
    print("Open report.html in your browser.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
