import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../theme';

interface Props {
  label: string;
  value: string | number;
  unit: string;
  icon: string;
  accent: string;
  barColor?: string;
  barPct?: number;
  subtitle?: string;
  subtitleColor?: string;
}

export default function SensorCard({
  label, value, unit, icon, accent, barColor, barPct, subtitle, subtitleColor,
}: Props) {
  return (
    <View style={styles.card}>
      <View style={[styles.topBar, { backgroundColor: accent }]} />
      <View style={styles.header}>
        <Text style={styles.label}>{label}</Text>
        <Text style={styles.icon}>{icon}</Text>
      </View>
      <View style={styles.valueRow}>
        <Text style={[styles.value, { color: accent }]}>{value}</Text>
        <Text style={styles.unit}>{unit}</Text>
      </View>
      {barPct !== undefined && (
        <View style={styles.bar}>
          <View style={[styles.barFill, { width: `${Math.min(100, barPct)}%`, backgroundColor: barColor || accent }]} />
        </View>
      )}
      {subtitle && (
        <Text style={[styles.subtitle, subtitleColor ? { color: subtitleColor } : {}]}>{subtitle}</Text>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.bgCard,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 14,
    flex: 1,
    minWidth: 140,
  },
  topBar: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    height: 3,
    borderTopLeftRadius: 12,
    borderTopRightRadius: 12,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  label: {
    fontSize: 10,
    fontWeight: '600',
    color: colors.textMuted,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  icon: { fontSize: 16 },
  valueRow: {
    flexDirection: 'row',
    alignItems: 'baseline',
    gap: 4,
  },
  value: {
    fontSize: 26,
    fontWeight: '700',
    fontVariant: ['tabular-nums'],
  },
  unit: {
    fontSize: 13,
    color: colors.textSecondary,
  },
  bar: {
    height: 6,
    backgroundColor: colors.border,
    borderRadius: 3,
    marginTop: 8,
    overflow: 'hidden',
  },
  barFill: {
    height: '100%',
    borderRadius: 3,
  },
  subtitle: {
    fontSize: 11,
    color: colors.textMuted,
    marginTop: 6,
  },
});
