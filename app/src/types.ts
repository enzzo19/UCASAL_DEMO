export interface SensorData {
  temperatura: number;
  humedad: number;
  presion: number;
  altitud: number;
  sens_termica: number;
  bateria_mV: number;
  rssi_lora: number;
  snr_lora: number;
  paquetes_rx: number;
  paquetes_perdidos: number;
  timestamp: number;
  fecha_hora: string;
  node_id?: number;
}

export type ViewType = 'general' | 'caldera' | 'litio';

export interface ViewConfig {
  name: string;
  icon: string;
  accent: string;
  accentLight: string;
}

export const VIEW_CONFIGS: Record<ViewType, ViewConfig> = {
  general: {
    name: 'Ambiental',
    icon: '🌍',
    accent: '#f59e0b',
    accentLight: 'rgba(245, 158, 11, 0.15)',
  },
  caldera: {
    name: 'Caldera',
    icon: '🏭',
    accent: '#ef4444',
    accentLight: 'rgba(239, 68, 68, 0.15)',
  },
  litio: {
    name: 'Litio',
    icon: '⚗️',
    accent: '#06b6d4',
    accentLight: 'rgba(6, 182, 212, 0.15)',
  },
};
