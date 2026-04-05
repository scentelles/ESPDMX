import axios, { AxiosInstance } from 'axios';
import type {
  ColorScene,
  Ambiance,
  DynamicShow,
  DMXFixture,
  SystemConfig,
  LightingState,
} from '@/types';

class APIService {
  private api: AxiosInstance;

  constructor() {
    this.api = axios.create({
      baseURL: '/api',
      timeout: 10000,
      headers: {
        'Content-Type': 'application/json',
      },
    });
  }

  // Scenes
  async getScenes(): Promise<ColorScene[]> {
    try {
      const response = await this.api.get('/scenes');
      return Array.isArray(response.data) ? response.data : response.data.scenes || [];
    } catch (error) {
      console.error('Failed to fetch scenes:', error);
      return [];
    }
  }

  async saveScene(scene: ColorScene): Promise<ColorScene> {
    const response = await this.api.post('/scenes', scene);
    return response.data;
  }

  async deleteScene(id: string): Promise<void> {
    await this.api.delete(`/scenes/${encodeURIComponent(id)}`);
  }

  // Ambiances (not yet implemented in firmware)
  async getAmbiances(): Promise<Ambiance[]> {
    return [];
  }

  async saveAmbiance(_ambiance: Ambiance): Promise<Ambiance> {
    throw new Error('Not implemented');
  }

  async deleteAmbiance(_id: string): Promise<void> {
    throw new Error('Not implemented');
  }

  // Shows
  async getShows(): Promise<DynamicShow[]> {
    try {
      const response = await this.api.get('/shows');
      return Array.isArray(response.data) ? response.data : response.data.shows || [];
    } catch (error) {
      console.error('Failed to fetch shows:', error);
      return [];
    }
  }

  async saveShow(show: DynamicShow): Promise<DynamicShow> {
    const response = await this.api.post('/shows', show);
    return response.data;
  }

  async deleteShow(id: string): Promise<void> {
    await this.api.delete(`/shows/${encodeURIComponent(id)}`);
  }

  // Fixtures
  async getFixtures(): Promise<DMXFixture[]> {
    try {
      const response = await this.api.get('/fixtures');
      return Array.isArray(response.data) ? response.data : response.data.fixtures || [];
    } catch (error) {
      console.error('Failed to fetch fixtures:', error);
      return [];
    }
  }

  async saveFixture(fixture: DMXFixture): Promise<DMXFixture> {
    const response = await this.api.post('/fixtures', fixture);
    return response.data;
  }

  async deleteFixture(id: string): Promise<void> {
    await this.api.delete(`/fixtures/${encodeURIComponent(id)}`);
  }

  // Control
  async setColor(fixtureId: string, color: [number, number, number], brightness: number): Promise<void> {
    await this.api.post('/control/color', { fixtureId, color, brightness });
  }

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

  async setDMXChannel(channel: number, value: number): Promise<void> {
    await this.api.post('/control/dmx', { channel, value });
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
    } catch (error) {
      console.error('Failed to fetch system config:', error);
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
}

export const apiService = new APIService();
