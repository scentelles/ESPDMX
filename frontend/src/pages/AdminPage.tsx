import { useEffect, useState, useRef, useCallback } from 'react';
import { Settings, Plus, Trash2, Save, LogOut, GripVertical, ChevronDown, ChevronUp, Sliders, Play, Mic, MicOff, Volume2 } from 'lucide-react';
import { Button, Card, Input, Select, Alert, LoadingSpinner, ErrorNotification } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { DMXFixture, ChannelDefinition, ChannelType, ColorScene, DynamicShow, ShowStep, SystemConfig } from '@/types';

// ── Preset channel templates ──────────────────────────────────────────
const FIXTURE_PRESETS: Record<string, ChannelDefinition[]> = {
  dimmer: [
    { name: 'Dimmer', offset: 0, defaultValue: 0, type: 'dimmer' },
  ],
  rgb: [
    { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
    { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
    { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
  ],
  rgbw: [
    { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
    { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
    { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
    { name: 'White', offset: 3, defaultValue: 0, type: 'color' },
  ],
  rgbwd: [
    { name: 'Dimmer', offset: 0, defaultValue: 0, type: 'dimmer' },
    { name: 'Red', offset: 1, defaultValue: 0, type: 'color' },
    { name: 'Green', offset: 2, defaultValue: 0, type: 'color' },
    { name: 'Blue', offset: 3, defaultValue: 0, type: 'color' },
    { name: 'White', offset: 4, defaultValue: 0, type: 'color' },
  ],
  moving_head: [
    { name: 'Pan', offset: 0, defaultValue: 128, type: 'position' },
    { name: 'Tilt', offset: 1, defaultValue: 128, type: 'position' },
    { name: 'Dimmer', offset: 2, defaultValue: 0, type: 'dimmer' },
    { name: 'Red', offset: 3, defaultValue: 0, type: 'color' },
    { name: 'Green', offset: 4, defaultValue: 0, type: 'color' },
    { name: 'Blue', offset: 5, defaultValue: 0, type: 'color' },
    { name: 'Speed', offset: 6, defaultValue: 0, type: 'speed' },
    { name: 'Gobo', offset: 7, defaultValue: 0, type: 'gobo' },
  ],
};

const CHANNEL_TYPES: { value: ChannelType; label: string }[] = [
  { value: 'dimmer', label: 'Variateur' },
  { value: 'color', label: 'Couleur' },
  { value: 'position', label: 'Position' },
  { value: 'speed', label: 'Vitesse' },
  { value: 'gobo', label: 'Gobo' },
  { value: 'other', label: 'Autre' },
];

// ── Helper: channel color hint for sliders ───────────────────────────
function channelSliderColor(ch: ChannelDefinition): string {
  const n = ch.name.toLowerCase();
  if (n === 'red') return 'accent-red-500';
  if (n === 'green') return 'accent-green-500';
  if (n === 'blue') return 'accent-blue-500';
  if (n === 'white') return 'accent-white';
  return 'accent-purple-600';
}

// ═══════════════════════════════════════════════════════════════════════
// Fixture Editor component
// ═══════════════════════════════════════════════════════════════════════
interface FixtureEditorProps {
  fixture: DMXFixture;
  onChange: (fixture: DMXFixture) => void;
  onSave: () => void;
  onCancel: () => void;
}

const FixtureEditor: React.FC<FixtureEditorProps> = ({ fixture, onChange, onSave, onCancel }) => {
  const computeChannelCount = (channels: ChannelDefinition[]) =>
    channels.length > 0 ? Math.max(...channels.map(c => c.offset)) + 1 : 0;

  const applyPreset = (preset: string) => {
    const channels = FIXTURE_PRESETS[preset];
    if (channels) {
      onChange({
        ...fixture,
        type: preset,
        channels: channels.map(ch => ({ ...ch })),
        channelCount: computeChannelCount(channels),
      });
    }
  };

  const updateChannel = (idx: number, updates: Partial<ChannelDefinition>) => {
    const newChannels = [...fixture.channels];
    newChannels[idx] = { ...newChannels[idx], ...updates };
    onChange({ ...fixture, channels: newChannels, channelCount: computeChannelCount(newChannels) });
  };

  const addChannel = () => {
    const nextOffset = fixture.channels.length > 0
      ? Math.max(...fixture.channels.map(c => c.offset)) + 1
      : 0;
    onChange({
      ...fixture,
      channels: [...fixture.channels, { name: 'Channel ' + (fixture.channels.length + 1), offset: nextOffset, defaultValue: 0, type: 'other' as ChannelType }],
      channelCount: nextOffset + 1,
    });
  };

  const removeChannel = (idx: number) => {
    const newChannels = fixture.channels.filter((_, i) => i !== idx);
    onChange({ ...fixture, channels: newChannels, channelCount: computeChannelCount(newChannels) });
  };

  return (
    <div className="mb-6 p-4 bg-slate-800 rounded-lg border border-slate-700">
      <h3 className="text-lg font-bold text-white mb-4">
        {fixture.id.startsWith('new-') ? 'Nouveau Projecteur' : 'Modifier le Projecteur'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Nom du Projecteur"
          value={fixture.name}
          onChange={(e) => onChange({ ...fixture, name: e.target.value })}
        />
        <Input
          label="Adresse DMX de Départ"
          type="number"
          min={1}
          max={512}
          value={fixture.dmxAddress}
          onChange={(e) => onChange({ ...fixture, dmxAddress: parseInt(e.target.value) || 1 })}
        />
        <Select
          label="Charger un Préréglage"
          value={fixture.type}
          onChange={(e) => applyPreset(e.target.value)}
          options={[
            { value: '', label: '— Choisir un préréglage —' },
            { value: 'dimmer', label: 'Variateur (1ch)' },
            { value: 'rgb', label: 'RGB (3ch)' },
            { value: 'rgbw', label: 'RGBW (4ch)' },
            { value: 'rgbwd', label: 'RGBW+Variateur (5ch)' },
            { value: 'moving_head', label: 'Lyre (8ch)' },
          ]}
        />
      </div>

      {/* Channel definitions */}
      <div className="mb-4">
        <div className="flex items-center justify-between mb-2">
          <label className="text-sm font-medium text-slate-300">
            Canaux ({fixture.channels.length}) — DMX {fixture.dmxAddress}–{fixture.dmxAddress + (fixture.channels.length > 0 ? Math.max(...fixture.channels.map(c => c.offset)) : 0)}
          </label>
          <Button variant="outline" size="sm" onClick={addChannel} className="flex items-center gap-1">
            <Plus size={14} /> Ajouter un Canal
          </Button>
        </div>

        <div className="space-y-2 max-h-80 overflow-y-auto pr-1">
          {fixture.channels.map((ch, idx) => (
            <div key={idx} className="flex items-center gap-2 bg-slate-900 rounded p-2">
              <span className="text-xs text-slate-500 w-8 text-center">{fixture.dmxAddress + ch.offset}</span>
              <Input
                value={ch.name}
                onChange={(e) => updateChannel(idx, { name: e.target.value })}
                className="flex-1 !py-1 text-sm"
              />
              <select
                value={ch.type}
                onChange={(e) => updateChannel(idx, { type: e.target.value as ChannelType })}
                className="bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
              >
                {CHANNEL_TYPES.map(t => (
                  <option key={t.value} value={t.value}>{t.label}</option>
                ))}
              </select>
              <Input
                type="number"
                min={0}
                max={511}
                value={ch.offset}
                onChange={(e) => updateChannel(idx, { offset: parseInt(e.target.value) || 0 })}
                className="w-16 !py-1 text-sm"
                title="Offset DMX"
              />
              <button onClick={() => removeChannel(idx)} className="text-red-400 hover:text-red-300 p-1">
                <Trash2 size={14} />
              </button>
            </div>
          ))}
        </div>
      </div>

      <div className="flex items-center gap-4 mb-4">
        <label className="flex items-center gap-2 text-sm text-slate-300">
          <input
            type="checkbox"
            checked={fixture.enabled}
            onChange={(e) => onChange({ ...fixture, enabled: e.target.checked })}
            className="accent-purple-600"
          />
          Activé
        </label>
      </div>

      {/* Strobe channel config */}
      <div className="mb-4 p-3 bg-slate-900 rounded border border-slate-700">
        <label className="flex items-center gap-2 text-sm text-slate-300 mb-2">
          <input
            type="checkbox"
            checked={fixture.strobeChannel?.enabled ?? false}
            onChange={(e) => onChange({ ...fixture, strobeChannel: { offset: fixture.strobeChannel?.offset ?? 0, value: fixture.strobeChannel?.value ?? 255, enabled: e.target.checked } })}
            className="accent-yellow-500"
          />
          Canal Stroboscope
        </label>
        {fixture.strobeChannel?.enabled && (
          <div className="flex items-center gap-4 mt-2">
            <Input
              label="Offset DMX"
              type="number"
              min={0}
              max={511}
              value={fixture.strobeChannel.offset}
              onChange={(e) => onChange({ ...fixture, strobeChannel: { ...fixture.strobeChannel!, offset: parseInt(e.target.value) || 0 } })}
              className="w-24"
            />
            <Input
              label="Valeur (0-255)"
              type="number"
              min={0}
              max={255}
              value={fixture.strobeChannel.value}
              onChange={(e) => onChange({ ...fixture, strobeChannel: { ...fixture.strobeChannel!, value: parseInt(e.target.value) || 0 } })}
              className="w-24"
            />
            <span className="text-xs text-slate-500 mt-5">
              → DMX {fixture.dmxAddress + (fixture.strobeChannel.offset ?? 0)}
            </span>
          </div>
        )}
      </div>

      <div className="flex gap-2">
        <Button variant="primary" onClick={onSave} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Enregistrer le Projecteur
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Annuler
        </Button>
      </div>
    </div>
  );
};

// ═══════════════════════════════════════════════════════════════════════
// Scene Editor component — per-fixture channel value sliders
// ═══════════════════════════════════════════════════════════════════════
interface SceneEditorProps {
  scene: ColorScene;
  fixtures: DMXFixture[];
  onChange: (scene: ColorScene) => void;
  onSave: () => void;
  onCancel: () => void;
  onTest: () => void;
}

const SceneEditor: React.FC<SceneEditorProps> = ({ scene, fixtures, onChange, onSave, onCancel, onTest }) => {
  const [expandedFixtures, setExpandedFixtures] = useState<Set<string>>(new Set());

  const toggleFixture = (fixtureId: string) => {
    const next = new Set(expandedFixtures);
    if (next.has(fixtureId)) next.delete(fixtureId);
    else next.add(fixtureId);
    setExpandedFixtures(next);
  };

  const isFixtureInScene = (fixtureId: string) =>
    scene.fixtureValues.some(fv => fv.fixtureId === fixtureId);

  const toggleFixtureInScene = (fixture: DMXFixture) => {
    if (isFixtureInScene(fixture.id)) {
      onChange({
        ...scene,
        fixtureValues: scene.fixtureValues.filter(fv => fv.fixtureId !== fixture.id),
      });
    } else {
      const defaults: Record<string, number> = {};
      fixture.channels.forEach(ch => { defaults[ch.name] = ch.defaultValue; });
      onChange({
        ...scene,
        fixtureValues: [...scene.fixtureValues, { fixtureId: fixture.id, values: defaults }],
      });
      setExpandedFixtures(prev => new Set(prev).add(fixture.id));
    }
  };

  const setChannelValue = (fixtureId: string, channelName: string, value: number) => {
    onChange({
      ...scene,
      fixtureValues: scene.fixtureValues.map(fv =>
        fv.fixtureId === fixtureId
          ? { ...fv, values: { ...fv.values, [channelName]: value } }
          : fv
      ),
    });
  };

  const getChannelValue = (fixtureId: string, channelName: string): number => {
    const fv = scene.fixtureValues.find(fv => fv.fixtureId === fixtureId);
    return fv?.values[channelName] ?? 0;
  };

  return (
    <div className="mb-6 p-4 bg-slate-800 rounded-lg border border-slate-700">
      <h3 className="text-lg font-bold text-white mb-4">
        {scene.id.startsWith('new-') ? 'Nouvelle Scène' : 'Modifier la Scène'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Nom de la Scène"
          value={scene.name}
          onChange={(e) => onChange({ ...scene, name: e.target.value })}
        />
        <Input
          label="Icône (emoji)"
          value={scene.icon}
          onChange={(e) => onChange({ ...scene, icon: e.target.value })}
        />
        <Input
          label="Description"
          value={scene.description}
          onChange={(e) => onChange({ ...scene, description: e.target.value })}
        />
      </div>

      {/* Per-fixture channel values */}
      <div className="mb-4">
        <label className="text-sm font-medium text-slate-300 mb-2 block">Valeurs des Projecteurs</label>
        {fixtures.length === 0 && (
          <Alert variant="warning">Aucun projecteur configuré. Ajoutez d'abord des projecteurs dans l'onglet Projecteurs.</Alert>
        )}
        <div className="space-y-2">
          {fixtures.map(fixture => {
            const inScene = isFixtureInScene(fixture.id);
            const expanded = expandedFixtures.has(fixture.id);
            return (
              <div key={fixture.id} className="bg-slate-900 rounded-lg border border-slate-700 overflow-hidden">
                <div
                  className="flex items-center gap-3 p-3 cursor-pointer hover:bg-slate-800/50"
                  onClick={() => inScene && toggleFixture(fixture.id)}
                >
                  <input
                    type="checkbox"
                    checked={inScene}
                    onChange={() => toggleFixtureInScene(fixture)}
                    onClick={(e) => e.stopPropagation()}
                    className="accent-purple-600"
                  />
                  <span className="font-medium text-white flex-1">{fixture.name}</span>
                  <span className="text-xs text-slate-500">DMX {fixture.dmxAddress} • {fixture.channels.length}ch</span>
                  {inScene && (
                    expanded ? <ChevronUp size={16} className="text-slate-400" /> : <ChevronDown size={16} className="text-slate-400" />
                  )}
                </div>

                {inScene && expanded && (
                  <div className="p-3 pt-0 space-y-3 border-t border-slate-700">
                    {fixture.channels.map(ch => (
                      <div key={ch.name} className="flex items-center gap-3">
                        <span className="text-sm text-slate-400 w-20 truncate" title={ch.name}>{ch.name}</span>
                        <input
                          type="range"
                          min={0}
                          max={255}
                          value={getChannelValue(fixture.id, ch.name)}
                          onChange={(e) => setChannelValue(fixture.id, ch.name, parseInt(e.target.value))}
                          className={`flex-1 h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer ${channelSliderColor(ch)}`}
                        />
                        <input
                          type="number"
                          min={0}
                          max={255}
                          value={getChannelValue(fixture.id, ch.name)}
                          onChange={(e) => setChannelValue(fixture.id, ch.name, Math.min(255, Math.max(0, parseInt(e.target.value) || 0)))}
                          className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white text-center"
                        />
                      </div>
                    ))}
                  </div>
                )}
              </div>
            );
          })}
        </div>
      </div>

      <div className="flex gap-2">
        <Button variant="primary" onClick={onSave} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Enregistrer la Scène
        </Button>
        <Button variant="outline" onClick={onTest} className="flex items-center justify-center gap-2" title="Envoyer cette scène aux projecteurs">
          <Play size={20} /> Tester
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Annuler
        </Button>
      </div>
    </div>
  );
};

// ═══════════════════════════════════════════════════════════════════════
// Show Editor component — scene sequence with timing
// ═══════════════════════════════════════════════════════════════════════
interface ShowEditorProps {
  show: DynamicShow;
  scenes: ColorScene[];
  onChange: (show: DynamicShow) => void;
  onSave: () => void;
  onCancel: () => void;
  onTestScene: (sceneId: string) => void;
}

const ShowEditor: React.FC<ShowEditorProps> = ({ show, scenes, onChange, onSave, onCancel, onTestScene }) => {
  const addStep = () => {
    if (scenes.length === 0) return;
    onChange({
      ...show,
      steps: [...show.steps, { sceneId: scenes[0].id, duration: 5000, transitionTime: 1000, smoothTransition: false }],
    });
  };

  const updateStep = (idx: number, updates: Partial<ShowStep>) => {
    const newSteps = [...show.steps];
    newSteps[idx] = { ...newSteps[idx], ...updates };
    onChange({ ...show, steps: newSteps });
  };

  const removeStep = (idx: number) => {
    onChange({ ...show, steps: show.steps.filter((_, i) => i !== idx) });
  };

  const moveStep = (idx: number, direction: -1 | 1) => {
    const newIdx = idx + direction;
    if (newIdx < 0 || newIdx >= show.steps.length) return;
    const newSteps = [...show.steps];
    [newSteps[idx], newSteps[newIdx]] = [newSteps[newIdx], newSteps[idx]];
    onChange({ ...show, steps: newSteps });
  };

  const totalDuration = show.steps.reduce((sum, s) => sum + s.duration + s.transitionTime, 0);

  return (
    <div className="mb-6 p-4 bg-slate-800 rounded-lg border border-slate-700">
      <h3 className="text-lg font-bold text-white mb-4">
        {show.id.startsWith('new-') ? 'Nouveau Show' : 'Modifier le Show'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Nom du Show"
          value={show.name}
          onChange={(e) => onChange({ ...show, name: e.target.value })}
        />
        <Input
          label="Icône (emoji)"
          value={show.icon}
          onChange={(e) => onChange({ ...show, icon: e.target.value })}
        />
        <Input
          label="Description"
          value={show.description}
          onChange={(e) => onChange({ ...show, description: e.target.value })}
        />
      </div>

      <label className="flex items-center gap-2 text-sm text-slate-300 mb-4">
        <input
          type="checkbox"
          checked={show.loop}
          onChange={(e) => onChange({ ...show, loop: e.target.checked })}
          className="accent-purple-600"
        />
        Boucle continue
      </label>

      {/* Scene steps */}
      <div className="mb-4">
        <div className="flex items-center justify-between mb-2">
          <label className="text-sm font-medium text-slate-300">
            Étapes ({show.steps.length}) — Total: {(totalDuration / 1000).toFixed(1)}s
          </label>
          <Button variant="outline" size="sm" onClick={addStep} disabled={scenes.length === 0} className="flex items-center gap-1">
            <Plus size={14} /> Ajouter une Étape
          </Button>
        </div>

        {scenes.length === 0 && (
          <Alert variant="warning">Aucune scène définie. Créez d'abord des scènes dans l'onglet Scènes.</Alert>
        )}

        <div className="space-y-2 max-h-96 overflow-y-auto pr-1">
          {show.steps.map((step, idx) => (
            <div key={idx} className="bg-slate-900 rounded-lg border border-slate-700 p-3">
              <div className="flex items-center gap-2 mb-2">
                <GripVertical size={16} className="text-slate-600" />
                <span className="text-xs text-slate-500 font-mono w-6">#{idx + 1}</span>
                <select
                  value={step.sceneId}
                  onChange={(e) => updateStep(idx, { sceneId: e.target.value })}
                  className="flex-1 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
                >
                  {scenes.map(s => (
                    <option key={s.id} value={s.id}>{s.icon} {s.name}</option>
                  ))}
                </select>
                <button onClick={() => onTestScene(step.sceneId)} className="text-green-400 hover:text-green-300 p-1" title="Tester cette scène">
                  <Play size={14} />
                </button>
                <button onClick={() => moveStep(idx, -1)} disabled={idx === 0} className="text-slate-400 hover:text-white disabled:opacity-30 p-1">
                  <ChevronUp size={16} />
                </button>
                <button onClick={() => moveStep(idx, 1)} disabled={idx === show.steps.length - 1} className="text-slate-400 hover:text-white disabled:opacity-30 p-1">
                  <ChevronDown size={16} />
                </button>
                <button onClick={() => removeStep(idx)} className="text-red-400 hover:text-red-300 p-1">
                  <Trash2 size={14} />
                </button>
              </div>
              <div className="grid grid-cols-2 gap-3 pl-8">
                <div>
                  <label className="text-xs text-slate-500">Durée de Maintien</label>
                  <div className="flex items-center gap-2">
                    <input
                      type="range"
                      min={100}
                      max={30000}
                      step={100}
                      value={step.duration}
                      onChange={(e) => updateStep(idx, { duration: parseInt(e.target.value) })}
                      className="flex-1 h-1.5 bg-slate-700 rounded appearance-none cursor-pointer accent-purple-600"
                    />
                    <span className="text-xs text-slate-300 w-12 text-right">{(step.duration / 1000).toFixed(1)}s</span>
                  </div>
                </div>
                <div>
                  <label className="text-xs text-slate-500">Durée de Transition</label>
                  <div className="flex items-center gap-2">
                    <input
                      type="range"
                      min={0}
                      max={10000}
                      step={100}
                      value={step.transitionTime}
                      onChange={(e) => updateStep(idx, { transitionTime: parseInt(e.target.value) })}
                      className="flex-1 h-1.5 bg-slate-700 rounded appearance-none cursor-pointer accent-pink-500"
                    />
                    <span className="text-xs text-slate-300 w-12 text-right">{(step.transitionTime / 1000).toFixed(1)}s</span>
                  </div>
                </div>
              </div>
              {step.transitionTime > 0 && (
                <div className="pl-8 mt-1">
                  <label className="flex items-center gap-2 text-xs text-slate-400">
                    <input
                      type="checkbox"
                      checked={step.smoothTransition ?? false}
                      onChange={(e) => updateStep(idx, { smoothTransition: e.target.checked })}
                      className="accent-pink-500"
                    />
                    Transition progressive (fondu DMX)
                  </label>
                </div>
              )}
            </div>
          ))}
        </div>
      </div>

      <div className="flex gap-2">
        <Button variant="primary" onClick={onSave} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Enregistrer le Show
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Annuler
        </Button>
      </div>
    </div>
  );
};

// ═══════════════════════════════════════════════════════════════════════
// Admin Page
// ═══════════════════════════════════════════════════════════════════════
interface AdminPageProps {
  onLogout: () => void;
}

export const AdminPage: React.FC<AdminPageProps> = ({ onLogout }) => {
  const store = useAppStore();
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'fixtures' | 'scenes' | 'shows' | 'dmxtest' | 'audio' | 'settings'>('fixtures');
  // Audio state
  const [audioData, setAudioData] = useState<{
    volume: number; bass: number; mid: number; high: number;
    peak: number; beat: boolean; mode: number; sensitivity: number;
    diag: { rawMin: number; rawMax: number; rawRms: number; rawSpan: number };
  } | null>(null);
  const audioTimerRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const [audioHistory, setAudioHistory] = useState<number[]>([]);
  const [editingFixture, setEditingFixture] = useState<DMXFixture | null>(null);
  const [editingScene, setEditingScene] = useState<ColorScene | null>(null);
  const [editingShow, setEditingShow] = useState<DynamicShow | null>(null);
  const [config, setConfig] = useState<Partial<SystemConfig>>({});
  const [dmxSliders, setDmxSliders] = useState<{ address: number; value: number }[]>([
    { address: 1, value: 0 },
    { address: 2, value: 0 },
    { address: 3, value: 0 },
    { address: 4, value: 0 },
    { address: 5, value: 0 },
    { address: 6, value: 0 },
    { address: 7, value: 0 },
    { address: 8, value: 0 },
    { address: 9, value: 0 },
    { address: 10, value: 0 },
  ]);

  useEffect(() => {
    loadAdminData();
  }, []);

  // Audio polling
  const pollAudio = useCallback(async () => {
    try {
      const data = await apiService.getAudio();
      setAudioData(data);
      setAudioHistory(prev => {
        const next = [...prev, data.volume];
        if (next.length > 60) next.shift();
        return next;
      });
    } catch { /* ignore polling errors */ }
  }, []);

  useEffect(() => {
    if (activeTab === 'audio') {
      pollAudio(); // immediate first poll
      audioTimerRef.current = setInterval(pollAudio, 200);
    } else {
      if (audioTimerRef.current) {
        clearInterval(audioTimerRef.current);
        audioTimerRef.current = null;
      }
    }
    return () => {
      if (audioTimerRef.current) {
        clearInterval(audioTimerRef.current);
        audioTimerRef.current = null;
      }
    };
  }, [activeTab, pollAudio]);

  const handleSetSoundMode = async (mode: number) => {
    try {
      await apiService.setSoundMode(mode, audioData?.sensitivity);
    } catch (e) {
      store.setError('Erreur changement mode son');
    }
  };

  const handleSetSensitivity = async (sens: number) => {
    try {
      await apiService.setSoundMode(audioData?.mode ?? 0, sens);
    } catch (e) {
      store.setError('Erreur changement sensibilité');
    }
  };

  const loadAdminData = async () => {
    try {
      const [fixtures, scenes, shows, sysConfig] = await Promise.all([
        apiService.getFixtures(),
        apiService.getScenes(),
        apiService.getShows(),
        apiService.getSystemConfig(),
      ]);
      store.setFixtures(fixtures);
      store.setScenes(scenes);
      store.setShows(shows);
      setConfig(sysConfig);
      setLoading(false);
    } catch (error) {
      store.setError('Erreur de chargement des données admin');
      console.error(error);
      setLoading(false);
    }
  };

  // ── Fixture CRUD ────────────────────────────────────────────────────
  const handleNewFixture = () => {
    setEditingFixture({
      id: 'new-' + Date.now(),
      name: 'Nouveau Projecteur',
      type: 'rgb',
      dmxAddress: 1,
      channelCount: 3,
      channels: [
        { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
        { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
        { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
      ],
      enabled: true,
    });
  };

  const handleSaveFixture = async () => {
    if (!editingFixture) return;
    try {
      await apiService.saveFixture(editingFixture);
      await loadAdminData();
      setEditingFixture(null);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de l\'enregistrement du projecteur';
      store.setError(message);
    }
  };

  const handleDeleteFixture = async (id: string) => {
    if (!confirm('Supprimer ce projecteur ? Les scènes l\'utilisant perdront ces valeurs.')) return;
    try {
      await apiService.deleteFixture(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de la suppression du projecteur';
      store.setError(message);
    }
  };

  // ── Scene CRUD ──────────────────────────────────────────────────────

  const handleTestScene = async (sceneId: string) => {
    try {
      await apiService.activateScene(sceneId);
    } catch (error) {
      console.error('Test scene failed:', error);
    }
  };

  const handleTestEditingScene = async () => {
    if (!editingScene) return;
    // Save the scene first (so firmware has the latest values), then activate it
    try {
      await apiService.saveScene(editingScene);
      await apiService.activateScene(editingScene.id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec du test de la scène';
      store.setError(message);
    }
  };

  const handleNewScene = () => {
    setEditingScene({
      id: 'new-' + Date.now(),
      name: 'Nouvelle Scène',
      description: '',
      icon: '🎨',
      fixtureValues: [],
    });
  };

  const handleSaveScene = async () => {
    if (!editingScene) return;
    try {
      await apiService.saveScene(editingScene);
      await loadAdminData();
      setEditingScene(null);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de l\'enregistrement de la scène';
      store.setError(message);
    }
  };

  const handleDeleteScene = async (id: string) => {
    if (!confirm('Supprimer cette scène ? Les shows la référençant perdront ces étapes.')) return;
    try {
      await apiService.deleteScene(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de la suppression de la scène';
      store.setError(message);
    }
  };

  // ── Show CRUD ───────────────────────────────────────────────────────
  const handleNewShow = () => {
    setEditingShow({
      id: 'new-' + Date.now(),
      name: 'Nouveau Show',
      description: '',
      icon: '🎆',
      loop: true,
      steps: [],
    });
  };

  const handleSaveShow = async () => {
    if (!editingShow) return;
    try {
      await apiService.saveShow(editingShow);
      await loadAdminData();
      setEditingShow(null);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de l\'enregistrement du show';
      store.setError(message);
    }
  };

  const handleDeleteShow = async (id: string) => {
    if (!confirm('Supprimer ce show ?')) return;
    try {
      await apiService.deleteShow(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de la suppression du show';
      store.setError(message);
    }
  };

  // ── Config ──────────────────────────────────────────────────────────
  const handleSaveConfig = async () => {
    try {
      await apiService.saveSystemConfig(config);
      store.setError(null);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Échec de l\'enregistrement de la configuration';
      store.setError(message);
    }
  };

  // ── Helpers ─────────────────────────────────────────────────────────
  const sceneSummary = (scene: ColorScene): string => {
    const count = scene.fixtureValues?.length || 0;
    return count === 0 ? 'Aucun projecteur' : `${count} projecteur${count > 1 ? 's' : ''}`;
  };

  const showSummary = (show: DynamicShow): string => {
    const steps = show.steps || [];
    const total = steps.reduce((s, step) => s + step.duration + step.transitionTime, 0);
    return `${steps.length} étape${steps.length !== 1 ? 's' : ''} • ${(total / 1000).toFixed(1)}s${show.loop ? ' • boucle' : ''}`;
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center min-h-screen">
        <LoadingSpinner size="lg" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-950 via-purple-900 to-slate-950 p-4 md:p-8">
      <ErrorNotification />

      {/* Header */}
      <div className="max-w-7xl mx-auto mb-8 flex justify-between items-center">
        <div>
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-2">⚙️ Panneau Admin</h1>
          <p className="text-slate-400">Configurez votre système d'éclairage DMX</p>
        </div>
        <Button variant="outline" onClick={onLogout} className="flex items-center gap-2">
          <LogOut size={20} /> Déconnexion
        </Button>
      </div>

      <div className="max-w-7xl mx-auto">
        {/* Navigation Tabs */}
        <div className="flex gap-2 mb-8 overflow-x-auto pb-2">
          {(['fixtures', 'scenes', 'shows', 'dmxtest', 'audio', 'settings'] as const).map((tab) => {
            const tabLabels: Record<string, string> = { fixtures: 'Projecteurs', scenes: 'Scènes', shows: 'Shows', dmxtest: 'Test DMX', audio: '🎤 Audio', settings: 'Paramètres' };
            return (
              <button
                key={tab}
                onClick={() => setActiveTab(tab)}
                className={`px-4 py-2 rounded-lg font-semibold transition-all whitespace-nowrap ${
                  activeTab === tab
                    ? 'bg-purple-600 text-white'
                    : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
                }`}
              >
                {tabLabels[tab]}
              </button>
            );
          })}
        </div>

        {/* ═══ Fixtures Tab ═══ */}
        {activeTab === 'fixtures' && (
          <div className="space-y-6">
            <Card>
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-2xl font-bold text-white flex items-center gap-2">
                  <Settings size={24} /> Projecteurs DMX
                </h2>
                <Button variant="secondary" onClick={handleNewFixture} className="flex items-center gap-2">
                  <Plus size={20} /> Ajouter un Projecteur
                </Button>
              </div>

              {editingFixture && (
                <FixtureEditor
                  fixture={editingFixture}
                  onChange={setEditingFixture}
                  onSave={handleSaveFixture}
                  onCancel={() => setEditingFixture(null)}
                />
              )}

              {store.fixtures.length === 0 && !editingFixture && (
                <div className="text-center py-8 text-slate-500">
                  Aucun projecteur configuré. Cliquez sur "Ajouter un Projecteur" pour commencer.
                </div>
              )}

              <div className="space-y-2">
                {store.fixtures.map((fixture) => (
                  <div
                    key={fixture.id}
                    className="flex items-center justify-between bg-slate-800 p-4 rounded-lg border border-slate-700"
                  >
                    <div className="flex-1">
                      <div className="font-semibold text-white">{fixture.name}</div>
                      <div className="text-sm text-slate-400">
                        DMX {fixture.dmxAddress}–{fixture.dmxAddress + (fixture.channels?.length || fixture.channelCount) - 1}
                        {' • '}{fixture.type}
                        {' • '}{fixture.channels?.length || fixture.channelCount} ch
                        {fixture.channels?.length > 0 && (
                          <span className="text-slate-600"> ({fixture.channels.map(c => c.name).join(', ')})</span>
                        )}
                        {!fixture.enabled && <span className="text-red-400 ml-2">(désactivé)</span>}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => setEditingFixture(fixture)}>
                        Modifier
                      </Button>
                      <Button variant="danger" size="sm" onClick={() => handleDeleteFixture(fixture.id)}>
                        <Trash2 size={16} />
                      </Button>
                    </div>
                  </div>
                ))}
              </div>
            </Card>
          </div>
        )}

        {/* ═══ Scenes Tab ═══ */}
        {activeTab === 'scenes' && (
          <div className="space-y-6">
            <Card>
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-2xl font-bold text-white">🎨 Scènes Couleur</h2>
                <Button variant="secondary" onClick={handleNewScene} className="flex items-center gap-2">
                  <Plus size={20} /> Ajouter une Scène
                </Button>
              </div>

              {editingScene && (
                <SceneEditor
                  scene={editingScene}
                  fixtures={store.fixtures}
                  onChange={setEditingScene}
                  onSave={handleSaveScene}
                  onCancel={() => setEditingScene(null)}
                  onTest={handleTestEditingScene}
                />
              )}

              {store.scenes.length === 0 && !editingScene && (
                <div className="text-center py-8 text-slate-500">
                  Aucune scène définie. Cliquez sur "Ajouter une Scène" pour en créer une.
                </div>
              )}

              <div className="space-y-2">
                {store.scenes.map((scene) => (
                  <div
                    key={scene.id}
                    className="flex items-center justify-between bg-slate-800 p-4 rounded-lg border border-slate-700"
                  >
                    <div className="flex-1">
                      <div className="font-semibold text-white">{scene.icon} {scene.name}</div>
                      <div className="text-sm text-slate-400">
                        {scene.description || 'Aucune description'}
                        {' • '}{sceneSummary(scene)}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => handleTestScene(scene.id)} title="Tester cette scène" className="flex items-center gap-1 text-green-400 border-green-700 hover:bg-green-900/30">
                        <Play size={14} /> Test
                      </Button>
                      <Button variant="outline" size="sm" onClick={() => setEditingScene(scene)}>
                        Modifier
                      </Button>
                      <Button variant="danger" size="sm" onClick={() => handleDeleteScene(scene.id)}>
                        <Trash2 size={16} />
                      </Button>
                    </div>
                  </div>
                ))}
              </div>
            </Card>
          </div>
        )}

        {/* ═══ Shows Tab ═══ */}
        {activeTab === 'shows' && (
          <div className="space-y-6">
            <Card>
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-2xl font-bold text-white">🎆 Shows Dynamiques</h2>
                <Button variant="secondary" onClick={handleNewShow} className="flex items-center gap-2">
                  <Plus size={20} /> Ajouter un Show
                </Button>
              </div>

              {editingShow && (
                <ShowEditor
                  show={editingShow}
                  scenes={store.scenes}
                  onChange={setEditingShow}
                  onSave={handleSaveShow}
                  onCancel={() => setEditingShow(null)}
                  onTestScene={handleTestScene}
                />
              )}

              {store.shows.length === 0 && !editingShow && (
                <div className="text-center py-8 text-slate-500">
                  Aucun show défini. Cliquez sur "Ajouter un Show" pour en créer un.
                </div>
              )}

              <div className="space-y-2">
                {store.shows.map((show) => (
                  <div
                    key={show.id}
                    className="flex items-center justify-between bg-slate-800 p-4 rounded-lg border border-slate-700"
                  >
                    <div className="flex-1">
                      <div className="font-semibold text-white">{show.icon} {show.name}</div>
                      <div className="text-sm text-slate-400">
                        {show.description || 'Aucune description'}
                        {' • '}{showSummary(show)}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => setEditingShow(show)}>
                        Modifier
                      </Button>
                      <Button variant="danger" size="sm" onClick={() => handleDeleteShow(show.id)}>
                        <Trash2 size={16} />
                      </Button>
                    </div>
                  </div>
                ))}
              </div>
            </Card>
          </div>
        )}

        {/* ═══ DMX Test Tab ═══ */}
        {activeTab === 'dmxtest' && (
          <div className="space-y-6">
            <Card>
              <h2 className="text-2xl font-bold text-white mb-6 flex items-center gap-2">
                <Sliders /> Test DMX — Sliders Directs
              </h2>
              <p className="text-sm text-slate-400 mb-6">
                Envoyez des valeurs DMX directement sur n'importe quelle adresse (1–512) pour tester vos projecteurs.
              </p>
              <div className="space-y-6">
                {dmxSliders.map((slider, index) => (
                  <div key={index} className="bg-slate-800 rounded-lg p-4 border border-slate-700">
                    <div className="flex items-center gap-4 mb-3">
                      <span className="text-white font-bold text-lg w-8">{index + 1}</span>
                      <label className="text-slate-400 text-sm">Adresse DMX :</label>
                      <input
                        type="number"
                        min={1}
                        max={512}
                        value={slider.address}
                        onChange={(e) => {
                          const addr = Math.max(1, Math.min(512, parseInt(e.target.value) || 1));
                          setDmxSliders(prev => prev.map((s, i) => i === index ? { ...s, address: addr } : s));
                        }}
                        className="w-20 bg-slate-900 border border-slate-600 rounded px-2 py-1 text-white text-center"
                      />
                      <span className="ml-auto text-2xl font-mono font-bold text-purple-400 w-16 text-right">
                        {slider.value}
                      </span>
                    </div>
                    <div className="flex items-center gap-3">
                      <span className="text-slate-500 text-xs w-6">0</span>
                      <input
                        type="range"
                        min={0}
                        max={255}
                        value={slider.value}
                        onChange={(e) => {
                          const val = parseInt(e.target.value);
                          setDmxSliders(prev => prev.map((s, i) => i === index ? { ...s, value: val } : s));
                          apiService.setDMXChannel(slider.address, val);
                        }}
                        className="flex-1 h-3 appearance-none rounded-full bg-slate-700 accent-purple-500 cursor-pointer"
                      />
                      <span className="text-slate-500 text-xs w-8">255</span>
                    </div>
                    <div className="flex justify-between mt-2">
                      <div className="flex gap-1">
                        {[0, 64, 128, 192, 255].map(preset => (
                          <button
                            key={preset}
                            onClick={() => {
                              setDmxSliders(prev => prev.map((s, i) => i === index ? { ...s, value: preset } : s));
                              apiService.setDMXChannel(slider.address, preset);
                            }}
                            className="px-2 py-0.5 text-xs rounded bg-slate-700 hover:bg-slate-600 text-slate-300 transition-colors"
                          >
                            {preset}
                          </button>
                        ))}
                      </div>
                    </div>
                  </div>
                ))}
              </div>
              <div className="mt-6 flex gap-2">
                <Button
                  variant="danger"
                  onClick={() => {
                    const reset = dmxSliders.map(s => ({ ...s, value: 0 }));
                    setDmxSliders(reset);
                    reset.forEach(s => apiService.setDMXChannel(s.address, 0));
                  }}
                  className="flex items-center gap-2"
                >
                  Tout à 0
                </Button>
                <Button
                  variant="secondary"
                  onClick={() => {
                    const full = dmxSliders.map(s => ({ ...s, value: 255 }));
                    setDmxSliders(full);
                    full.forEach(s => apiService.setDMXChannel(s.address, 255));
                  }}
                  className="flex items-center gap-2"
                >
                  Tout à 255
                </Button>
              </div>
            </Card>
          </div>
        )}

        {/* ═══ Audio Tab ═══ */}
        {activeTab === 'audio' && (
          <div className="space-y-6">
            <Card>
              <h2 className="text-2xl font-bold text-white mb-6 flex items-center gap-2">
                <Volume2 /> 🎤 Micro INMP441 — VU Mètre
              </h2>

              {/* Connection status */}
              <div className={`flex items-center gap-2 mb-6 px-4 py-2 rounded-lg border ${
                audioData && (audioData.diag.rawSpan > 100)
                  ? 'bg-green-900/30 border-green-700 text-green-400'
                  : 'bg-red-900/30 border-red-700 text-red-400'
              }`}>
                {audioData && audioData.diag.rawSpan > 100 ? (
                  <><Mic size={20} /> Micro détecté — Signal actif</>
                ) : (
                  <><MicOff size={20} /> Aucun signal détecté — Vérifiez le câblage</>
                )}
              </div>

              {/* VU Meter bars */}
              <div className="space-y-4 mb-8">
                {/* Volume */}
                <div>
                  <div className="flex justify-between text-sm mb-1">
                    <span className="text-slate-400">Volume</span>
                    <span className="font-mono text-white font-bold">{audioData?.volume ?? 0}%</span>
                  </div>
                  <div className="h-6 bg-slate-800 rounded-full overflow-hidden border border-slate-700">
                    <div
                      className="h-full rounded-full transition-all duration-150"
                      style={{
                        width: `${audioData?.volume ?? 0}%`,
                        background: `linear-gradient(90deg, #22c55e ${0}%, #eab308 ${50}%, #ef4444 ${85}%)`,
                      }}
                    />
                  </div>
                </div>
                {/* Peak */}
                <div>
                  <div className="flex justify-between text-sm mb-1">
                    <span className="text-slate-400">Peak</span>
                    <span className="font-mono text-yellow-400 font-bold">{audioData?.peak ?? 0}%</span>
                  </div>
                  <div className="h-4 bg-slate-800 rounded-full overflow-hidden border border-slate-700">
                    <div
                      className="h-full bg-yellow-500 rounded-full transition-all duration-100"
                      style={{ width: `${audioData?.peak ?? 0}%` }}
                    />
                  </div>
                </div>
                {/* Bass / Mid / High */}
                <div className="grid grid-cols-3 gap-4">
                  {[{ label: 'Bass', key: 'bass' as const, color: '#ef4444' },
                    { label: 'Mid', key: 'mid' as const, color: '#22c55e' },
                    { label: 'High', key: 'high' as const, color: '#3b82f6' }
                  ].map(band => (
                    <div key={band.key}>
                      <div className="flex justify-between text-xs mb-1">
                        <span className="text-slate-400">{band.label}</span>
                        <span className="font-mono text-white">{audioData?.[band.key] ?? 0}%</span>
                      </div>
                      <div className="h-3 bg-slate-800 rounded-full overflow-hidden border border-slate-700">
                        <div
                          className="h-full rounded-full transition-all duration-150"
                          style={{ width: `${audioData?.[band.key] ?? 0}%`, backgroundColor: band.color }}
                        />
                      </div>
                    </div>
                  ))}
                </div>
                {/* Beat indicator */}
                <div className="flex items-center gap-3">
                  <span className="text-sm text-slate-400">Beat:</span>
                  <div className={`w-6 h-6 rounded-full transition-all duration-100 ${
                    audioData?.beat ? 'bg-orange-500 scale-125 shadow-lg shadow-orange-500/50' : 'bg-slate-700 scale-100'
                  }`} />
                </div>
              </div>

              {/* Volume history waveform */}
              <div className="mb-8">
                <h3 className="text-sm font-medium text-slate-300 mb-2">Historique Volume</h3>
                <div className="h-20 bg-slate-900 rounded-lg border border-slate-700 flex items-end gap-px p-1 overflow-hidden">
                  {audioHistory.map((v, i) => (
                    <div
                      key={i}
                      className="flex-1 min-w-[2px] rounded-t"
                      style={{
                        height: `${Math.max(2, v)}%`,
                        backgroundColor: v > 80 ? '#ef4444' : v > 50 ? '#eab308' : '#22c55e',
                        transition: 'height 0.1s',
                      }}
                    />
                  ))}
                </div>
              </div>

              {/* Sound mode selector */}
              <div className="mb-6">
                <h3 className="text-sm font-medium text-slate-300 mb-3">Mode Son Réactif</h3>
                <div className="grid grid-cols-2 md:grid-cols-5 gap-2">
                  {[
                    { mode: 0, label: 'OFF', icon: '🔇', desc: 'Désactivé' },
                    { mode: 1, label: 'Volume', icon: '📊', desc: 'Luminosité suit le volume' },
                    { mode: 2, label: 'Beat', icon: '💥', desc: 'Flash sur les beats' },
                    { mode: 3, label: 'Couleur', icon: '🌈', desc: 'Bass=R Mid=G High=B' },
                    { mode: 4, label: 'VU', icon: '📈', desc: 'Vert → Jaune → Rouge' },
                  ].map(m => (
                    <button
                      key={m.mode}
                      onClick={() => handleSetSoundMode(m.mode)}
                      className={`p-3 rounded-lg border text-left transition-all ${
                        audioData?.mode === m.mode
                          ? 'bg-purple-900/50 border-purple-500 text-white'
                          : 'bg-slate-800 border-slate-700 text-slate-400 hover:bg-slate-700'
                      }`}
                    >
                      <div className="text-lg mb-1">{m.icon}</div>
                      <div className="font-semibold text-sm">{m.label}</div>
                      <div className="text-xs text-slate-500 mt-1">{m.desc}</div>
                    </button>
                  ))}
                </div>
              </div>

              {/* Sensitivity */}
              <div className="mb-8">
                <div className="flex items-center justify-between mb-2">
                  <span className="text-sm text-slate-300">Sensibilité</span>
                  <span className="text-sm font-mono text-purple-400 font-bold">{audioData?.sensitivity ?? 5}</span>
                </div>
                <input
                  type="range"
                  min={1}
                  max={10}
                  value={audioData?.sensitivity ?? 5}
                  onChange={(e) => handleSetSensitivity(parseInt(e.target.value))}
                  className="w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-purple-500"
                />
                <div className="flex justify-between text-xs text-slate-600 mt-1">
                  <span>1 (faible)</span>
                  <span>10 (max)</span>
                </div>
              </div>

              {/* Raw diagnostic */}
              <div className="p-4 bg-slate-900 rounded-lg border border-slate-700">
                <h3 className="text-sm font-medium text-slate-300 mb-3">🔧 Diagnostic Micro (données brutes)</h3>
                <div className="grid grid-cols-2 md:grid-cols-4 gap-4 font-mono text-sm">
                  <div>
                    <div className="text-slate-500 text-xs">Raw Min</div>
                    <div className="text-white">{audioData?.diag?.rawMin ?? '—'}</div>
                  </div>
                  <div>
                    <div className="text-slate-500 text-xs">Raw Max</div>
                    <div className="text-white">{audioData?.diag?.rawMax ?? '—'}</div>
                  </div>
                  <div>
                    <div className="text-slate-500 text-xs">Raw Span</div>
                    <div className={`font-bold ${
                      (audioData?.diag?.rawSpan ?? 0) > 1000 ? 'text-green-400' :
                      (audioData?.diag?.rawSpan ?? 0) > 100 ? 'text-yellow-400' : 'text-red-400'
                    }`}>{audioData?.diag?.rawSpan ?? '—'}</div>
                  </div>
                  <div>
                    <div className="text-slate-500 text-xs">Raw RMS (x10000)</div>
                    <div className="text-white">{audioData?.diag?.rawRms ?? '—'}</div>
                  </div>
                </div>
                <div className="mt-4 text-xs text-slate-600">
                  Si Raw Span reste proche de 0, le micro ne capte rien. Vérifiez le câblage I2S.
                </div>
              </div>
            </Card>

            {/* Pin reference */}
            <Card>
              <h3 className="text-lg font-bold text-white mb-4">📌 Câblage INMP441</h3>
              <div className="grid grid-cols-2 md:grid-cols-3 gap-3">
                {[
                  { pin: 'VDD', esp: '3.3V', color: 'text-red-400' },
                  { pin: 'GND', esp: 'GND', color: 'text-slate-400' },
                  { pin: 'WS (LRCLK)', esp: 'GPIO 25', color: 'text-yellow-400' },
                  { pin: 'SCK (BCLK)', esp: 'GPIO 26', color: 'text-green-400' },
                  { pin: 'SD (DOUT)', esp: 'GPIO 14', color: 'text-blue-400' },
                  { pin: 'L/R', esp: 'GND (canal gauche)', color: 'text-purple-400' },
                ].map(p => (
                  <div key={p.pin} className="bg-slate-800 rounded p-3 border border-slate-700">
                    <div className="text-xs text-slate-500">{p.pin}</div>
                    <div className={`font-mono font-bold ${p.color}`}>{p.esp}</div>
                  </div>
                ))}
              </div>
              <Alert variant="info" className="mt-4">
                Le pin L/R de l'INMP441 doit être connecté à GND pour le canal gauche (configuration actuelle du firmware).
              </Alert>
            </Card>
          </div>
        )}

        {/* ═══ Settings Tab ═══ */}
        {activeTab === 'settings' && (
          <div className="space-y-6">
            <Card>
              <h2 className="text-2xl font-bold text-white mb-6 flex items-center gap-2">
                <Settings /> Paramètres Système
              </h2>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
                <Input
                  label="SSID WiFi"
                  value={config.wifiSSID || ''}
                  onChange={(e) => setConfig({ ...config, wifiSSID: e.target.value })}
                />
                <Input
                  label="Mot de Passe WiFi"
                  type="password"
                  value={config.wifiPassword || ''}
                  onChange={(e) => setConfig({ ...config, wifiPassword: e.target.value })}
                />
                <Input
                  label="PIN Admin"
                  type="password"
                  value={config.adminPin || ''}
                  onChange={(e) => setConfig({ ...config, adminPin: e.target.value })}
                />
                <Input
                  label="Débit DMX (Baud)"
                  type="number"
                  value={config.dmxBaud || 250000}
                  onChange={(e) => setConfig({ ...config, dmxBaud: parseInt(e.target.value) })}
                />
              </div>

              <div className="flex gap-2">
                <Button variant="primary" onClick={handleSaveConfig} className="flex-1 flex items-center justify-center gap-2">
                  <Save size={20} /> Enregistrer les Paramètres
                </Button>
                <Button variant="danger" size="md">
                  Redémarrer le Système
                </Button>
              </div>
            </Card>

            <Alert variant="info">
              Toutes les modifications sont automatiquement sauvegardées dans la mémoire flash de l'ESP32. Le système redémarrera après certaines modifications.
            </Alert>
          </div>
        )}
      </div>
    </div>
  );
};
