import React from 'react';
import { Zap, Wind } from 'lucide-react';
import { Button } from '@/components/ui';

interface EffectButtonsProps {
  onStrobeClick: () => void;
  onSmokeClick: () => void;
  strobeActive?: boolean;
  smokeActive?: boolean;
}

export const EffectButtons: React.FC<EffectButtonsProps> = ({
  onStrobeClick,
  onSmokeClick,
  strobeActive = false,
  smokeActive = false,
}) => {
  return (
    <div className="grid grid-cols-2 gap-4">
      <Button
        variant="danger"
        size="lg"
        onClick={onStrobeClick}
        className={`flex items-center justify-center gap-2 ${strobeActive ? 'animate-pulse-glow' : ''}`}
      >
        <Zap size={24} />
        <span>STROBE</span>
      </Button>

      <Button
        variant="danger"
        size="lg"
        onClick={onSmokeClick}
        className={`flex items-center justify-center gap-2 ${smokeActive ? 'animate-pulse-glow' : ''}`}
      >
        <Wind size={24} />
        <span>SMOKE</span>
      </Button>
    </div>
  );
};

interface SceneGridProps {
  scenes: Array<{ id: string; name: string; description: string; icon: string }>;
  onSceneSelect: (sceneId: string) => void;
  activeSceneId?: string;
}

export const SceneGrid: React.FC<SceneGridProps> = ({ scenes, onSceneSelect, activeSceneId }) => {
  return (
    <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-3">
      {scenes.map((scene) => (
        <button
          key={scene.id}
          onClick={() => onSceneSelect(scene.id)}
          className={`p-4 rounded-lg border-2 transition-all duration-200 ${
            activeSceneId === scene.id
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
