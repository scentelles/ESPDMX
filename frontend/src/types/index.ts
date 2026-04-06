export type RGB = [number, number, number];

export type ChannelType = 'dimmer' | 'color' | 'position' | 'speed' | 'gobo' | 'other';

export interface ChannelDefinition {
  name: string;         // e.g., "Red", "Green", "Blue", "Dimmer", "Pan", "Tilt"
  offset: number;       // Channel offset from dmxAddress (0-based)
  defaultValue: number; // Default value (0-255)
  type: ChannelType;
}

export interface StrobeChannelConfig {
  enabled: boolean;
  offset: number;       // DMX offset from fixture base address
  value: number;        // Value to send when strobe is triggered (0-255)
}

export interface DMXFixture {
  id: string;
  name: string;
  type: string;
  dmxAddress: number;
  channelCount: number;
  channels: ChannelDefinition[];
  enabled: boolean;
  strobeChannel?: StrobeChannelConfig;
}

export interface FixtureChannelValue {
  fixtureId: string;
  values: Record<string, number>; // channel name -> value (0-255)
}

export interface ColorScene {
  id: string;
  name: string;
  description: string;
  icon: string;
  fixtureValues: FixtureChannelValue[];
}

export interface Ambiance {
  id: string;
  name: string;
  description: string;
  icon: string;
  scenes: string[]; // array of scene IDs
}

export interface ShowStep {
  sceneId: string;
  duration: number;       // ms to hold this scene
  transitionTime: number; // ms to fade/transition into this scene
  smoothTransition: boolean; // progressive DMX value interpolation
}

export interface DynamicShow {
  id: string;
  name: string;
  description: string;
  icon: string;
  loop: boolean;
  steps: ShowStep[];
  isRunning?: boolean;
}

export interface SystemConfig {
  wifiSSID: string;
  wifiPassword: string;
  adminPin: string;
  dmxBaud: number;
  maxFixtures: number;
  updateInterval: number;
}

export interface LightingState {
  activeScene?: string;
  activeAmbiance?: string;
  activeShow?: string;
  strobeActive: boolean;
  smokeActive: boolean;
  masterBrightness: number;
  dmxOutput: number[];
  soundMode?: number;
  soundSensitivity?: number;
  audio?: {
    volume: number;
    bass: number;
    mid: number;
    high: number;
    beat: boolean;
  };
}
