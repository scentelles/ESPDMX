import React from 'react';
import { Zap, Wind, Mic, MicOff } from 'lucide-react';

interface EffectButtonsProps {
  onStrobeDown: () => void;
  onStrobeUp: () => void;
  onSmokeDown: () => void;
  onSmokeUp: () => void;
  strobeActive?: boolean;
  smokeActive?: boolean;
}

export const EffectButtons: React.FC<EffectButtonsProps> = ({
  onStrobeDown,
  onStrobeUp,
  onSmokeDown,
  onSmokeUp,
  strobeActive = false,
  smokeActive = false,
}) => {
  return (
    <div className="grid grid-cols-2 gap-4">
      <button
        onPointerDown={onStrobeDown}
        onPointerUp={onStrobeUp}
        onPointerLeave={onStrobeUp}
        onContextMenu={(e) => e.preventDefault()}
        className={`flex items-center justify-center gap-2 select-none touch-none font-semibold rounded-lg px-6 py-4 text-lg transition-all duration-200 ${
          strobeActive
            ? 'bg-red-600 text-white ring-2 ring-red-400 shadow-lg shadow-red-500/40 animate-pulse-glow'
            : 'bg-slate-700 text-slate-300 hover:bg-slate-600 border border-slate-600'
        }`}
      >
        <Zap size={24} />
        <span>STROBOSCOPE</span>
      </button>

      <button
        onPointerDown={onSmokeDown}
        onPointerUp={onSmokeUp}
        onPointerLeave={onSmokeUp}
        onContextMenu={(e) => e.preventDefault()}
        className={`flex items-center justify-center gap-2 select-none touch-none font-semibold rounded-lg px-6 py-4 text-lg transition-all duration-200 ${
          smokeActive
            ? 'bg-red-600 text-white ring-2 ring-red-400 shadow-lg shadow-red-500/40 animate-pulse-glow'
            : 'bg-slate-700 text-slate-300 hover:bg-slate-600 border border-slate-600'
        }`}
      >
        <Wind size={24} />
        <span>FUMÉE</span>
      </button>

      <p className="col-span-2 text-center text-xs text-slate-500">Maintenir appuyé pour activer</p>
    </div>
  );
};

interface SceneGridProps {
  scenes: Array<{ id: string; name: string; description: string; icon: string }>;
  onSceneSelect: (sceneId: string) => void;
  activeSceneIds?: string[];
}

export const SceneGrid: React.FC<SceneGridProps> = ({ scenes, onSceneSelect, activeSceneIds = [] }) => {
  return (
    <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-3">
      {scenes.map((scene) => (
        <button
          key={scene.id}
          onClick={() => onSceneSelect(scene.id)}
          className={`p-4 rounded-lg border-2 transition-all duration-200 ${
            activeSceneIds.includes(scene.id)
              ? 'border-purple-600 bg-purple-600/20 shadow-lg shadow-purple-500/50'
              : 'border-slate-700 bg-slate-800 hover:border-purple-600'
          }`}
          title={scene.description}
        >
          <div className="text-3xl mb-2">{scene.icon}</div>
          <div className="text-sm font-semibold text-white">{scene.name}</div>
        </button>
      ))}
    </div>
  );
};

interface AmbianceGridProps {
  ambiances: Array<{ id: string; name: string; description: string; icon: string }>;
  onAmbianceSelect: (ambianceId: string) => void;
  activeAmbianceId?: string;
}

export const AmbianceGrid: React.FC<AmbianceGridProps> = ({
  ambiances,
  onAmbianceSelect,
  activeAmbianceId,
}) => {
  return (
    <div className="grid grid-cols-2 md:grid-cols-3 gap-3">
      {ambiances.map((ambiance) => (
        <button
          key={ambiance.id}
          onClick={() => onAmbianceSelect(ambiance.id)}
          className={`p-4 rounded-lg border-2 transition-all duration-200 ${
            activeAmbianceId === ambiance.id
              ? 'border-pink-600 bg-pink-600/20 shadow-lg shadow-pink-500/50'
              : 'border-slate-700 bg-slate-800 hover:border-pink-600'
          }`}
          title={ambiance.description}
        >
          <div className="text-3xl mb-2">{ambiance.icon}</div>
          <div className="text-sm font-semibold text-white">{ambiance.name}</div>
        </button>
      ))}
    </div>
  );
};

interface ShowGridProps {
  shows: Array<{ id: string; name: string; description: string; icon: string }>;
  onShowStart: (showId: string) => void;
  activeShowId?: string;
  isRunning?: boolean;
}

export const ShowGrid: React.FC<ShowGridProps> = ({
  shows,
  onShowStart,
  activeShowId,
  isRunning,
}) => {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
      {shows.map((show) => (
        <button
          key={show.id}
          onClick={() => onShowStart(show.id)}
          className={`p-4 rounded-lg border-2 transition-all duration-200 text-left ${
            activeShowId === show.id
              ? 'border-purple-600 bg-purple-600/20 shadow-lg shadow-purple-500/50'
              : 'border-slate-700 bg-slate-800 hover:border-purple-600'
          }`}
          title={show.description}
        >
          <div className="flex items-center gap-3">
            <div className="text-3xl">{show.icon}</div>
            <div className="flex-1">
              <div className="font-semibold text-white">{show.name}</div>
              <div className="text-xs text-slate-400">{show.description}</div>
            </div>
            {isRunning && activeShowId === show.id && (
              <div className="w-3 h-3 rounded-full bg-green-500 animate-pulse" />
            )}
          </div>
        </button>
      ))}
    </div>
  );
};

// ── Connection status indicator ─────────────────────────────────────
interface ConnectionIndicatorProps {
  connected: boolean;
  label?: string;
}

export const ConnectionIndicator: React.FC<ConnectionIndicatorProps> = ({ connected, label }) => {
  return (
    <div className="flex items-center gap-2 px-3 py-1.5 bg-slate-900/80 rounded-full border border-slate-700">
      <div className={`w-2.5 h-2.5 rounded-full ${
        connected
          ? 'bg-green-500 shadow-[0_0_6px_rgba(34,197,94,0.7)] animate-connection-pulse'
          : 'bg-red-500 shadow-[0_0_6px_rgba(239,68,68,0.7)]'
      }`} />
      <span className={`text-xs font-medium ${connected ? 'text-green-400' : 'text-red-400'}`}>
        {label ? label : (connected ? 'Connecté' : 'Déconnecté')}
      </span>
    </div>
  );
};

// ── Sound-reactive panel ────────────────────────────────────────────

const SOUND_MODES = [
  { id: 0, name: 'Désactivé', icon: '🔇', desc: 'Mode son désactivé' },
  { id: 1, name: 'Volume', icon: '📊', desc: 'Luminosité suit le volume' },
  { id: 2, name: 'Beat', icon: '🥁', desc: 'Flash sur les basses' },
  { id: 3, name: 'Couleur', icon: '🌈', desc: 'Bass=R, Mid=V, Aigu=B' },
  { id: 4, name: 'VU-Mètre', icon: '📶', desc: 'Vert → Jaune → Rouge' },
];

interface SoundPanelProps {
  soundMode: number;
  sensitivity: number;
  dynamics: number;
  audio?: { volume: number; bass: number; mid: number; high: number; beat: boolean };
  onModeChange: (mode: number) => void;
  onSensitivityChange: (sens: number) => void;
  onDynamicsChange: (dyn: number) => void;
}

export const SoundPanel: React.FC<SoundPanelProps> = ({
  soundMode,
  sensitivity,
  dynamics,
  audio,
  onModeChange,
  onSensitivityChange,
  onDynamicsChange,
}) => {
  return (
    <div className="space-y-4">
      {/* Mode selector */}
      <div className="grid grid-cols-2 md:grid-cols-5 gap-2">
        {SOUND_MODES.map((mode) => (
          <button
            key={mode.id}
            onClick={() => onModeChange(mode.id)}
            className={`p-3 rounded-lg border-2 transition-all duration-200 text-center ${
              soundMode === mode.id
                ? mode.id === 0
                  ? 'border-slate-500 bg-slate-700/50'
                  : 'border-cyan-500 bg-cyan-600/20 shadow-lg shadow-cyan-500/30'
                : 'border-slate-700 bg-slate-800 hover:border-cyan-600'
            }`}
            title={mode.desc}
          >
            <div className="text-2xl mb-1">{mode.icon}</div>
            <div className="text-xs font-semibold text-white">{mode.name}</div>
          </button>
        ))}
      </div>

      {/* Active indicator & controls */}
      {soundMode > 0 && (
        <div className="space-y-3">
          {/* Sensitivity & Dynamics sliders */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
             <div className="flex flex-col gap-1">
               <label className="text-xs font-semibold text-slate-400">Sensibilité (Gain)</label>
               <div className="flex items-center gap-3 bg-slate-800/50 p-2 rounded-lg">
                 <MicOff size={16} className="text-slate-500" />
                 <input
                   type="range"
                   min={1}
                   max={10}
                   value={sensitivity}
                   onChange={(e) => onSensitivityChange(parseInt(e.target.value))}
                   className="flex-1 h-2 rounded-lg appearance-none cursor-pointer bg-slate-700
                              [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4
                              [&::-webkit-slider-thumb]:h-4 [&::-webkit-slider-thumb]:rounded-full
                              [&::-webkit-slider-thumb]:bg-cyan-500"
                 />
                 <Mic size={16} className="text-cyan-400" />
                 <span className="text-xs text-slate-400 w-6 text-right">{sensitivity}</span>
               </div>
             </div>
             <div className="flex flex-col gap-1">
               <label className="text-xs font-semibold text-slate-400">Dynamique (Compression)</label>
               <div className="flex items-center gap-3 bg-slate-800/50 p-2 rounded-lg">
                 <span className="text-xs text-slate-500">Min</span>
                 <input
                   type="range"
                   min={0}
                   max={100}
                   value={dynamics}
                   onChange={(e) => onDynamicsChange(parseInt(e.target.value))}
                   className="flex-1 h-2 rounded-lg appearance-none cursor-pointer bg-slate-700
                              [&::-webkit-slider-thumb]:appearance-none [&::-webkit-slider-thumb]:w-4
                              [&::-webkit-slider-thumb]:h-4 [&::-webkit-slider-thumb]:rounded-full
                              [&::-webkit-slider-thumb]:bg-purple-500"
                 />
                 <span className="text-xs text-purple-400">Max</span>
                 <span className="text-xs text-slate-400 w-8 text-right">{dynamics}%</span>
               </div>
             </div>
          </div>

          {/* Audio level bars */}
          {audio && (
            <div className="flex items-end gap-1.5 h-12 p-2 bg-slate-800/50 rounded-lg">
              <AudioBar label="Vol" value={audio.volume} color="bg-cyan-500" />
              <AudioBar label="Bass" value={audio.bass} color="bg-red-500" />
              <AudioBar label="Mid" value={audio.mid} color="bg-green-500" />
              <AudioBar label="Aigu" value={audio.high} color="bg-blue-500" />
              {audio.beat && (
                <div className="ml-2 flex items-center">
                  <div className="w-3 h-3 rounded-full bg-yellow-400 animate-ping" />
                  <span className="text-xs text-yellow-400 ml-1">BEAT</span>
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  );
};

const AudioBar: React.FC<{ label: string; value: number; color: string }> = ({ label, value, color }) => (
  <div className="flex flex-col items-center gap-0.5 flex-1">
    <div className="w-full bg-slate-700 rounded-sm h-8 relative overflow-hidden">
      <div
        className={`absolute bottom-0 w-full rounded-sm transition-all duration-100 ${color}`}
        style={{ height: `${value}%` }}
      />
    </div>
    <span className="text-[8px] text-slate-500">{label}</span>
  </div>
);
