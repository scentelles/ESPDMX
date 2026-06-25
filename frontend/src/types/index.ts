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

export interface FixtureEffect {
  type: string;
  speed: number;
  colorHex: string;
  reverse?: boolean;
}

export interface FixtureChannelValue {
  fixtureId: string;              // refers to instanceId
  values: Record<string, number>; // channel name -> value (0-255)
  effect?: FixtureEffect;
}

export interface VirtualGroupSceneValue {
  groupId: string;
  colorHex?: string;
  dimmer?: number;
}

export interface ColorScene {
  id: string;
  name: string;
  description: string;
  icon: string;
  groupId?: number; // 1 or 2
  fixtureValues: FixtureChannelValue[];
  virtualGroupValues?: VirtualGroupSceneValue[];
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

export type PedalAction = 'none' | 'smoke' | 'strobe' | 'scene' | 'show' | 'scene_sequence' | 'scene_sequence_g1' | 'scene_sequence_g2' | 'show_sequence' | 'sound_volume' | 'sound_beat' | 'sound_color' | 'sound_vu';

export interface PedalButtonConfig {
  action: PedalAction;
  targetId: string;
}

export interface Setup {
  id: string;
  name: string;
  instances: FixtureInstance[];
  virtualGroups: VirtualGroup[];
  scenes: ColorScene[];
  shows: DynamicShow[];
  pedalConfig?: PedalButtonConfig[];
}

export interface BlePedalActionDef {
  ccId: number;
  action: string;
  param: string;
}

export interface SystemConfig {
  wifiSSID: string;
  wifiPassword: string;
  adminPin: string;
  dmxBaud: number;
  maxFixtures: number;
  updateInterval: number;
  soundSensitivity: number;
  soundDynamics: number;
  activeSetupId?: string;
}

export interface LightingState {
  activeSetupId?: string;
  activeScene?: string;
  activeSceneG2?: string;
  activeShow?: string;
  strobeActive: boolean;
  smokeActive: boolean;
  masterBrightness: number;
  dmxOutput: number[];
  soundMode?: number;
  soundSensitivity?: number;
  soundDynamics?: number;
  audio?: {
    volume: number;
    bass: number;
    mid: number;
    high: number;
    beat: boolean;
  };
  pedalConnected?: boolean;
}
