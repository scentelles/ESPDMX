import { create } from 'zustand';
import type {
  ColorScene,
  Ambiance,
  DynamicShow,
  DMXFixture,
  LightingState,
} from '@/types';

interface AppStore {
  // Auth
  isAuthenticated: boolean;
  setAuthenticated: (value: boolean) => void;

  // Data
  scenes: ColorScene[];
  setScenes: (scenes: ColorScene[]) => void;

  ambiances: Ambiance[];
  setAmbiances: (ambiances: Ambiance[]) => void;

  shows: DynamicShow[];
  setShows: (shows: DynamicShow[]) => void;

  fixtures: DMXFixture[];
  setFixtures: (fixtures: DMXFixture[]) => void;

  // State
  lightingState: LightingState;
  setLightingState: (state: LightingState) => void;

  // UI
  loading: boolean;
  setLoading: (loading: boolean) => void;

  error: string | null;
  setError: (error: string | null) => void;
}

export const useAppStore = create<AppStore>((set) => ({
  isAuthenticated: false,
  setAuthenticated: (value) => set({ isAuthenticated: value }),

  scenes: [],
  setScenes: (scenes) => set({ scenes }),

  ambiances: [],
  setAmbiances: (ambiances) => set({ ambiances }),

  shows: [],
  setShows: (shows) => set({ shows }),

  fixtures: [],
  setFixtures: (fixtures) => set({ fixtures }),

  lightingState: {
    masterBrightness: 100,
    dmxOutput: [],
    strobeActive: false,
    smokeActive: false,
  },
  setLightingState: (lightingState) => set({ lightingState }),

  loading: false,
  setLoading: (loading) => set({ loading }),

  error: null,
  setError: (error) => set({ error }),
}));
