#!/usr/bin/env python3
"""
Web Interface for OpenTimeInstrument
Real-time visualization of PTP TIE measurements
"""

import os
import sys
import json
import time
import subprocess
import threading
from collections import deque
from datetime import datetime
import signal

from flask import Flask, render_template, jsonify, request, Response
from flask_socketio import SocketIO, emit
import plotly.graph_objs as go
import plotly.utils

app = Flask(__name__)
app.config['SECRET_KEY'] = 'opentimeinstrument_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

# Data storage for measurements
measurement_data = {
    'channel_1': deque(maxlen=1000),
    'channel_2': deque(maxlen=1000),
    'channel_3': deque(maxlen=1000),
    'channel_4': deque(maxlen=1000)
}

measurement_timestamps = {
    'channel_1': deque(maxlen=1000),
    'channel_2': deque(maxlen=1000),
    'channel_3': deque(maxlen=1000),
    'channel_4': deque(maxlen=1000)
}

measurement_stats = {
    'channel_1': {'count': 0, 'min': None, 'max': None, 'avg': 0},
    'channel_2': {'count': 0, 'min': None, 'max': None, 'avg': 0},
    'channel_3': {'count': 0, 'min': None, 'max': None, 'avg': 0},
    'channel_4': {'count': 0, 'min': None, 'max': None, 'avg': 0}
}

# Global variables for measurement monitoring
measurement_process = None
monitoring_active = False
monitoring_thread = None

def scan_ptp_devices():
    """Scan for available PTP devices"""
    devices = []
    
    # Always add demo mode
    devices.append({
        'number': 'demo',
        'path': '/dev/demo',
        'accessible': True
    })
    
    try:
        for i in range(8):  # Check ptp0 through ptp7
            device_path = f"/dev/ptp{i}"
            if os.path.exists(device_path):
                try:
                    # Check if device is accessible
                    accessible = os.access(device_path, os.R_OK | os.W_OK)
                    devices.append({
                        'number': i,
                        'path': device_path,
                        'accessible': accessible
                    })
                except:
                    pass
    except Exception as e:
        print(f"Error scanning PTP devices: {e}")
    
    return devices

def parse_measurement_line(line):
    """Parse measurement output line"""
    # Expected format: "*****Measurement channel X: Y ns" or "*****Measurement channel X: -Y ns"
    if "*****Measurement channel" in line:
        try:
            parts = line.split(":")
            if len(parts) >= 2:
                channel_part = parts[0].split("channel")[-1].strip()
                channel = int(channel_part)
                
                value_part = parts[1].strip()
                if value_part.endswith("ns"):
                    value_str = value_part[:-2].strip()
                    if value_str.startswith('-'):
                        value = -int(value_str[1:])
                    else:
                        value = int(value_str)
                    
                    return channel, value
        except Exception as e:
            print(f"Error parsing line '{line}': {e}")
    
    return None, None

def monitor_measurements():
    """Monitor measurement output in a separate thread"""
    global measurement_process, monitoring_active
    
    while monitoring_active:
        try:
            if measurement_process and measurement_process.poll() is None:
                line = measurement_process.stdout.readline()
                if line:
                    line = line.decode('utf-8').strip()
                    channel, value = parse_measurement_line(line)
                    
                    if channel is not None and value is not None:
                        timestamp = datetime.now()
                        channel_key = f'channel_{channel}'
                        
                        # Store data
                        measurement_data[channel_key].append(value)
                        measurement_timestamps[channel_key].append(timestamp)
                        
                        # Update statistics
                        stats = measurement_stats[channel_key]
                        stats['count'] += 1
                        if stats['min'] is None or value < stats['min']:
                            stats['min'] = value
                        if stats['max'] is None or value > stats['max']:
                            stats['max'] = value
                        
                        # Calculate running average
                        if len(measurement_data[channel_key]) > 0:
                            stats['avg'] = sum(measurement_data[channel_key]) / len(measurement_data[channel_key])
                        
                        # Emit real-time data via WebSocket
                        print(f"Emitting WebSocket event: channel={channel}, value={value}")
                        socketio.emit('measurement_update', {
                            'channel': channel,
                            'value': value,
                            'timestamp': timestamp.isoformat(),
                            'stats': stats
                        })
            else:
                time.sleep(0.1)
        except Exception as e:
            print(f"Error in monitoring thread: {e}")
            time.sleep(1)

def start_measurement(device_path):
    """Start measurement process"""
    global measurement_process, monitoring_active, monitoring_thread
    
    try:
        # Stop any existing measurement
        stop_measurement()
        
        # Check if demo mode requested
        if device_path == "demo" or device_path == "/dev/demo":
            # Start demo data generator
            cmd = ['python3', 'demo_data_generator.py']
        else:
            # Start OpenTimeInstrument with continuous measurement
            cmd = ['./OpenTimeInstrument', '-d', device_path, '-e', '-1']
        
        measurement_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=False,
            bufsize=1
        )
        
        # Start monitoring thread
        monitoring_active = True
        monitoring_thread = threading.Thread(target=monitor_measurements)
        monitoring_thread.daemon = True
        monitoring_thread.start()
        
        return True
    except Exception as e:
        print(f"Error starting measurement: {e}")
        return False

def stop_measurement():
    """Stop measurement process"""
    global measurement_process, monitoring_active, monitoring_thread
    
    monitoring_active = False
    
    if measurement_process:
        try:
            measurement_process.terminate()
            measurement_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            measurement_process.kill()
        except Exception as e:
            print(f"Error stopping measurement process: {e}")
        measurement_process = None
    
    if monitoring_thread and monitoring_thread.is_alive():
        monitoring_thread.join(timeout=2)
    monitoring_thread = None

@app.route('/')
def index():
    """Main web interface"""
    devices = scan_ptp_devices()
    return render_template('index.html', devices=devices)

@app.route('/api/devices')
def api_devices():
    """Get available PTP devices"""
    devices = scan_ptp_devices()
    return jsonify(devices)

@app.route('/api/start_measurement', methods=['POST'])
def api_start_measurement():
    """Start measurement on specified device"""
    data = request.json
    device_path = data.get('device_path')
    
    if not device_path:
        return jsonify({'success': False, 'error': 'Device path required'})
    
    success = start_measurement(device_path)
    return jsonify({'success': success})

@app.route('/api/stop_measurement', methods=['POST'])
def api_stop_measurement():
    """Stop measurement"""
    stop_measurement()
    return jsonify({'success': True})

@app.route('/api/measurement_data')
def api_measurement_data():
    """Get current measurement data"""
    data = {}
    for channel_key in measurement_data:
        channel_data = list(measurement_data[channel_key])
        timestamps = [ts.isoformat() for ts in measurement_timestamps[channel_key]]
        data[channel_key] = {
            'values': channel_data,
            'timestamps': timestamps,
            'stats': measurement_stats[channel_key]
        }
    return jsonify(data)

@app.route('/api/clear_data', methods=['POST'])
def api_clear_data():
    """Clear all measurement data"""
    for channel_key in measurement_data:
        measurement_data[channel_key].clear()
        measurement_timestamps[channel_key].clear()
        measurement_stats[channel_key] = {'count': 0, 'min': None, 'max': None, 'avg': 0}
    
    socketio.emit('data_cleared')
    return jsonify({'success': True})

@app.route('/api/export_data')
def api_export_data():
    """Export measurement data as CSV"""
    import csv
    from io import StringIO
    
    output = StringIO()
    writer = csv.writer(output)
    
    # Write header
    writer.writerow(['Timestamp', 'Channel', 'Value_ns'])
    
    # Combine all data with timestamps
    all_data = []
    for channel_key in measurement_data:
        channel_num = channel_key.split('_')[1]
        for i, value in enumerate(measurement_data[channel_key]):
            if i < len(measurement_timestamps[channel_key]):
                timestamp = measurement_timestamps[channel_key][i]
                all_data.append((timestamp, channel_num, value))
    
    # Sort by timestamp
    all_data.sort(key=lambda x: x[0])
    
    # Write data
    for timestamp, channel, value in all_data:
        writer.writerow([timestamp.isoformat(), channel, value])
    
    output.seek(0)
    return Response(
        output.getvalue(),
        mimetype='text/csv',
        headers={'Content-Disposition': 'attachment; filename=tie_measurements.csv'}
    )

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    emit('status', {'message': 'Connected to OpenTimeInstrument Web Interface'})

@socketio.on('disconnect')
def handle_disconnect():
    """Handle WebSocket disconnection"""
    print('Client disconnected')

def signal_handler(sig, frame):
    """Handle shutdown signals"""
    print('\nShutting down...')
    stop_measurement()
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    print("Starting OpenTimeInstrument Web Interface...")
    print("Access the interface at: http://localhost:5000")
    
    # Create templates directory if it doesn't exist
    os.makedirs('templates', exist_ok=True)
    os.makedirs('static', exist_ok=True)
    
    try:
        socketio.run(app, host='0.0.0.0', port=5000, debug=True, allow_unsafe_werkzeug=True)
    finally:
        stop_measurement()