import pandas as pd
import numpy as np
from random import random

# Load the dataset
df = pd.read_csv('ftp75.csv')

# Convert speed to float
df['Speed (km/h)'] = df['Speed (km/h)'].str.replace(',', '.').astype(float)

# Parameters for smooth variation
INTERNAL_TEMP_RANGE = (20, 25)
EXTERNAL_TEMP_RANGE = (26, 31)
CHANGE_PROBABILITY = 0.01
MAX_TILT_CHANGE = 1.0

# Global state to maintain current values
current_internal = np.random.randint(*INTERNAL_TEMP_RANGE)
current_external = np.random.randint(*EXTERNAL_TEMP_RANGE)
current_engine = current_external  # Initial engine temperature
current_tilt = 0.0
last_moving_tilt = 0.0
was_stopped = True  # Track if vehicle was previously stopped

# Generation functions
def generate_internal_temp():
    global current_internal
    if random() < CHANGE_PROBABILITY:
        new_temp = current_internal + np.random.uniform(-0.5, 0.5)
        current_internal = np.clip(new_temp, *INTERNAL_TEMP_RANGE)
    return round(current_internal)

def generate_external_temp(time):
    global current_external
    hour_effect = 2 * np.sin(2 * np.pi * time / 86400)
    if random() < CHANGE_PROBABILITY:
        new_temp = 28 + hour_effect + np.random.uniform(-1, 1)
        current_external = np.clip(new_temp, *EXTERNAL_TEMP_RANGE)
    return round(current_external)

def generate_engine_temp(speed, ext_temp, prev_engine_temp):
    global current_engine, was_stopped
    
    """ # Check for transition from stopped to moving
    if was_stopped and speed > 0:
        current_engine = 70.0  # Reset to starting temperature
        was_stopped = False
        return current_engine """
    
    # Update stopped state
    was_stopped = (speed == 0)
    
    """ # Target temperature based on operation state
    if speed == 0:
        target_temp = ext_temp  # Engine off tends to ambient temperature
    else:
        target_temp = 90.0  # Ideal operating temperature """
        
    target_temp = 90.0

    # Adjustments based on speed
    if speed < 30:
        target_temp += 2.5  # Slow traffic heats more
    elif speed > 100:
        target_temp -= 1.5  # High speed cools a bit
    
    # Slower change factor to stabilize at 90°C
    temp_change = (target_temp - prev_engine_temp) * 0.1
    
    # Add random variation (greater when moving)
    if speed > 0:
        temp_change += np.random.uniform(-2.5, 2.5)
    
    new_temp = prev_engine_temp + temp_change
    
    # Allow occasional exceedances, but with clear tendency to 90°C
    if new_temp > 105:
        # If overheating, chance of faster cooling
        if random() < 0.3:  # 30% chance of forced cooling
            new_temp -= np.random.uniform(2, 5)
    
    # Limits with small flexibility
    current_engine = np.clip(new_temp, ext_temp - 5, 110)  # Minimum never below ambient-5
    return round(current_engine, 1)

def generate_door_status(speed):
    if speed == 0:
        return np.random.choice([0, 1], p=[0.9, 0.1])
    return 0

def generate_tilt(speed, prev_tilt):
    global current_tilt, last_moving_tilt
    if speed > 0:
        change = np.random.uniform(-MAX_TILT_CHANGE, MAX_TILT_CHANGE)
        new_tilt = prev_tilt + change
        current_tilt = np.clip(new_tilt, 0, 10)
        last_moving_tilt = current_tilt
    else:
        current_tilt = last_moving_tilt
    return round(current_tilt, 1)

# First, create all necessary columns
df['Tilt Angle (deg)'] = 0.0
df['Internal Temp (C)'] = [generate_internal_temp() for _ in range(len(df))]
df['External Temp (C)'] = [generate_external_temp(t) for t in df['Time (seconds)']]
df['Door Open'] = df['Speed (km/h)'].apply(generate_door_status)
df['Engine Temp (C)'] = df['External Temp (C)'].astype(float)  # Initialize with external temp

# Now simulate engine temperature and tilt sequentially
for i in range(1, len(df)):
    speed = df.at[i, 'Speed (km/h)']
    prev_tilt = df.at[i-1, 'Tilt Angle (deg)']
    ext_temp = df.at[i-1, 'External Temp (C)']
    prev_engine = df.at[i-1, 'Engine Temp (C)']
    
    df.at[i, 'Tilt Angle (deg)'] = generate_tilt(speed, prev_tilt)
    df.at[i, 'Engine Temp (C)'] = generate_engine_temp(speed, ext_temp, prev_engine)

# Initialize first row's engine temperature if starting from stop
if df.at[0, 'Speed (km/h)'] == 0:
    df.at[0, 'Engine Temp (C)'] = df.at[0, 'External Temp (C)']
else:
    df.at[0, 'Engine Temp (C)'] = 70.0

# Save the new dataset
df.to_csv('full_simu.csv', index=False)