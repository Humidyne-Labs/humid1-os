#!/usr/bin/env python3
"""
play_audio_header.py - Inspect and play C-header audio arrays (.h) or raw PCM files.
Zero external pip dependencies (uses built-in winsound on Windows or ffplay).
"""

import argparse
import array
import io
import math
import re
import shutil
import subprocess
import sys
import wave
from pathlib import Path


def parse_header_file(header_path: Path):
    """
    Parses a C header file to extract sample rate, channels, array name, and PCM bytes.
    """
    text = header_path.read_text(encoding="utf-8", errors="ignore")

    # 1. Parse sample rate from comments (default 16000)
    rate_match = re.search(r"(\d{4,6})\s*Hz", text, re.IGNORECASE)
    sample_rate = int(rate_match.group(1)) if rate_match else 16000

    # 2. Parse channels (default 1 / Mono)
    channels = 2 if re.search(r"channels?:\s*2|stereo", text, re.IGNORECASE) else 1

    # 3. Parse variable name
    var_match = re.search(r"const\s+uint8_t\s+([a-zA-Z0-9_]+)\[\]", text)
    var_name = var_match.group(1) if var_match else header_path.stem

    # 4. Extract hex bytes inside the array brackets { ... }
    body_match = re.search(r"\{([^}]+)\}", text, re.DOTALL)
    if not body_match:
        raise ValueError(f"Could not find array curly braces '{{ ... }}' in {header_path.name}")

    hex_tokens = re.findall(r"0x([0-9a-fA-F]{2})", body_match.group(1))
    if not hex_tokens:
        raise ValueError(f"No hex bytes (0x..) found in array inside {header_path.name}")

    pcm_bytes = bytes.fromhex("".join(hex_tokens))
    return pcm_bytes, sample_rate, channels, var_name


def analyze_pcm(pcm_bytes: bytes, sample_rate: int, channels: int):
    """
    Calculates audio engineering metrics: Peak dBFS, RMS dBFS, DC offset, and clipping.
    """
    # 16-bit signed PCM = 2 bytes per sample
    samples = array.array("h")
    samples.frombytes(pcm_bytes)
    if sys.byteorder == "big":
        samples.byteswap()

    total_samples = len(samples)
    duration_sec = total_samples / (sample_rate * channels)

    if total_samples == 0:
        return {"error": "Empty audio data"}

    peak_pos = max(samples)
    peak_neg = min(samples)
    peak_abs = max(abs(peak_pos), abs(peak_neg))

    # Peak dBFS (relative to 32768 full scale)
    peak_dbfs = 20.0 * math.log10(peak_abs / 32768.0) if peak_abs > 0 else -float("inf")

    # RMS & RMS dBFS
    sum_squares = sum(s * s for s in samples)
    rms = math.sqrt(sum_squares / total_samples)
    rms_dbfs = 20.0 * math.log10(rms / 32768.0) if rms > 0 else -float("inf")

    # DC Offset (average amplitude drift from 0)
    dc_offset = sum(samples) / total_samples
    dc_offset_pct = (dc_offset / 32768.0) * 100.0

    # Clipping detection (samples hitting maximum 16-bit boundaries)
    clipped_count = sum(1 for s in samples if s >= 32767 or s <= -32768)
    clip_pct = (clipped_count / total_samples) * 100.0

    return {
        "bytes": len(pcm_bytes),
        "samples": total_samples,
        "duration": duration_sec,
        "sample_rate": sample_rate,
        "channels": "Mono (1)" if channels == 1 else "Stereo (2)",
        "peak_sample": peak_abs,
        "peak_dbfs": peak_dbfs,
        "rms_dbfs": rms_dbfs,
        "dc_offset": dc_offset,
        "dc_offset_pct": dc_offset_pct,
        "clipped_samples": clipped_count,
        "clip_pct": clip_pct,
    }


def play_audio(pcm_bytes: bytes, sample_rate: int, channels: int):
    """Plays raw PCM by building an in-memory WAV buffer."""
    wav_io = io.BytesIO()
    with wave.open(wav_io, "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)  # 16-bit
        wav.setframerate(sample_rate)
        wav.writeframes(pcm_bytes)
    wav_data = wav_io.getvalue()

    # Strategy 1: Built-in Windows Sound (zero install)
    if sys.platform == "win32":
        try:
            import winsound

            winsound.PlaySound(wav_data, winsound.SND_MEMORY)
            return
        except Exception as e:
            print(f"[WARN] winsound playback failed: {e}", file=sys.stderr)

    # Strategy 2: ffplay fallback
    if shutil.which("ffplay"):
        proc = subprocess.Popen(
            ["ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", "-i", "pipe:0"],
            stdin=subprocess.PIPE,
        )
        proc.communicate(input=wav_data)
        return

    print(
        "[WARN] Playback skipped: No playback device or 'ffplay' found.",
        file=sys.stderr,
    )


def print_report(target_name: str, var_name: str, diag: dict):
    print("\n" + "=" * 52)
    print(f" AUDIO DIAGNOSTICS: {target_name}")
    print("=" * 52)
    if var_name:
        print(f" C Variable Name : {var_name}")
    print(f" Format          : 16-bit Signed Little-Endian (s16le)")
    print(f" Sample Rate     : {diag['sample_rate']} Hz")
    print(f" Channels        : {diag['channels']}")
    print(f" Duration        : {diag['duration']:.3f} seconds")
    print(f" Total Payload   : {diag['bytes']:,} bytes ({diag['samples']:,} samples)")
    print("-" * 52)
    print(f" Peak Level      : {diag['peak_dbfs']:+.2f} dBFS  (sample value: {diag['peak_sample']} / 32768)")
    print(f" RMS Level       : {diag['rms_dbfs']:+.2f} dBFS")
    print(f" DC Offset       : {diag['dc_offset']:+.1f} ({diag['dc_offset_pct']:+.2f}%)")

    if diag["clipped_samples"] > 0:
        print(f" Clipping        : ALERT! {diag['clipped_samples']} clipped samples ({diag['clip_pct']:.2f}%)")
    else:
        headroom = -diag["peak_dbfs"]
        print(f" Clipping        : Clean (0 clipped samples, ~{headroom:.1f} dB headroom)")
    print("=" * 52 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="Inspect and play raw PCM arrays from C headers or binary files."
    )
    parser.add_argument("file", type=Path, help="Path to .h header file or .pcm binary")
    parser.add_argument("-r", "--rate", type=int, default=None, help="Override sample rate (Hz)")
    parser.add_argument("-c", "--channels", type=int, default=None, choices=[1, 2], help="Override channels")
    parser.add_argument("--no-play", action="store_true", help="Print diagnostics only without playing sound")

    args = parser.parse_args()

    if not args.file.exists():
        print(f"Error: File '{args.file}' not found.", file=sys.stderr)
        sys.exit(1)

    # Load data based on file extension
    ext = args.file.suffix.lower()
    if ext in [".h", ".hpp", ".c"]:
        try:
            pcm_bytes, detected_rate, detected_channels, var_name = parse_header_file(args.file)
        except Exception as e:
            print(f"Error parsing header: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        # Raw .pcm binary file
        pcm_bytes = args.file.read_bytes()
        detected_rate = 16000
        detected_channels = 1
        var_name = None

    sample_rate = args.rate or detected_rate
    channels = args.channels or detected_channels

    diag = analyze_pcm(pcm_bytes, sample_rate, channels)
    print_report(args.file.name, var_name, diag)

    if not args.no_play:
        print("Playing audio...")
        play_audio(pcm_bytes, sample_rate, channels)
        print("Done.")


if __name__ == "__main__":
    main()