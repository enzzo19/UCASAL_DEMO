import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, ScrollView, TouchableOpacity, StyleSheet,
  SafeAreaView, ActivityIndicator, Animated,
} from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { database, ref, onValue, get } from './src/config/firebase';
import { SensorData, ViewType, VIEW_CONFIGS } from './src/types';
import { colors } from './src/theme';
import GeneralView from './src/views/GeneralView';
import CalderaView from './src/views/CalderaView';
import LitioView from './src/views/LitioView';

export default function App() {
  const [data, setData] = useState<SensorData | null>(null);
  const [connected, setConnected] = useState(false);
  const [currentView, setCurrentView] = useState<ViewType>('general');
  const [lastTimestamp, setLastTimestamp] = useState<number | null>(null);
  const fadeAnim = useRef(new Animated.Value(1)).current;
  const pulseAnim = useRef(new Animated.Value(1)).current;

  // Pulse animation for status dot
  useEffect(() => {
    const pulse = Animated.loop(
      Animated.sequence([
        Animated.timing(pulseAnim, { toValue: 0.3, duration: 1000, useNativeDriver: true }),
        Animated.timing(pulseAnim, { toValue: 1, duration: 1000, useNativeDriver: true }),
      ])
    );
    pulse.start();
    return () => pulse.stop();
  }, []);

  // Firebase listener
  useEffect(() => {
    const dataRef = ref(database, '/actual');

    const unsubscribe = onValue(dataRef, (snapshot) => {
      const val = snapshot.val();
      if (val && val.timestamp !== lastTimestamp) {
        setData(val);
        setLastTimestamp(val.timestamp);
        setConnected(true);
      }
    });

    // Polling fallback every 3s
    const interval = setInterval(async () => {
      try {
        const snapshot = await get(dataRef);
        const val = snapshot.val();
        if (val && val.timestamp !== lastTimestamp) {
          setData(val);
          setLastTimestamp(val.timestamp);
          setConnected(true);
        }
      } catch (e) {
        // Silent fail on polling
      }
    }, 3000);

    // Connection state
    const connRef = ref(database, '.info/connected');
    const connUnsub = onValue(connRef, (snap) => {
      setConnected(snap.val() === true);
    });

    return () => {
      unsubscribe();
      connUnsub();
      clearInterval(interval);
    };
  }, [lastTimestamp]);

  // View switch animation
  const switchView = (view: ViewType) => {
    if (view === currentView) return;
    Animated.timing(fadeAnim, { toValue: 0.2, duration: 150, useNativeDriver: true }).start(() => {
      setCurrentView(view);
      Animated.timing(fadeAnim, { toValue: 1, duration: 200, useNativeDriver: true }).start();
    });
  };

  const viewConfig = VIEW_CONFIGS[currentView];

  // Loading screen
  if (!data) {
    return (
      <SafeAreaView style={styles.loadingContainer}>
        <StatusBar style="light" />
        <ActivityIndicator size="large" color={colors.amber} />
        <Text style={styles.loadingText}>Conectando a Firebase...</Text>
        <Text style={styles.loadingSubtext}>IITA IoT Monitor</Text>
      </SafeAreaView>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar style="light" />
      <ScrollView style={styles.scroll} showsVerticalScrollIndicator={false}>

        {/* ═══ HEADER ═══ */}
        <View style={styles.header}>
          <View>
            <Text style={styles.logoText}>IITA IoT</Text>
            <Text style={styles.logoSub}>MONITOREO INDUSTRIAL</Text>
          </View>
          <View style={[styles.statusBadge, connected ? styles.badgeOnline : styles.badgeOffline]}>
            <Animated.View style={[
              styles.statusDot,
              { backgroundColor: connected ? colors.green : colors.red, opacity: pulseAnim },
            ]} />
            <Text style={{ color: connected ? colors.green : colors.red, fontSize: 12, fontWeight: '600' }}>
              {connected ? 'En Vivo' : 'Offline'}
            </Text>
          </View>
        </View>

        {/* ═══ VIEW SELECTOR ═══ */}
        <View style={styles.viewSelector}>
          {(Object.keys(VIEW_CONFIGS) as ViewType[]).map((key) => {
            const vc = VIEW_CONFIGS[key];
            const active = key === currentView;
            return (
              <TouchableOpacity
                key={key}
                style={[
                  styles.viewTab,
                  active && { borderColor: vc.accent, backgroundColor: vc.accentLight },
                ]}
                onPress={() => switchView(key)}
                activeOpacity={0.7}
              >
                <Text style={styles.viewTabIcon}>{vc.icon}</Text>
                <Text style={[styles.viewTabName, active && { color: vc.accent }]}>{vc.name}</Text>
              </TouchableOpacity>
            );
          })}
        </View>

        {/* ═══ INFO BAR ═══ */}
        <View style={styles.infoBar}>
          <Text style={styles.timestamp}>{data.fecha_hora || '--'}</Text>
          <View style={[styles.viewBadge, { backgroundColor: `${viewConfig.accent}20`, borderColor: `${viewConfig.accent}40` }]}>
            <Text style={[styles.viewBadgeText, { color: viewConfig.accent }]}>
              {viewConfig.icon} {viewConfig.name.toUpperCase()}
            </Text>
          </View>
        </View>

        {/* ═══ VIEW CONTENT ═══ */}
        <Animated.View style={{ opacity: fadeAnim }}>
          {currentView === 'general' && <GeneralView data={data} />}
          {currentView === 'caldera' && <CalderaView data={data} />}
          {currentView === 'litio' && <LitioView data={data} />}
        </Animated.View>

        {/* ═══ SIGNAL PANEL ═══ */}
        <View style={styles.signalPanel}>
          <Text style={styles.sectionTitle}>📡 SEÑAL LORA</Text>
          <View style={styles.signalRow}>
            <Text style={styles.signalLabel}>RSSI</Text>
            <View style={styles.rssiContainer}>
              {[1,2,3,4,5].map(i => {
                const rssi = data.rssi_lora || -120;
                let level = 0;
                if (rssi > -60) level = 5;
                else if (rssi > -70) level = 4;
                else if (rssi > -85) level = 3;
                else if (rssi > -100) level = 2;
                else if (rssi > -115) level = 1;
                return (
                  <View key={i} style={[
                    styles.rssiBar,
                    { height: 4 + i * 4 },
                    i <= level && { backgroundColor: colors.green },
                  ]} />
                );
              })}
              <Text style={styles.signalValue}>{data.rssi_lora ?? '--'} dBm</Text>
            </View>
          </View>
          <View style={styles.signalRow}>
            <Text style={styles.signalLabel}>SNR</Text>
            <Text style={styles.signalValue}>{data.snr_lora?.toFixed(1) ?? '--'} dB</Text>
          </View>
          <View style={styles.signalRow}>
            <Text style={styles.signalLabel}>Paquetes RX</Text>
            <Text style={styles.signalValue}>{data.paquetes_rx ?? '--'}</Text>
          </View>
          <View style={[styles.signalRow, { borderBottomWidth: 0 }]}>
            <Text style={styles.signalLabel}>Perdidos</Text>
            <Text style={styles.signalValue}>{data.paquetes_perdidos ?? '--'}</Text>
          </View>
        </View>

        {/* ═══ NODE INFO ═══ */}
        <View style={styles.nodeInfo}>
          <Text style={styles.nodeText}>
            Node {data.node_id || 1} · Seq {data.paquetes_rx} · UCASAL 2026
          </Text>
        </View>

        <View style={{ height: 30 }} />
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.bgPrimary,
  },
  scroll: {
    flex: 1,
    paddingHorizontal: 16,
  },
  loadingContainer: {
    flex: 1,
    backgroundColor: colors.bgPrimary,
    justifyContent: 'center',
    alignItems: 'center',
    gap: 16,
  },
  loadingText: { color: colors.textSecondary, fontSize: 16 },
  loadingSubtext: { color: colors.textMuted, fontSize: 12, letterSpacing: 2, textTransform: 'uppercase' },

  // Header
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 16,
    paddingTop: 24,
  },
  logoText: {
    fontSize: 24,
    fontWeight: '800',
    color: colors.amber,
  },
  logoSub: {
    fontSize: 9,
    color: colors.textMuted,
    letterSpacing: 2,
  },
  statusBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 20,
    borderWidth: 1,
  },
  badgeOnline: {
    backgroundColor: 'rgba(16,185,129,0.1)',
    borderColor: 'rgba(16,185,129,0.3)',
  },
  badgeOffline: {
    backgroundColor: 'rgba(239,68,68,0.1)',
    borderColor: 'rgba(239,68,68,0.3)',
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },

  // View Selector
  viewSelector: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 12,
  },
  viewTab: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    paddingVertical: 10,
    borderRadius: 10,
    borderWidth: 1.5,
    borderColor: colors.border,
    backgroundColor: colors.bgCard,
  },
  viewTabIcon: { fontSize: 16 },
  viewTabName: { fontSize: 11, fontWeight: '700', color: colors.textMuted },

  // Info Bar
  infoBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  timestamp: {
    fontSize: 11,
    color: colors.textSecondary,
    fontVariant: ['tabular-nums'],
  },
  viewBadge: {
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 6,
    borderWidth: 1,
  },
  viewBadgeText: {
    fontSize: 10,
    fontWeight: '700',
    letterSpacing: 1,
  },

  // Signal Panel
  signalPanel: {
    backgroundColor: colors.bgCard,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: colors.border,
    padding: 14,
    marginTop: 12,
  },
  sectionTitle: {
    fontSize: 11,
    fontWeight: '600',
    color: colors.textSecondary,
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginBottom: 10,
  },
  signalRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: 'rgba(30,41,64,0.5)',
  },
  signalLabel: { fontSize: 12, color: colors.textMuted },
  signalValue: { fontSize: 13, color: colors.textPrimary, fontVariant: ['tabular-nums'], fontWeight: '500' },
  rssiContainer: { flexDirection: 'row', alignItems: 'flex-end', gap: 3 },
  rssiBar: {
    width: 5,
    borderRadius: 2,
    backgroundColor: colors.border,
  },

  // Node Info
  nodeInfo: {
    alignItems: 'center',
    paddingVertical: 12,
    marginTop: 8,
  },
  nodeText: {
    fontSize: 11,
    color: colors.textMuted,
    letterSpacing: 1,
  },
});
