#!/usr/bin/env python3
"""
Demo data generator for OpenTimeInstrument
Simulates measurement output for testing web and GUI interfaces
"""

import time
import random
import math
import sys

def generate_measurements():
    """Generate simulated TIE measurements"""
    
    print("VER:;1;")
    print("DataType:;TIEDATA; Format:;CSV;")
    print("MeasType:;1pps TE Absolute;")
    print("Port:;A;")
    
    # Simulate startup
    time.sleep(1)
    
    measurement_count = 0
    start_time = time.time()
    
    # Different patterns for each channel
    patterns = {
        1: {'offset': 100, 'amplitude': 50, 'frequency': 0.1},    # Slow sine wave
        2: {'offset': -50, 'amplitude': 30, 'frequency': 0.2},    # Faster sine wave  
        3: {'offset': 0, 'amplitude': 80, 'frequency': 0.05},     # Very slow wave
        4: {'offset': 200, 'amplitude': 20, 'frequency': 0.3}     # High frequency
    }
    
    try:
        while True:
            current_time = time.time() - start_time
            
            # Generate measurements for each channel
            for channel in [1, 2, 3, 4]:
                pattern = patterns[channel]
                
                # Base sine wave
                base_value = pattern['offset'] + pattern['amplitude'] * math.sin(
                    2 * math.pi * pattern['frequency'] * current_time
                )
                
                # Add some noise
                noise = random.gauss(0, 5)
                
                # Occasional spikes
                if random.random() < 0.02:  # 2% chance of spike
                    spike = random.choice([-1, 1]) * random.randint(100, 300)
                    noise += spike
                
                value = int(base_value + noise)
                
                # Output in the same format as OpenTimeInstrument
                if value < 0:
                    print(f"*****Measurement channel {channel}: -{abs(value)} ns")
                else:
                    print(f"*****Measurement channel {channel}: {value} ns")
                
                sys.stdout.flush()
                
                # Small delay between channels
                time.sleep(0.01)
            
            measurement_count += 1
            
            # Status update every 50 measurements
            if measurement_count % 50 == 0:
                print(f"Generated {measurement_count} measurement sets")
                sys.stdout.flush()
            
            # Main delay between measurement cycles
            time.sleep(0.2)  # 5 measurements per second per channel
            
    except KeyboardInterrupt:
        print("\nDemo data generation stopped")
        sys.exit(0)

if __name__ == "__main__":
    print("OpenTimeInstrument Demo Data Generator")
    print("Simulating TIE measurements for 4 channels")
    print("Press Ctrl+C to stop")
    print("-" * 50)
    
    generate_measurements()