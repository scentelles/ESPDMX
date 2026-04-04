import { useEffect, useState } from 'react';
import { Settings, Plus, Trash2, Save, LogOut, GripVertical, ChevronDown, ChevronUp } from 'lucide-react';
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
  { value: 'dimmer', label: 'Dimmer' },
  { value: 'color', label: 'Color' },
  { value: 'position', label: 'Position' },
  { value: 'speed', label: 'Speed' },
  { value: 'gobo', label: 'Gobo' },
  { value: 'other', label: 'Other' },
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
  const applyPreset = (preset: string) => {
    const channels = FIXTURE_PRESETS[preset];
    if (channels) {
      onChange({
        ...fixture,
        type: preset,
        channels: channels.map(ch => ({ ...ch })),
        channelCount: channels.length,
      });
    }
  };

  const updateChannel = (idx: number, updates: Partial<ChannelDefinition>) => {
    const newChannels = [...fixture.channels];
    newChannels[idx] = { ...newChannels[idx], ...updates };
    onChange({ ...fixture, channels: newChannels, channelCount: newChannels.length });
  };

  const addChannel = () => {
    const nextOffset = fixture.channels.length > 0
      ? Math.max(...fixture.channels.map(c => c.offset)) + 1
      : 0;
    onChange({
      ...fixture,
      channels: [...fixture.channels, { name: 'Channel ' + (fixture.channels.length + 1), offset: nextOffset, defaultValue: 0, type: 'other' as ChannelType }],
      channelCount: fixture.channels.length + 1,
    });
  };

  const removeChannel = (idx: number) => {
    const newChannels = fixture.channels.filter((_, i) => i !== idx);
    onChange({ ...fixture, channels: newChannels, channelCount: newChannels.length });
  };

  return (
    <div className="mb-6 p-4 bg-slate-800 rounded-lg border border-slate-700">
      <h3 className="text-lg font-bold text-white mb-4">
        {fixture.id.startsWith('new-') ? 'New Fixture' : 'Edit Fixture'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Fixture Name"
          value={fixture.name}
          onChange={(e) => onChange({ ...fixture, name: e.target.value })}
        />
        <Input
          label="DMX Start Address"
          type="number"
          min={1}
          max={512}
          value={fixture.dmxAddress}
          onChange={(e) => onChange({ ...fixture, dmxAddress: parseInt(e.target.value) || 1 })}
        />
        <Select
          label="Load Preset"
          value={fixture.type}
          onChange={(e) => applyPreset(e.target.value)}
          options={[
            { value: '', label: '— Select preset —' },
            { value: 'dimmer', label: 'Dimmer (1ch)' },
            { value: 'rgb', label: 'RGB (3ch)' },
            { value: 'rgbw', label: 'RGBW (4ch)' },
            { value: 'rgbwd', label: 'RGBW+Dimmer (5ch)' },
            { value: 'moving_head', label: 'Moving Head (8ch)' },
          ]}
        />
      </div>

      {/* Channel definitions */}
      <div className="mb-4">
        <div className="flex items-center justify-between mb-2">
          <label className="text-sm font-medium text-slate-300">
            Channels ({fixture.channels.length}) — DMX {fixture.dmxAddress}–{fixture.dmxAddress + fixture.channels.length - 1}
          </label>
          <Button variant="outline" size="sm" onClick={addChannel} className="flex items-center gap-1">
            <Plus size={14} /> Add Channel
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
                max={255}
                value={ch.defaultValue}
                onChange={(e) => updateChannel(idx, { defaultValue: parseInt(e.target.value) || 0 })}
                className="w-20 !py-1 text-sm"
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
          Enabled
        </label>
      </div>

      <div className="flex gap-2">
        <Button variant="primary" onClick={onSave} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Save Fixture
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Cancel
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
}

const SceneEditor: React.FC<SceneEditorProps> = ({ scene, fixtures, onChange, onSave, onCancel }) => {
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
        {scene.id.startsWith('new-') ? 'New Scene' : 'Edit Scene'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Scene Name"
          value={scene.name}
          onChange={(e) => onChange({ ...scene, name: e.target.value })}
        />
        <Input
          label="Icon (emoji)"
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
        <label className="text-sm font-medium text-slate-300 mb-2 block">Fixture Values</label>
        {fixtures.length === 0 && (
          <Alert variant="warning">No fixtures configured. Add fixtures first in the Fixtures tab.</Alert>
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
          <Save size={20} /> Save Scene
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Cancel
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
}

const ShowEditor: React.FC<ShowEditorProps> = ({ show, scenes, onChange, onSave, onCancel }) => {
  const addStep = () => {
    if (scenes.length === 0) return;
    onChange({
      ...show,
      steps: [...show.steps, { sceneId: scenes[0].id, duration: 5000, transitionTime: 1000 }],
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
        {show.id.startsWith('new-') ? 'New Show' : 'Edit Show'}
      </h3>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
        <Input
          label="Show Name"
          value={show.name}
          onChange={(e) => onChange({ ...show, name: e.target.value })}
        />
        <Input
          label="Icon (emoji)"
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
        Loop show continuously
      </label>

      {/* Scene steps */}
      <div className="mb-4">
        <div className="flex items-center justify-between mb-2">
          <label className="text-sm font-medium text-slate-300">
            Scene Steps ({show.steps.length}) — Total: {(totalDuration / 1000).toFixed(1)}s
          </label>
          <Button variant="outline" size="sm" onClick={addStep} disabled={scenes.length === 0} className="flex items-center gap-1">
            <Plus size={14} /> Add Step
          </Button>
        </div>

        {scenes.length === 0 && (
          <Alert variant="warning">No scenes defined. Create scenes first in the Scenes tab.</Alert>
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
                  <label className="text-xs text-slate-500">Hold Duration</label>
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
                  <label className="text-xs text-slate-500">Transition Time</label>
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
            </div>
          ))}
        </div>
      </div>

      <div className="flex gap-2">
        <Button variant="primary" onClick={onSave} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Save Show
        </Button>
        <Button variant="outline" onClick={onCancel} className="flex-1">
          Cancel
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
  const [activeTab, setActiveTab] = useState<'fixtures' | 'scenes' | 'shows' | 'settings'>('fixtures');
  const [editingFixture, setEditingFixture] = useState<DMXFixture | null>(null);
  const [editingScene, setEditingScene] = useState<ColorScene | null>(null);
  const [editingShow, setEditingShow] = useState<DynamicShow | null>(null);
  const [config, setConfig] = useState<Partial<SystemConfig>>({});

  useEffect(() => {
    loadAdminData();
  }, []);

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
      store.setError('Failed to load admin data');
      console.error(error);
      setLoading(false);
    }
  };

  // ── Fixture CRUD ────────────────────────────────────────────────────
  const handleNewFixture = () => {
    setEditingFixture({
      id: 'new-' + Date.now(),
      name: 'New Fixture',
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
      const message = error instanceof Error ? error.message : 'Failed to save fixture';
      store.setError(message);
    }
  };

  const handleDeleteFixture = async (id: string) => {
    if (!confirm('Delete this fixture? Scenes using it will lose those values.')) return;
    try {
      await apiService.deleteFixture(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete fixture';
      store.setError(message);
    }
  };

  // ── Scene CRUD ──────────────────────────────────────────────────────
  const handleNewScene = () => {
    setEditingScene({
      id: 'new-' + Date.now(),
      name: 'New Scene',
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
      const message = error instanceof Error ? error.message : 'Failed to save scene';
      store.setError(message);
    }
  };

  const handleDeleteScene = async (id: string) => {
    if (!confirm('Delete this scene? Shows referencing it will lose those steps.')) return;
    try {
      await apiService.deleteScene(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete scene';
      store.setError(message);
    }
  };

  // ── Show CRUD ───────────────────────────────────────────────────────
  const handleNewShow = () => {
    setEditingShow({
      id: 'new-' + Date.now(),
      name: 'New Show',
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
      const message = error instanceof Error ? error.message : 'Failed to save show';
      store.setError(message);
    }
  };

  const handleDeleteShow = async (id: string) => {
    if (!confirm('Delete this show?')) return;
    try {
      await apiService.deleteShow(id);
      await loadAdminData();
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to delete show';
      store.setError(message);
    }
  };

  // ── Config ──────────────────────────────────────────────────────────
  const handleSaveConfig = async () => {
    try {
      await apiService.saveSystemConfig(config);
      store.setError(null);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Failed to save configuration';
      store.setError(message);
    }
  };

  // ── Helpers ─────────────────────────────────────────────────────────
  const sceneSummary = (scene: ColorScene): string => {
    const count = scene.fixtureValues?.length || 0;
    return count === 0 ? 'No fixtures' : `${count} fixture${count > 1 ? 's' : ''}`;
  };

  const showSummary = (show: DynamicShow): string => {
    const steps = show.steps || [];
    const total = steps.reduce((s, step) => s + step.duration + step.transitionTime, 0);
    return `${steps.length} step${steps.length !== 1 ? 's' : ''} • ${(total / 1000).toFixed(1)}s${show.loop ? ' • loop' : ''}`;
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
          <h1 className="text-4xl md:text-5xl font-bold text-white mb-2">⚙️ Admin Panel</h1>
          <p className="text-slate-400">Configure your DMX lighting system</p>
        </div>
        <Button variant="outline" onClick={onLogout} className="flex items-center gap-2">
          <LogOut size={20} /> Logout
        </Button>
      </div>

      <div className="max-w-7xl mx-auto">
        {/* Navigation Tabs */}
        <div className="flex gap-2 mb-8 overflow-x-auto pb-2">
          {(['fixtures', 'scenes', 'shows', 'settings'] as const).map((tab) => (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              className={`px-4 py-2 rounded-lg font-semibold transition-all whitespace-nowrap ${
                activeTab === tab
                  ? 'bg-purple-600 text-white'
                  : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
              }`}
            >
              {tab.charAt(0).toUpperCase() + tab.slice(1)}
            </button>
          ))}
        </div>

        {/* ═══ Fixtures Tab ═══ */}
        {activeTab === 'fixtures' && (
          <div className="space-y-6">
            <Card>
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-2xl font-bold text-white flex items-center gap-2">
                  <Settings size={24} /> DMX Fixtures
                </h2>
                <Button variant="secondary" onClick={handleNewFixture} className="flex items-center gap-2">
                  <Plus size={20} /> Add Fixture
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
                  No fixtures configured yet. Click "Add Fixture" to get started.
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
                        {!fixture.enabled && <span className="text-red-400 ml-2">(disabled)</span>}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => setEditingFixture(fixture)}>
                        Edit
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
                <h2 className="text-2xl font-bold text-white">🎨 Color Scenes</h2>
                <Button variant="secondary" onClick={handleNewScene} className="flex items-center gap-2">
                  <Plus size={20} /> Add Scene
                </Button>
              </div>

              {editingScene && (
                <SceneEditor
                  scene={editingScene}
                  fixtures={store.fixtures}
                  onChange={setEditingScene}
                  onSave={handleSaveScene}
                  onCancel={() => setEditingScene(null)}
                />
              )}

              {store.scenes.length === 0 && !editingScene && (
                <div className="text-center py-8 text-slate-500">
                  No scenes defined yet. Click "Add Scene" to create one.
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
                        {scene.description || 'No description'}
                        {' • '}{sceneSummary(scene)}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => setEditingScene(scene)}>
                        Edit
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
                <h2 className="text-2xl font-bold text-white">🎆 Dynamic Shows</h2>
                <Button variant="secondary" onClick={handleNewShow} className="flex items-center gap-2">
                  <Plus size={20} /> Add Show
                </Button>
              </div>

              {editingShow && (
                <ShowEditor
                  show={editingShow}
                  scenes={store.scenes}
                  onChange={setEditingShow}
                  onSave={handleSaveShow}
                  onCancel={() => setEditingShow(null)}
                />
              )}

              {store.shows.length === 0 && !editingShow && (
                <div className="text-center py-8 text-slate-500">
                  No shows defined yet. Click "Add Show" to create one.
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
                        {show.description || 'No description'}
                        {' • '}{showSummary(show)}
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" onClick={() => setEditingShow(show)}>
                        Edit
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

        {/* ═══ Settings Tab ═══ */}
        {activeTab === 'settings' && (
          <div className="space-y-6">
            <Card>
              <h2 className="text-2xl font-bold text-white mb-6 flex items-center gap-2">
                <Settings /> System Settings
              </h2>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
                <Input
                  label="WiFi SSID"
                  value={config.wifiSSID || ''}
                  onChange={(e) => setConfig({ ...config, wifiSSID: e.target.value })}
                />
                <Input
                  label="WiFi Password"
                  type="password"
                  value={config.wifiPassword || ''}
                  onChange={(e) => setConfig({ ...config, wifiPassword: e.target.value })}
                />
                <Input
                  label="Admin PIN"
                  type="password"
                  value={config.adminPin || ''}
                  onChange={(e) => setConfig({ ...config, adminPin: e.target.value })}
                />
                <Input
                  label="DMX Baud Rate"
                  type="number"
                  value={config.dmxBaud || 250000}
                  onChange={(e) => setConfig({ ...config, dmxBaud: parseInt(e.target.value) })}
                />
              </div>

              <div className="flex gap-2">
                <Button variant="primary" onClick={handleSaveConfig} className="flex-1 flex items-center justify-center gap-2">
                  <Save size={20} /> Save Settings
                </Button>
                <Button variant="danger" size="md">
                  Reboot System
                </Button>
              </div>
            </Card>

            <Alert variant="info">
              All changes are automatically saved to the ESP32 flash memory. System will restart after some changes.
            </Alert>
          </div>
        )}
      </div>
    </div>
  );
};
