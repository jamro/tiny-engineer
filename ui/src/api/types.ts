export interface ApiError {
  ok: false;
  error: string;
}

export type LinkMode = "" | "wifi" | "usb";

export interface AuthInfo {
  ok: true;
  version: string;
  required: boolean;
  wifi_configured: boolean;
  link_mode: LinkMode;
  provisioning: boolean;
}

export interface Health {
  ok: true;
  version: string;
  uptime_ms: number;
  free_heap: number;
  heap_size: number;
  cpu_temp_c: number | null;
  wifi: { connected: boolean; ip: string; rssi: number; hostname: string };
  wifi_configured: boolean;
  link_mode: LinkMode;
  provisioning: boolean;
}

export type LoadingMode = "progress" | "sleep_inertia";

export interface Settings {
  ok: true;
  sleep_timeout: number;
  hostname: string;
  volume: number;
  welcome: boolean;
  serial_log: boolean;
  continuous_timeout: number;
  loading: LoadingMode;
  eyes_style: string;
  access_token_set: boolean;
  link_mode: LinkMode;
  wifi_configured: boolean;
  servo_mins: number[];
  servo_maxs: number[];
  rgb_order: string;
  oled_rotate_180: boolean;
  wifi_networks: string[];
  reboot_required?: boolean;
  wifi_connect_success?: boolean;
  wifi_ip?: string;
  wifi_hostname?: string;
}

export interface AnimationReply {
  ok: true;
  animation: string;
}

export interface WifiNetwork {
  ssid: string;
  password?: string;
}
