import pandas as pd
import numpy as np
from random import random

# Carregar o dataset
df = pd.read_csv('ftp75.csv')

# Converter velocidade para float
df['Speed (km/h)'] = df['Speed (km/h)'].str.replace(',', '.').astype(float)

# Parâmetros para variação suave
INTERNAL_TEMP_RANGE = (21, 25)
EXTERNAL_TEMP_RANGE = (26, 31)
CHANGE_PROBABILITY = 0.01
MAX_TILT_CHANGE = 1.5

# Estado global para manter os valores atuais
current_internal = np.random.randint(*INTERNAL_TEMP_RANGE)
current_external = np.random.randint(*EXTERNAL_TEMP_RANGE)
current_engine = current_external  # Começa igual à temperatura externa
current_tilt = 0.0
last_moving_tilt = 0.0

# Funções de geração (mantidas iguais)
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
    global current_engine
    
    # Temperatura alvo baseada no estado de operação
    if speed == 0:
        target_temp = ext_temp  # Motor desligado tende à temperatura ambiente
    else:
        target_temp = 90.0  # Temperatura ideal de operação
        
        # Ajustes baseados na velocidade
        if speed < 30:
            target_temp += 2.5  # Trânsito lento aquece mais
        elif speed > 100:
            target_temp -= 1.5  # Alta velocidade resfria um pouco
    
    # Fator de mudança mais lento para estabilizar em 90°C
    temp_change = (target_temp - prev_engine_temp) * 0.1
    
    # Adicionar variação aleatória (maior quando em movimento)
    if speed > 0:
        temp_change += np.random.uniform(-2.5, 2.5)
    
    new_temp = prev_engine_temp + temp_change
    
    # Permitir excedências ocasionais, mas com tendência clara para 90°C
    if new_temp > 105:
        # Se estiver superaquecendo, chance de resfriamento mais rápido
        if random() < 0.3:  # 30% de chance de resfriamento forçado
            new_temp -= np.random.uniform(2, 5)
    
    # Limites com pequena flexibilidade
    current_engine = np.clip(new_temp, ext_temp - 5, 110)  # Mínimo nunca abaixo da ambiente-5
    return round(current_engine, 1)

def generate_door_status(speed):
    if speed == 0:
        return np.random.choice([0, 1], p=[0.8, 0.2])
    return 0

def generate_tilt(speed, prev_tilt):
    global current_tilt, last_moving_tilt
    if speed > 0:
        change = np.random.uniform(-MAX_TILT_CHANGE, MAX_TILT_CHANGE)
        new_tilt = prev_tilt + change
        current_tilt = np.clip(new_tilt, 0, 20)
        last_moving_tilt = current_tilt
    else:
        current_tilt = last_moving_tilt
    return round(current_tilt, 1)

# Primeiro, criar todas as colunas necessárias
df['Tilt Angle (deg)'] = 0.0
df['Internal Temp (C)'] = [generate_internal_temp() for _ in range(len(df))]
df['External Temp (C)'] = [generate_external_temp(t) for t in df['Time (seconds)']]
df['Door Open'] = df['Speed (km/h)'].apply(generate_door_status)
df['Engine Temp (C)'] = df['External Temp (C)'].astype(float)  # Inicializa com temp externa

# Agora simular a temperatura do motor e inclinação sequencialmente
for i in range(1, len(df)):
    speed = df.at[i, 'Speed (km/h)']
    prev_tilt = df.at[i-1, 'Tilt Angle (deg)']
    ext_temp = df.at[i-1, 'External Temp (C)']
    prev_engine = df.at[i-1, 'Engine Temp (C)']
    
    df.at[i, 'Tilt Angle (deg)'] = generate_tilt(speed, prev_tilt)
    df.at[i, 'Engine Temp (C)'] = generate_engine_temp(speed, ext_temp, prev_engine)

# Salvar o novo dataset
df.to_csv('full_simu.csv', index=False)