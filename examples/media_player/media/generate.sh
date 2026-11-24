#!/bin/sh
# Makes the media of examples/media_player (also used by the tests of
# VideoView and Rsound) with FFmpeg: synthetic pictures and tones, nothing
# recorded, so the files carry no third-party content. Run it from this folder;
# the files it writes are committed, the build does not run it.
#
#   clip.mp4   2 s, 160 x 120, 24 fps H.264 (the test pattern of FFmpeg,
#              moving) and a 440 Hz tone in AAC, about 32 KB
#   zoom.mp4   3 s, 160 x 120, a Mandelbrot zoom and a 660 Hz tone, about 38 KB
#   click.wav  a 0.12 s click at 880 Hz, 16-bit PCM, 22 050 Hz, mono
#   chime.wav  a 1.2 s C major chord that fades out, 16-bit PCM, mono
set -e
ffmpeg -v error -y -f lavfi -i "testsrc2=size=160x120:rate=24:duration=2" \
	-f lavfi -i "sine=frequency=440:sample_rate=44100:duration=2" \
	-c:v libx264 -preset veryslow -crf 28 -pix_fmt yuv420p -profile:v main \
	-c:a aac -b:a 48k -ac 1 -movflags +faststart -map_metadata -1 -fflags +bitexact clip.mp4
ffmpeg -v error -y -f lavfi -i "mandelbrot=size=160x120:rate=24:end_scale=0.6" \
	-f lavfi -i "sine=frequency=660:sample_rate=44100:duration=3,volume=0.5" -t 3 \
	-c:v libx264 -preset veryslow -crf 30 -pix_fmt yuv420p -profile:v main \
	-c:a aac -b:a 48k -ac 1 -movflags +faststart -map_metadata -1 -fflags +bitexact zoom.mp4
ffmpeg -v error -y -f lavfi -i "sine=frequency=880:sample_rate=22050:duration=0.12,afade=t=out:st=0.04:d=0.08" \
	-c:a pcm_s16le -ac 1 -map_metadata -1 -fflags +bitexact click.wav
ffmpeg -v error -y -f lavfi \
	-i "aevalsrc=0.3*sin(2*PI*(523.25*t))*exp(-3*t)+0.3*sin(2*PI*659.25*t)*exp(-3*t)+0.3*sin(2*PI*783.99*t)*exp(-3*t):s=22050:d=1.2" \
	-c:a pcm_s16le -ac 1 -map_metadata -1 -fflags +bitexact chime.wav
