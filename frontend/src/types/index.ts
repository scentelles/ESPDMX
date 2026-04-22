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

export interface FixtureProfile {
  id: string;
  name: string;
  type: string;         // e.g., "rgb", "moving_head"
  channelCount: number;
  channels: ChannelDefinition[];
  strobeChannel?: StrobeChannelConfig;
}

export interface FixtureInstance {
  id: string;           // Unique instance ID
  profileId: string;    // References FixtureProfile.id
  name: string;         // E.g. "Front Wash Left"
  dmxAddress: number;
  enabled: boolean;
}

export interface VirtualGroupAssignment {
  instanceId: string;
  channelName: string;
}

export interface VirtualGroup {
  id: string;
  name: string;
  assignments: VirtualGroupAssignment[];
}

export interface FixtureChannelValue {
  fixtureId: string;              // refers to instanceId
  values: Record<string, number>; // channel name -> value (0-255)
}

export interface ColorScene {
  id: string;
  name: string;
  description: string;
  icon: string;
  fixtureValues: FixtureChannelValue[];
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

export interface Setup {
  id: string;
  name: string;
  instances: FixtureInstance[];
  virtualGroups: VirtualGroup[];
  scenes: ColorScene[];
  shows: DynamicShow[];
}

export interface SystemConfig {
  wifiSSID: string;
  wifiPassword: string;
  adminPin: string;
  dmxBaud: number;
  maxFixtures: number;
  updateInterval: number;
  activeSetupId?: string;
}

export interface LightingState {
  activeSetupId?: string;
  activeScene?: string;
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
