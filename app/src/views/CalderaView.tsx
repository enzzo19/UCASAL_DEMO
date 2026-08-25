import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../theme';
import { SensorData } from '../types';
import DerivedMetric from '../components/DerivedMetric';
import { boilerZone, steamEfficiency } from '../utils/calculations';

export default function CalderaView({ data }: { data: SensorData }) {
  const zone = boilerZone(data.temperatura, data.presion);
  const eff = steamEfficiency(data.temperatura, data.presion, data.humedad);
  const tempPct = Math.min(100, Math.max(0, (data.temperatura / 60) * 100));

  return (
    <View style={styles.container}>
      {/* Hazard Stripe */}
      <View style={styles.hazardStripe} />

      {/* Zone Banner */}
      <View style={[styles.zoneBanner, { borderColor: zone.color }]}>
        <Text style={styles.zoneLabel}>ZONA OPERATIVA</Text>
        <Text style={[styles.zoneValue, { color: zone.color }]}>{zone.zone}</Text>
        <Text style={styles.zoneEff}>
          Eficiencia de vapor: <Text style={{ color: parseFloat(eff) > 80 ? colors.green : colors.amber, fontWeight: '700' }}>{eff}%</Text>
        </Text>
      </View>

      {/* Thermometer + Pressure + Chimney */}
      <View style={styles.row}>
        {/* Temp Panel */}
        <View style={styles.panel}>
          <Text style={styles.panelTitle}>♨️ TEMP. CÁMARA</Text>
          <View style={styles.thermoContainer}>
            <View style={styles.thermoTrack}>
              <View style={[styles.thermoFill, {
                height: `${tempPct}%`,
                backgroundColor: data.temperatura > 45 ? colors.red : data.temperatura > 35 ? colors.orange : colors.amber,
              }]} />
            </View>
            <View style={[styles.thermoBulb, {
              backgroundColor: data.temperatura > 45 ? colors.red : data.temperatura > 35 ? colors.orange : colors.amber,
            }]} />
          </View>
          <Text style={[styles.thermoValue, {
            color: data.temperatura > 45 ? colors.red : colors.orange,
          }]}>{data.temperatura?.toFixed(1)}°C</Text>
        </View>

        {/* Pressure Panel */}
        <View style={styles.panel}>
          <Text style={styles.panelTitle}>⚙️ PRESIÓN VAPOR</Text>
          <View style={styles.gaugeContainer}>
            <Text style={[styles.gaugeValue, {
              color: data.presion > 930 ? colors.red : data.presion > 910 ? colors.amber : colors.green,
            }]}>{data.presion?.toFixed(0)}</Text>
            <Text style={styles.gaugeUnit}>hPa</Text>
            {/* Simple bar gauge for mobile */}
            <View style={styles.gaugeBar}>
              <View style={[styles.gaugeBarFill, {
                width: `${Math.min(100, ((data.presion - 850) / 100) * 100)}%`,
                backgroundColor: data.presion > 930 ? colors.red : data.presion > 910 ? colors.amber : colors.green,
              }]} />
            </View>
            <View style={styles.gaugeLabels}>
              <Text style={styles.gaugeLabelText}>850</Text>
              <Text style={styles.gaugeLabelText}>950</Text>
            </View>
          </View>
        </View>

        {/* Chimney Panel */}
        <View style={styles.panel}>
          <Text style={styles.panelTitle}>🔥 CHIMENEA</Text>
          <Text style={styles.chimneyValue}>{data.sens_termica?.toFixed(1)}</Text>
          <Text style={styles.chimneyUnit}>°C</Text>
          <View style={styles.fireRow}>
            <Text style={styles.flame}>🔥</Text>
            <Text style={styles.flame}>🔥</Text>
            <Text style={styles.flame}>🔥</Text>
          </View>
        </View>
      </View>

      <View style={styles.hazardStripe} />

      {/* Derived Row */}
      <View style={styles.derivedRow}>
        <DerivedMetric icon="🎯" value={zone.zone} label="Zona" color={zone.color} />
        <DerivedMetric icon="⚡" value={`${eff}%`} label="Eficiencia" color={parseFloat(eff) > 80 ? colors.green : colors.amber} />
        <DerivedMetric icon="💨" value={`${data.humedad?.toFixed(0)}%`} label="Hum. Sala" color={colors.blue} />
        <DerivedMetric icon="🔋" value={`${data.bateria_mV}`} label="Batería" color={data.bateria_mV < 3300 ? colors.amber : colors.green} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { gap: 10 },
  hazardStripe: {
    height: 4,
    backgroundColor: colors.amber,
    borderRadius: 2,
    opacity: 0.7,
  },
  zoneBanner: {
    backgroundColor: colors.bgCard,
    borderRadius: 12,
    borderWidth: 2,
    padding: 14,
    alignItems: 'center',
  },
  zoneLabel: { fontSize: 10, color: colors.textMuted, letterSpacing: 2, textTransform: 'uppercase' },
  zoneValue: { fontSize: 28, fontWeight: '800', fontVariant: ['tabular-nums'], marginVertical: 2 },
  zoneEff: { fontSize: 12, color: colors.textSecondary },
  row: { flexDirection: 'row', gap: 8 },
  panel: {
    flex: 1,
    backgroundColor: colors.bgCard,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 10,
    alignItems: 'center',
  },
  panelTitle: { fontSize: 8, fontWeight: '600', color: colors.textMuted, textTransform: 'uppercase', letterSpacing: 1, marginBottom: 8 },
  thermoContainer: { alignItems: 'center', height: 90, justifyContent: 'flex-end' },
  thermoTrack: {
    width: 14,
    height: 70,
    backgroundColor: colors.border,
    borderRadius: 7,
    overflow: 'hidden',
    justifyContent: 'flex-end',
  },
  thermoFill: { width: '100%', borderRadius: 0 },
  thermoBulb: { width: 24, height: 24, borderRadius: 12, marginTop: -4, borderWidth: 2, borderColor: colors.border },
  thermoValue: { fontSize: 16, fontWeight: '700', fontVariant: ['tabular-nums'], marginTop: 4 },
  gaugeContainer: { alignItems: 'center', paddingVertical: 8 },
  gaugeValue: { fontSize: 24, fontWeight: '700', fontVariant: ['tabular-nums'] },
  gaugeUnit: { fontSize: 10, color: colors.textMuted, marginBottom: 8 },
  gaugeBar: { width: '100%', height: 8, backgroundColor: colors.border, borderRadius: 4, overflow: 'hidden' },
  gaugeBarFill: { height: '100%', borderRadius: 4 },
  gaugeLabels: { flexDirection: 'row', justifyContent: 'space-between', width: '100%', marginTop: 4 },
  gaugeLabelText: { fontSize: 8, color: colors.textMuted, fontVariant: ['tabular-nums'] },
  chimneyValue: { fontSize: 22, fontWeight: '700', color: colors.amber, fontVariant: ['tabular-nums'], marginTop: 10 },
  chimneyUnit: { fontSize: 10, color: colors.textMuted },
  fireRow: { flexDirection: 'row', marginTop: 8, gap: 2 },
  flame: { fontSize: 16 },
  derivedRow: { flexDirection: 'row', gap: 6 },
});
