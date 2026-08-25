import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../theme';
import { SensorData } from '../types';
import SensorCard from '../components/SensorCard';
import DerivedMetric from '../components/DerivedMetric';
import { evapRate, brineConcentration } from '../utils/calculations';

export default function LitioView({ data }: { data: SensorData }) {
  const evap = evapRate(data.temperatura, data.humedad, data.presion, data.altitud).toFixed(2);
  const conc = brineConcentration(data.temperatura, data.humedad);
  const evapQuality = parseFloat(evap) > 3 ? 'Óptima' : parseFloat(evap) > 1.5 ? 'Moderada' : 'Lenta';
  const evapColor = parseFloat(evap) > 3 ? colors.green : parseFloat(evap) > 1.5 ? colors.amber : colors.red;
  const satPct = Math.min(100, (parseFloat(conc) / 3.5) * 100);
  const humLabel = data.humedad < 30 ? 'Favorable' : data.humedad < 60 ? 'Moderada' : 'Limitada';
  const humColor = data.humedad < 30 ? colors.green : data.humedad < 60 ? colors.amber : colors.red;

  return (
    <View style={styles.container}>
      {/* Brine Pool Visual */}
      <View style={styles.poolBox}>
        <Text style={styles.poolSun}>☀️</Text>
        <Text style={styles.poolTemp}>{data.temperatura?.toFixed(1)}°C</Text>
        <View style={styles.poolWaves}>
          <Text style={styles.waveText}>〰️ 〰️ 〰️ 〰️ 〰️ 〰️</Text>
        </View>
        <Text style={styles.poolLabel}>▼ PILETA DE SALMUERA — Li₂CO₃ ▼</Text>
      </View>

      {/* Primary: Evaporation + Concentration + Humidity */}
      <View style={styles.row}>
        <SensorCard
          label="Tasa Evaporación" value={evap} unit="mm/día"
          icon="☀️" accent={evapColor}
          subtitle={evapQuality} subtitleColor={evapColor}
          barPct={parseFloat(evap) * 15} barColor={evapColor}
        />
        <SensorCard
          label="Concentración Li" value={conc} unit="%"
          icon="🧪" accent={colors.cyan}
          subtitle={`Saturación: ${satPct.toFixed(0)}%`}
          barPct={satPct} barColor={colors.cyan}
        />
      </View>

      <View style={styles.row}>
        <SensorCard
          label="Humedad Ambiente" value={data.humedad?.toFixed(1) ?? '--'} unit="%"
          icon="💧" accent={colors.blue}
          subtitle={`Evaporación ${humLabel.toLowerCase()}`} subtitleColor={humColor}
          barPct={data.humedad} barColor={humColor}
        />
        <SensorCard
          label="Presión Atm." value={data.presion?.toFixed(0) ?? '--'} unit="hPa"
          icon="📊" accent={colors.purple}
          subtitle={`Alt: ${Math.round(data.altitud)}m`}
        />
      </View>

      {/* Derived Row */}
      <View style={styles.derivedRow}>
        <DerivedMetric icon="🌡️" value={`${data.temperatura?.toFixed(1)}°`} label="Salmuera" color={colors.orange} />
        <DerivedMetric icon="☀️" value={`${data.sens_termica?.toFixed(1)}°`} label="Evap." color={colors.amber} />
        <DerivedMetric icon="🏔️" value={`${Math.round(data.altitud)}m`} label="Altitud" color={colors.green} />
        <DerivedMetric icon="🔋" value={`${data.bateria_mV}`} label="Batería" color={data.bateria_mV < 3300 ? colors.amber : colors.green} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { gap: 12 },
  poolBox: {
    borderRadius: 14,
    borderWidth: 1,
    borderColor: colors.cyan,
    overflow: 'hidden',
    padding: 16,
    alignItems: 'center',
    backgroundColor: '#0c2d4a',
  },
  poolSun: { position: 'absolute', top: 8, right: 14, fontSize: 22 },
  poolTemp: {
    fontSize: 32,
    fontWeight: '700',
    color: '#fff',
    fontVariant: ['tabular-nums'],
    textShadowColor: 'rgba(6,182,212,0.5)',
    textShadowOffset: { width: 0, height: 0 },
    textShadowRadius: 12,
  },
  poolWaves: { marginVertical: 6, opacity: 0.4 },
  waveText: { fontSize: 14, color: colors.cyan, letterSpacing: 2 },
  poolLabel: {
    fontSize: 9,
    color: 'rgba(6,182,212,0.6)',
    letterSpacing: 2,
    fontVariant: ['tabular-nums'],
  },
  row: { flexDirection: 'row', gap: 10 },
  derivedRow: { flexDirection: 'row', gap: 6 },
});
