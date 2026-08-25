import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../theme';
import { SensorData } from '../types';
import SensorCard from '../components/SensorCard';
import DerivedMetric from '../components/DerivedMetric';
import { dewPoint, comfortIndex, weatherCondition } from '../utils/calculations';

export default function GeneralView({ data }: { data: SensorData }) {
  const dp = dewPoint(data.temperatura, data.humedad).toFixed(1);
  const comfort = comfortIndex(data.temperatura, data.humedad);
  const weather = weatherCondition(data.temperatura, data.humedad);

  return (
    <View style={styles.container}>
      {/* Weather Box */}
      <View style={styles.weatherBox}>
        <Text style={styles.weatherEmoji}>{weather.emoji}</Text>
        <Text style={styles.weatherText}>{weather.text}</Text>
      </View>

      {/* Primary Sensors */}
      <View style={styles.row}>
        <SensorCard
          label="Temperatura" value={data.temperatura?.toFixed(1) ?? '--'} unit="°C"
          icon="🌡️" accent={colors.orange}
          subtitle={`Sensación: ${data.sens_termica?.toFixed(1)}°C`}
        />
        <SensorCard
          label="Humedad" value={data.humedad?.toFixed(1) ?? '--'} unit="%"
          icon="💧" accent={colors.blue}
          barPct={data.humedad} barColor={data.humedad > 80 ? colors.red : data.humedad > 60 ? colors.amber : colors.blue}
        />
      </View>

      <View style={styles.row}>
        <SensorCard
          label="Presión Atm." value={data.presion?.toFixed(1) ?? '--'} unit="hPa"
          icon="📊" accent={colors.purple}
          subtitle={`Altitud: ${Math.round(data.altitud)}m`}
        />
        <SensorCard
          label="Batería" value={data.bateria_mV ?? '--'} unit="mV"
          icon="🔋" accent={data.bateria_mV < 3300 ? colors.amber : colors.green}
          barPct={(data.bateria_mV / 4200) * 100}
          barColor={data.bateria_mV < 3000 ? colors.red : data.bateria_mV < 3300 ? colors.amber : colors.green}
        />
      </View>

      {/* Derived Metrics */}
      <View style={styles.derivedRow}>
        <DerivedMetric icon="🎯" value={comfort.level} label="Confort" color={comfort.color}
          barPct={comfort.pct} barColor={comfort.color} />
        <DerivedMetric icon="💦" value={`${dp}°`} label="P. Rocío" color={colors.cyan} />
        <DerivedMetric icon="⛰️" value={`${Math.round(data.altitud)}m`} label="Altitud" color={colors.green} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { gap: 12 },
  weatherBox: {
    backgroundColor: '#1a2340',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: 'rgba(245,158,11,0.3)',
    padding: 20,
    alignItems: 'center',
    justifyContent: 'center',
  },
  weatherEmoji: { fontSize: 44 },
  weatherText: {
    fontSize: 12,
    fontWeight: '600',
    color: colors.textSecondary,
    textTransform: 'uppercase',
    letterSpacing: 2,
    marginTop: 6,
  },
  row: { flexDirection: 'row', gap: 10 },
  derivedRow: { flexDirection: 'row', gap: 8 },
});
