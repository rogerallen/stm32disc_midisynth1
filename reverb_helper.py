#!/usr/bin/env python3
# calculating delays for our 48kHz sample rate
FRAME_RATE = 48000
comb_delays_ms = [36.04, 31.12, 40.44, 44.92]
allp_delays_ms = [5, 1.68, 0.48]

print(f"FRAME_RATE = {FRAME_RATE}")
count = 0
for i,d in enumerate(comb_delays_ms):
    x = int( (d/1000) * FRAME_RATE + 0.5)
    count += x
    print(f"comb{i}: {d:6.3f}ms -> {x} frames")

for i,d in enumerate(allp_delays_ms):
    x = int( (d/1000) * FRAME_RATE + 0.5)
    count += x
    print(f"allp{i}: {d:6.3f}ms -> {x} frames")

print(f"total frame count = {count} or {count*4} bytes")