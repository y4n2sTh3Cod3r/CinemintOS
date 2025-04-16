import sys
import wave
import struct
import numpy as np
import os

def wav_to_header(wav_file, raw):
    header_file = "build/wav_" + raw + ".h"
    with wave.open(wav_file, 'rb') as wav:
        n_channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        framerate = wav.getframerate()
        n_frames = wav.getnframes()
        
        # Read all frames
        frames = wav.readframes(n_frames)
        
        # Convert to appropriate format based on sample width
        if sample_width == 1:
            # 8-bit samples are unsigned
            samples = struct.unpack('%dB' % (n_frames * n_channels), frames)
        elif sample_width == 2:
            # 16-bit samples are signed
            samples = struct.unpack('%dh' % (n_frames * n_channels), frames)
        else:
            print("Unsupported sample width:", sample_width)
            return
        
        # Take only first channel
        samples = samples[::n_channels]
        
        # Downsample to make it more suitable for PC speaker
        # Target 8kHz sample rate for PC speaker
        target_rate = 8000
        if framerate > target_rate:
            downsample_factor = framerate // target_rate
            samples = samples[::downsample_factor]
            framerate = framerate // downsample_factor
        
        # Apply a low-pass filter to smooth the signal
        # This is a simple moving average filter
        window_size = 3
        if len(samples) > window_size:
            smoothed_samples = []
            for i in range(len(samples)):
                window_start = max(0, i - window_size // 2)
                window_end = min(len(samples), i + window_size // 2 + 1)
                window = samples[window_start:window_end]
                smoothed_samples.append(sum(window) // len(window))
            samples = smoothed_samples
        
        # Write header file
        with open(header_file, 'w') as f:
            f.write('#ifndef WAV_DATA_H\n')
            f.write('#define WAV_DATA_H\n\n')
            
            f.write('#include <stdint.h>\n\n')
            
            f.write('namespace wav_' + raw + ' {\n\n')
            
            f.write('    const uint32_t SAMPLE_RATE = %d;\n' % framerate)
            f.write('    const uint8_t NUM_CHANNELS = 1;\n')  # Always mono for PC speaker
            f.write('    const uint8_t BITS_PER_SAMPLE = %d;\n' % (sample_width * 8))
            f.write('    const uint32_t NUM_SAMPLES = %d;\n\n' % len(samples))
            
            f.write('    const %s SAMPLES[NUM_SAMPLES] = {\n        ' % 
                   ('uint8_t' if sample_width == 1 else 'int16_t'))
            
            # Write samples in rows of 12
            for i, sample in enumerate(samples):
                if i > 0 and i % 12 == 0:
                    f.write('\n        ')
                f.write('%d, ' % sample)
            
            f.write('\n    };\n\n')
            
            f.write('} // namespace wav\n\n')
            
            f.write('#endif // WAV_DATA_H')

for file in os.listdir("audio"):
    print("- importing " + file)
    raw = file[:file.index(".")]
    wav_to_header(os.path.join("audio", file), raw)