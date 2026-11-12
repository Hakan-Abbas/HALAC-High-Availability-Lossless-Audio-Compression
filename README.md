## HALAC 1.0.0: key features

**General**
Lossless audio compressor: the decoded file is byte-for-byte identical to the original WAV. The WAV header, metadata chunks and trailing data are kept as they are.
Command-line tool: halac encode | decode | test | info, file extension .halac.
Encoder and decoder in a single Windows x64 executable (~760 KB), no installation needed.

**Supported input**
WAV, RF64/BW64 and WAVE_FORMAT_EXTENSIBLE.
8, 16, 24 and 32-bit integer PCM, and 32-bit float.
1–128 channels, any sample rate.
Unsupported or non-WAV files are stored without compression and returned unchanged.

**Compression**
7 levels (-0..-6, default -2):
-0, -1: fixed predictors only, fastest.
-2 and higher: LPC, smallest files.
Inter-channel prediction for multichannel audio (--mc, on at -3 and higher).
Residuals are Rice-coded; ANS is optional (--ans, --ra).
Optional adaptive block size (--ab).

**Speed**
Single-threaded by default; -t N runs up to 64 threads in parallel.
Decoding is fast at every level and uses integer arithmetic only.
Encoder decisions use cheap estimates.

**Reliability**
Every segment and the whole file are verified with checksums (XXH3).
Test decodes and verifies a file without writing output. --verify decodes the output after encoding and compares it with the input.
A damaged file does not stop decoding: damaged parts are listed by segment, time and duration, undecodable parts are filled with silence, and the output is written at full length.
Fuzz-tested: corrupt input produces an error instead of a crash.

**Ease of use**
Coloured summary and progress bar; -q for quiet mode.
Reads from standard input and writes to standard output (-).
Processes multiple files in one command and prints totals.
Three builds by CPU level: AVX2, AVX and SSE2. On an unsupported CPU the program prints a clear error message.

## HALAC Compression Benchmark Results

This benchmark compares HALAC 1.0.0 and HALAC 0.6.6 across five audio datasets, evaluating compressed file size, encoding time, and decoding time.

All results are presented using consistent units and formatting.

---

## KARNOKKOROK 01.wav

**Dataset Information:** 32 channels · 32-bit float · 48 kHz · 3.96 GB

| Version | Level | Compressed Size (%) | Size (MB) | Encoding (s) | Decoding (s) |
|---|---:|---:|---:|---:|---:|
| HALAC 1.0.0 | 0 | 34.25 | 1,354.97 | 4.17 | 3.84 |
| HALAC 1.0.0 | 1 | 33.84 | 1,339.01 | 4.58 | 4.05 |
| HALAC 1.0.0 | 2 | 31.69 | 1,253.95 | 5.03 | 4.58 |
| HALAC 1.0.0 | 3 | 31.64 | 1,251.68 | 7.61 | 4.80 |
| HALAC 1.0.0 | 4 | 31.61 | 1,250.70 | 8.63 | 4.88 |
| HALAC 1.0.0 | 5 | 31.59 | 1,250.08 | 9.26 | 4.97 |
| HALAC 1.0.0 | 6 | 31.54 | 1,248.07 | 12.00 | 5.17 |
| HALAC 0.6.6 | 0 | 36.90 | 1,460.10 | 8.31 | 8.27 |
| HALAC 0.6.6 | 1 | 35.99 | 1,424.17 | 7.34 | 7.71 |
| HALAC 0.6.6 | 2 | 34.23 | 1,354.48 | 7.63 | 8.48 |
| HALAC 0.6.6 | 3 | 33.46 | 1,323.81 | 8.12 | 8.94 |
| HALAC 0.6.6 | 4 | 33.37 | 1,320.44 | 10.08 | 11.13 |
| HALAC 0.6.6 | 5 | 33.38 | 1,320.86 | 10.79 | 11.50 |

---

## BIP TUNIA
**Dataset Information:** 6 tracks · 24-bit · 44.1 kHz · 5.62 GB

| Version | Level | Compressed Size (%) | Size (MB) | Encoding (s) | Decoding (s) |
|---|---:|---:|---:|---:|---:|
| HALAC 1.0.0 | 0 | 75.72 | 4,254.41 | 4.84 | 6.07 |
| HALAC 1.0.0 | 1 | 74.57 | 4,189.79 | 5.21 | 6.56 |
| HALAC 1.0.0 | 2 | 72.29 | 4,061.91 | 6.40 | 7.51 |
| HALAC 1.0.0 | 3 | 72.07 | 4,049.44 | 7.94 | 8.43 |
| HALAC 1.0.0 | 4 | 71.63 | 4,024.64 | 10.99 | 9.76 |
| HALAC 1.0.0 | 5 | 71.54 | 4,019.36 | 13.09 | 9.78 |
| HALAC 1.0.0 | 6 | 70.94 | 3,985.77 | 23.77 | 11.71 |
| HALAC 0.6.6 | 0 | 76.23 | 4,283.02 | 5.20 | 6.58 |
| HALAC 0.6.6 | 1 | 76.00 | 4,269.93 | 5.96 | 7.18 |
| HALAC 0.6.6 | 2 | 73.52 | 4,130.85 | 6.47 | 8.62 |
| HALAC 0.6.6 | 3 | 72.62 | 4,080.34 | 7.41 | 9.67 |
| HALAC 0.6.6 | 4 | 72.19 | 4,056.35 | 10.07 | 12.55 |
| HALAC 0.6.6 | 5 | 72.05 | 4,047.99 | 12.42 | 13.27 |

---

## Yuri Korzunov

**Dataset Information:** 6 files · 24-bit · 192 kHz · 1.90 GB

| Version | Level | Compressed Size (%) | Size (MB) | Encoding (s) | Decoding (s) |
|---|---:|---:|---:|---:|---:|
| HALAC 0.6.6 | 0 | 56.61 | 1,077.27 | 1.80 | 2.20 |
| HALAC 0.6.6 | 1 | 43.24 | 822.74 | 1.92 | 2.48 |
| HALAC 0.6.6 | 2 | 32.38 | 616.23 | 2.09 | 2.96 |
| HALAC 0.6.6 | 3 | 28.87 | 549.35 | 2.32 | 3.23 |
| HALAC 0.6.6 | 4 | 27.47 | 522.65 | 3.90 | 4.21 |
| HALAC 0.6.6 | 5 | 27.17 | 517.07 | 4.40 | 4.44 |
| HALAC 1.0.0 | 0 | 56.08 | 1,067.16 | 1.97 | 2.08 |
| HALAC 1.0.0 | 1 | 42.43 | 807.32 | 1.77 | 2.67 |
| HALAC 1.0.0 | 2 | 27.00 | 513.71 | 2.05 | 2.46 |
| HALAC 1.0.0 | 3 | 24.96 | 474.89 | 3.23 | 2.82 |
| HALAC 1.0.0 | 4 | 22.98 | 437.24 | 5.82 | 3.59 |
| HALAC 1.0.0 | 5 | 22.97 | 437.12 | 6.36 | 3.58 |
| HALAC 1.0.0 | 6 | 22.42 | 426.57 | 14.02 | 3.85 |

---

## Amvizo 5.1

**Dataset Information:** 16-bit · 48 kHz · 5.1 channels · 2.77 GB

| Version | Level | Compressed Size (%) | Size (MB) | Encoding (s) | Decoding (s) |
|---|---:|---:|---:|---:|---:|
| HALAC 0.6.6 | 0 | 48.90 | 1,355.75 | 3.05 | 3.57 |
| HALAC 0.6.6 | 1 | 49.62 | 1,375.89 | 3.56 | 3.97 |
| HALAC 0.6.6 | 2 | 45.59 | 1,264.12 | 3.80 | 4.97 |
| HALAC 0.6.6 | 3 | 43.09 | 1,194.70 | 4.32 | 5.62 |
| HALAC 0.6.6 | 4 | 42.81 | 1,187.06 | 6.44 | 7.38 |
| HALAC 0.6.6 | 5 | 42.60 | 1,181.17 | 7.38 | 7.85 |
| HALAC 1.0.0 | 0 | 48.21 | 1,336.80 | 2.96 | 3.15 |
| HALAC 1.0.0 | 1 | 47.34 | 1,312.52 | 3.06 | 3.55 |
| HALAC 1.0.0 | 2 | 43.32 | 1,201.27 | 3.92 | 4.21 |
| HALAC 1.0.0 | 3 | 38.75 | 1,074.37 | 6.38 | 4.75 |
| HALAC 1.0.0 | 4 | 38.05 | 1,055.11 | 8.34 | 5.62 |
| HALAC 1.0.0 | 5 | **37.81** | **1,048.41** | 10.00 | 5.49 |
| HALAC 1.0.0 | 6 | 37.84 | 1,049.22 | 15.25 | 5.84 |

---
