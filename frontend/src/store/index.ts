import { create } from 'zustand';
import type {
  LightingState,
  FixtureProfile,
  Setup,
} from '@/types';

interface AppStore {
  // Auth
  isAuthenticated: boolean;
  setAuthenticated: (value: boolean) => void;

  // Catalog
  profiles: FixtureProfile[];
  setProfiles: (profiles: FixtureProfile[]) => void;

  // Setups
  setupsList: {id: string, name: string, active?: boolean}[];
  setSetupsList: (list: {id: string, name: string, active?: boolean}[]) => void;

  activeSetup: Setup | null;
  setActiveSetup: (setup: Setup | null) => void;

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

  profiles: [],
  setProfiles: (profiles) => set({ profiles }),

  setupsList: [],
  setSetupsList: (setupsList) => set({ setupsList }),

  activeSetup: null,
  setActiveSetup: (activeSetup) => set({ activeSetup }),

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
