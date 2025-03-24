import pandas as pd
import numpy as np
from random import random

# Carregar o dataset
df = pd.read_csv('ftp75.csv')

# Converter velocidade para float
df['Speed (km/h)'] = df['Speed (km/h)'].str.replace(',', '.').astype(float)

# Parâmetros para variação suave
INTERNAL_TEMP_RANGE = (21, 24)
EXTERNAL_TEMP_RANGE = (26, 31)
CHANGE_PROBABILITY = 0.01
MAX_TILT_CHANGE = 1.5  # Máxima variação por segundo (graus)

# Estado global para manter os valores atuais
current_internal = np.random.randint(*INTERNAL_TEMP_RANGE)
current_external = np.random.randint(*EXTERNAL_TEMP_RANGE)
current_tilt = 0.0  # Valor inicial médio
last_moving_tilt = 0.0  # Último valor registrado em movimento

# Função para temperatura interna
def generate_internal_temp():
    global current_internal
    if random() < CHANGE_PROBABILITY:
        new_temp = current_internal + np.random.uniform(-0.5, 0.5)
        current_internal = np.clip(new_temp, *INTERNAL_TEMP_RANGE)
    return round(current_internal)

# Função para temperatura externa
def generate_external_temp(time):
    global current_external
    hour_effect = 2 * np.sin(2 * np.pi * time / 86400)  # Variação diurna
    if random() < CHANGE_PROBABILITY:
        new_temp = 28 + hour_effect + np.random.uniform(-1, 1)
        current_external = np.clip(new_temp, *EXTERNAL_TEMP_RANGE)
    return round(current_external)

# Função para status da porta
def generate_door_status(speed):
    if speed == 0:
        return np.random.choice([0, 1], p=[0.8, 0.2])  # 20% chance de porta aberta quando parado
    return 0  # Sempre fechada em movimento

# Função para inclinação com variação suave
def generate_tilt(speed, prev_tilt):
    global current_tilt, last_moving_tilt
    
    if speed > 0:  # Veículo em movimento
        # Variação suave limitada
        change = np.random.uniform(-MAX_TILT_CHANGE, MAX_TILT_CHANGE)
        new_tilt = prev_tilt + change
        
        # Limitar entre 0-20 graus
        new_tilt = np.clip(new_tilt, 0, 20)
        
        # Atualizar valores
        current_tilt = new_tilt
        last_moving_tilt = current_tilt
    else:  # Veículo parado
        current_tilt = last_moving_tilt  # Mantém o último valor válido
    
    return round(current_tilt, 1)

# Preparar as colunas
df['Tilt Angle (deg)'] = 0.0  # Inicializar

# Gerar valores sequencialmente para inclinação (importante para suavidade)
for i in range(len(df)):
    if i == 0:
        df.at[i, 'Tilt Angle (deg)'] = 0.0  # Valor inicial
    else:
        speed = df.at[i, 'Speed (km/h)']
        prev_tilt = df.at[i-1, 'Tilt Angle (deg)']
        df.at[i, 'Tilt Angle (deg)'] = generate_tilt(speed, prev_tilt)

# Gerar outras colunas
df['Internal Temp (C)'] = [generate_internal_temp() for _ in range(len(df))]
df['External Temp (C)'] = [generate_external_temp(t) for t in df['Time (seconds)']]
df['Door Open'] = df['Speed (km/h)'].apply(generate_door_status)

# Salvar o novo dataset
df.to_csv('full_simu.csv', index=False)