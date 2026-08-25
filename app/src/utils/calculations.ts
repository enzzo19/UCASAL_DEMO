// Derived industrial calculations (same logic as web dashboard)

export function dewPoint(t: number, rh: number): number {
  const a = 17.27, b = 237.7;
  const alpha = (a * t) / (b + t) + Math.log(rh / 100);
  return (b * alpha) / (a - alpha);
}

export function comfortIndex(t: number, rh: number) {
  if (t >= 20 && t <= 26 && rh >= 30 && rh <= 60)
    return { level: 'Confortable', color: '#10b981', pct: 90 };
  if (t >= 17 && t <= 30 && rh >= 20 && rh <= 70)
    return { level: 'Aceptable', color: '#f59e0b', pct: 60 };
  return { level: 'Incómodo', color: '#ef4444', pct: 30 };
}

export function weatherCondition(t: number, rh: number) {
  if (rh > 85 && t > 20) return { emoji: '🌧️', text: 'Húmedo' };
  if (rh > 85) return { emoji: '🌫️', text: 'Niebla' };
  if (t > 35) return { emoji: '🔥', text: 'Calor Extremo' };
  if (t > 28) return { emoji: '☀️', text: 'Caluroso' };
  if (t > 20) return { emoji: '🌤️', text: 'Templado' };
  if (t > 10) return { emoji: '🌥️', text: 'Fresco' };
  if (t > 0) return { emoji: '❄️', text: 'Frío' };
  return { emoji: '🥶', text: 'Helada' };
}

export function evapRate(t: number, rh: number, p: number, alt: number): number {
  const es = 6.108 * Math.exp((17.27 * t) / (t + 237.3));
  const ea = es * (rh / 100);
  const vpd = es - ea;
  const altFactor = 1 + (alt / 5000) * 0.15;
  return Math.max(0, (vpd * 0.45 * altFactor) / (p / 1013.25));
}

export function brineConcentration(t: number, rh: number): string {
  const base = 1.8;
  const tempBoost = Math.max(0, (t - 15) * 0.02);
  const humBoost = Math.max(0, (60 - rh) * 0.01);
  return (base + tempBoost + humBoost).toFixed(2);
}

export function boilerZone(t: number, p: number) {
  if (t > 50 || p > 940)
    return { zone: 'PELIGRO', color: '#ef4444' };
  if (t > 38 || p > 910)
    return { zone: 'PRECAUCIÓN', color: '#f59e0b' };
  return { zone: 'NORMAL', color: '#10b981' };
}

export function steamEfficiency(t: number, p: number, rh: number): string {
  const base = 85;
  const tempP = t > 40 ? (t - 40) * 0.5 : 0;
  const presP = p > 920 ? (p - 920) * 0.1 : 0;
  const humP = rh > 70 ? (rh - 70) * 0.2 : 0;
  return Math.max(50, Math.min(99, base - tempP - presP - humP)).toFixed(1);
}
