import pandas as pd
import numpy as np
from random import random

# Load dataset
df = pd.read_csv('ftp75.csv')

# Convert speed to float
df['Speed (km/h)'] = df['Speed (km/h)'].str.replace(',', '.').astype(float)

# Parameters
INTERNAL_TEMP_RANGE = (21, 25)
EXTERNAL_TEMP_RANGE = (26, 31)
CHANGE_PROBABILITY = 0.01
MAX_TILT_CHANGE = 1.5
TEMP_SET = 22  # Default temperature setting
MAX_TEMP_DIFF = 3  # Maximum allowed difference from temp_set
ENGINE_TEMP_IDLE_TARGET = 87.5  # Target temperature when idle
ENGINE_TEMP_VARIANCE = 0.3  # Probability of temp going outside normal range

# Global state
current_internal = np.random.randint(*INTERNAL_TEMP_RANGE)
current_external = np.random.randint(*EXTERNAL_TEMP_RANGE)
current_engine = current_external
current_tilt = 0.0
last_moving_tilt = 0.0
door_status = 0
abnormal_engine_temp = False  # Track if we're in abnormal temp range
abnormal_duration = 0  # How long we've been in abnormal range

# Generation functions (same as before)
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

def generate_engine_temp(speed, ext_temp, prev_engine_temp, door_open):
    global current_engine, abnormal_engine_temp, abnormal_duration
    
    if speed == 0:
        # If door is open, engine cools faster toward ambient
        if door_open:
            cooling_factor = 0.2  # Faster cooling when door is open
            target_temp = ext_temp + np.random.uniform(-2, 2)
        else:
            cooling_factor = 0.1
            # Sometimes allow temperature to go outside normal range
            if not abnormal_engine_temp and random() < ENGINE_TEMP_VARIANCE:
                abnormal_engine_temp = True
                abnormal_duration = np.random.randint(10, 60)  # Duration in samples
            
            if abnormal_engine_temp:
                # Generate abnormal temperature (outside 70-105)
                if random() < 0.5:  # 50% chance for high or low
                    # High temperature (105-110)
                    target_temp = 105 + np.random.uniform(0, 5)
                else:
                    # Low temperature (ext_temp-5 to 70)
                    target_temp = ext_temp - np.random.uniform(0, 5)
                
                abnormal_duration -= 1
                if abnormal_duration <= 0:
                    abnormal_engine_temp = False
            else:
                # Normal operation - tend toward target temperature
                target_temp = ENGINE_TEMP_IDLE_TARGET + np.random.uniform(-2, 2)
        
        # Smooth transition toward target temperature
        temp_change = (target_temp - prev_engine_temp) * cooling_factor
        new_temp = prev_engine_temp + temp_change
        
    else:
        # Moving behavior (same as before)
        target_temp = 90.0
        if speed < 30:
            target_temp += 2.5
        elif speed > 100:
            target_temp -= 1.5
        
        temp_change = (target_temp - prev_engine_temp) * 0.1
        temp_change += np.random.uniform(-2.5, 2.5)
        new_temp = prev_engine_temp + temp_change
        
        # Reset abnormal state when moving
        abnormal_engine_temp = False
    
    current_engine = np.clip(new_temp, ext_temp - 5, 110)
    return round(current_engine, 1)

def generate_door_status(speed, internal_temp, external_temp, engine_temp):
    global door_status
    
    if speed == 0:
        # Conditions when door should open sometimes
        condition_met = (internal_temp <= (TEMP_SET + MAX_TEMP_DIFF) and 
                        external_temp <= TEMP_SET and
                        (engine_temp < 70 or engine_temp > 105))
        
        if condition_met:
            # Higher probability to open door when conditions are met
            if random() < 0.4:  # 40% chance to change state
                door_status = 1 if door_status == 0 else 0
        else:
            # Lower probability otherwise
            if random() < 0.1:  # 10% chance to change state
                door_status = 1 if door_status == 0 else 0
    else:
        door_status = 0
        
    return door_status

def generate_tilt(speed, prev_tilt, internal_temp, external_temp, engine_temp):
    global current_tilt, last_moving_tilt
    
    if speed > 0:
        change = np.random.uniform(-MAX_TILT_CHANGE, MAX_TILT_CHANGE)
        new_tilt = prev_tilt + change
        current_tilt = np.clip(new_tilt, 0, 20)
        last_moving_tilt = current_tilt
    else:
        # When stopped, ensure tilt is >5 if conditions are met
        condition_met = (internal_temp <= (TEMP_SET + MAX_TEMP_DIFF) and 
                         external_temp <= TEMP_SET and
                         (engine_temp < 70 or engine_temp > 105))
        
        if condition_met:
            if last_moving_tilt <= 5:
                # Gradually increase tilt if it's <=5
                current_tilt = min(5.1, last_moving_tilt + np.random.uniform(0.3, 0.7))
            else:
                # Small variations around current tilt
                current_tilt = last_moving_tilt + np.random.uniform(-0.5, 0.5)
            
            # Ensure tilt stays within bounds
            current_tilt = np.clip(current_tilt, 0, 20)
            last_moving_tilt = current_tilt
        else:
            current_tilt = last_moving_tilt
            
    return round(current_tilt, 1)

# Create all columns in correct order
df['Tilt Angle (deg)'] = 0.0
df['Internal Temp (C)'] = [generate_internal_temp() for _ in range(len(df))]
df['External Temp (C)'] = [generate_external_temp(t) for t in df['Time (seconds)']]
df['Door Open'] = 0  # Initialize door status column first
df['Engine Temp (C)'] = df['External Temp (C)'].astype(float)  # Initialize engine temp

# Sequential simulation
for i in range(1, len(df)):
    speed = df.at[i, 'Speed (km/h)']
    prev_tilt = df.at[i-1, 'Tilt Angle (deg)']
    ext_temp = df.at[i-1, 'External Temp (C)']
    prev_engine = df.at[i-1, 'Engine Temp (C)']
    int_temp = df.at[i, 'Internal Temp (C)']
    
    # First generate door status based on previous engine temp
    df.at[i, 'Door Open'] = generate_door_status(
        speed, int_temp, ext_temp, prev_engine
    )
    door_open = df.at[i, 'Door Open']
    
    # Then generate engine temp which can use door status
    df.at[i, 'Engine Temp (C)'] = generate_engine_temp(
        speed, ext_temp, prev_engine, door_open
    )
    engine_temp = df.at[i, 'Engine Temp (C)']
    
    # Finally generate tilt which uses all current values
    df.at[i, 'Tilt Angle (deg)'] = generate_tilt(
        speed, prev_tilt, int_temp, ext_temp, engine_temp
    )

# Save the new dataset
df.to_csv('full_simu.csv', index=False)