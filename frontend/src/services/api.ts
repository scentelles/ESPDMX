import axios, { AxiosInstance } from 'axios';
import type {
  ColorScene,
  DynamicShow,
  FixtureProfile,
  FixtureInstance,
  VirtualGroup,
  Setup,
  SystemConfig,
  LightingState,
} from '@/types';

class APIService {
  private api: AxiosInstance;
  private ws: WebSocket | null = null;
  private wsReady: boolean = false;
  private dmxPending: Map<number, number> = new Map();
  private groupPending: Map<string, number> = new Map();
  private dmxTimer: ReturnType<typeof setTimeout> | null = null;

  constructor() {
    this.api = axios.create({
      baseURL: '/api',
      timeout: 10000,
      headers: {
        'Content-Type': 'application/json',
      },
    });
    this.connectWebSocket();
  }

  private connectWebSocket(): void {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${proto}//${location.host}/ws`;
    try {
      this.ws = new WebSocket(wsUrl);
      this.ws.onopen = () => { this.wsReady = true; };
      this.ws.onclose = () => {
        this.wsReady = false;
        this.ws = null;
        setTimeout(() => this.connectWebSocket(), 2000);
      };
      this.ws.onerror = () => {
        this.ws?.close();
      };
    } catch {
      setTimeout(() => this.connectWebSocket(), 2000);
    }
  }

  // Profiles (Catalog)
  async getProfiles(): Promise<FixtureProfile[]> {
    try {
      const response = await this.api.get('/profiles');
      return response.data.profiles || [];
    } catch { return []; }
  }

  async saveProfile(profile: FixtureProfile): Promise<void> {
    await this.api.post('/profiles', profile);
  }

  async deleteProfile(id: string): Promise<void> {
    await this.api.delete(`/profiles/${encodeURIComponent(id)}`);
  }

  // Setups (Configurations)
  async getSetupsList(): Promise<{id: string, name: string, active?: boolean}[]> {
    try {
      const response = await this.api.get('/setups');
      return response.data.setups || [];
    } catch { return []; }
  }

  async getActiveSetup(): Promise<Setup | null> {
    try {
      const response = await this.api.get('/setups/active');
      return response.data.setup || null;
    } catch { return null; }
  }

  async createSetup(id: string, name: string): Promise<void> {
    await this.api.post('/setups', { id, name });
  }

  async deleteSetup(id: string): Promise<void> {
    await this.api.delete(`/setups/${encodeURIComponent(id)}`);
  }

  async activateSetup(setupId: string): Promise<void> {
    await this.api.post('/control/setup', { setupId });
  }

  // Modifiers on Active Setup
  async saveInstance(instance: FixtureInstance): Promise<void> {
    await this.api.post('/instances', instance);
  }
  async deleteInstance(id: string): Promise<void> {
    await this.api.delete(`/instances/${encodeURIComponent(id)}`);
  }

  async saveVirtualGroup(vg: VirtualGroup): Promise<void> {
    await this.api.post('/virtual-groups', vg);
  }
  async deleteVirtualGroup(id: string): Promise<void> {
    await this.api.delete(`/virtual-groups/${encodeURIComponent(id)}`);
  }

  async saveScene(scene: ColorScene): Promise<void> {
    await this.api.post('/scenes', scene);
  }
  async deleteScene(id: string): Promise<void> {
    await this.api.delete(`/scenes/${encodeURIComponent(id)}`);
  }

  async saveShow(show: DynamicShow): Promise<void> {
    await this.api.post('/shows', show);
  }
  async deleteShow(id: string): Promise<void> {
    await this.api.delete(`/shows/${encodeURIComponent(id)}`);
  }

  // Control
  async activateScene(sceneId: string): Promise<void> {
    await this.api.post('/control/scene', { sceneId });
  }

  async startShow(showId: string): Promise<void> {
    await this.api.post('/control/show', { showId });
  }

  async stopShow(): Promise<void> {
    await this.api.post('/control/show-stop');
  }

  async triggerStrobe(speed: number): Promise<void> {
    await this.api.post('/control/strobe', { speed });
  }

  async stopStrobe(): Promise<void> {
    await this.api.post('/control/strobe-stop');
  }

  async triggerSmoke(duration: number): Promise<void> {
    await this.api.post('/control/smoke', { duration });
  }

  async stopSmoke(): Promise<void> {
    await this.api.post('/control/smoke-stop');
  }

  async setMasterBrightness(brightness: number): Promise<void> {
    await this.api.post('/control/brightness', { brightness });
  }

  async setSoundMode(mode: number, sensitivity?: number): Promise<void> {
    const payload: { mode: number; sensitivity?: number } = { mode };
    if (sensitivity !== undefined) payload.sensitivity = sensitivity;
    await this.api.post('/control/sound', payload);
  }

  async stopSound(): Promise<void> {
    await this.api.post('/control/sound-stop');
  }

  // DMX and Virtual Group realtime control
  async setDMXChannel(channel: number, value: number): Promise<void> {
    this.dmxPending.set(channel, value);
    if (!this.dmxTimer) {
      this.dmxTimer = setTimeout(() => this.flushDMX(), 50);
    }
  }

  async setVirtualGroupValue(groupId: string, value: number): Promise<void> {
    this.groupPending.set(groupId, value);
    if (!this.dmxTimer) {
      this.dmxTimer = setTimeout(() => this.flushDMX(), 50);
    }
  }

  private flushDMX(): void {
    this.dmxTimer = null;
    if (this.dmxPending.size === 0 && this.groupPending.size === 0) return;

    if (this.wsReady && this.ws) {
      const batch: Record<string, number> = {};
      for (const [ch, val] of this.dmxPending) batch[String(ch)] = val;
      
      if (Object.keys(batch).length > 0) this.ws.send(JSON.stringify({ dmx: batch }));

      for (const [grp, val] of this.groupPending) {
         this.ws.send(JSON.stringify({ group: grp, val }));
      }
    } else {
      // Fallback
      if (this.dmxPending.size > 0) {
        for (const [ch, val] of this.dmxPending) {
           this.api.post('/control/dmx', { channel: ch, value: val }).catch(() => {});
        }
      }
      if (this.groupPending.size > 0) {
        for (const [grp, val] of this.groupPending) {
           this.api.post('/control/group', { groupId: grp, value: val }).catch(() => {});
        }
      }
    }
    
    this.dmxPending.clear();
    this.groupPending.clear();
  }

  // Audio
  async getAudio(): Promise<any> {
    const response = await this.api.get('/audio');
    return response.data;
  }

  // System
  async getLightingState(): Promise<LightingState> {
    const response = await this.api.get('/state');
    return response.data;
  }

  async getSystemConfig(): Promise<SystemConfig> {
    try {
      const response = await this.api.get('/config');
      return response.data;
    } catch {
      return {
        wifiSSID: '',
        wifiPassword: '',
        adminPin: '',
        dmxBaud: 250000,
        maxFixtures: 10,
        updateInterval: 500,
      };
    }
  }

  async saveSystemConfig(config: Partial<SystemConfig>): Promise<SystemConfig> {
    const response = await this.api.post('/config', config);
    return response.data;
  }

  async reboot(): Promise<void> {
    await this.api.post('/system/reboot');
  }

  async getStatus(): Promise<{ uptime: number; freeMemory: number }> {
    const response = await this.api.get('/system/status');
    return response.data;
  }

  async uploadOTA(file: File, type: 'firmware' | 'spiffs', onProgress: (pct: number) => void): Promise<void> {
    const formData = new FormData();
    formData.append('update', file);
    await this.api.post(`/system/update?type=${type}`, formData, {
      headers: {
        'Content-Type': 'multipart/form-data',
      },
      onUploadProgress: (progressEvent) => {
        if (progressEvent.total) {
          const pct = Math.round((progressEvent.loaded * 100) / progressEvent.total);
          onProgress(pct);
        }
      },
      timeout: 120000, // Important: OTA can take a long time
    });
  }
}

export const apiService = new APIService();
