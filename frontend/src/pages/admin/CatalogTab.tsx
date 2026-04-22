import { useState } from 'react';
import { Plus, Trash2, Save, Edit2 } from 'lucide-react';
import { Button, Card, Input, Select } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { FixtureProfile, ChannelDefinition, ChannelType } from '@/types';

const FIXTURE_PRESETS: Record<string, ChannelDefinition[]> = {
  dimmer: [{ name: 'Dimmer', offset: 0, defaultValue: 0, type: 'dimmer' }],
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
  moving_head: [
    { name: 'Pan', offset: 0, defaultValue: 128, type: 'position' },
    { name: 'Tilt', offset: 1, defaultValue: 128, type: 'position' },
    { name: 'Dimmer', offset: 2, defaultValue: 0, type: 'dimmer' },
    { name: 'Red', offset: 3, defaultValue: 0, type: 'color' },
    { name: 'Green', offset: 4, defaultValue: 0, type: 'color' },
    { name: 'Blue', offset: 5, defaultValue: 0, type: 'color' },
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

export const CatalogTab = () => {
  const store = useAppStore();
  const [editingProfile, setEditingProfile] = useState<FixtureProfile | null>(null);

  const handleNewProfile = () => {
    setEditingProfile({
      id: 'prof-' + Date.now(),
      name: 'Nouveau Modèle',
      type: 'rgb',
      channelCount: 3,
      channels: [
        { name: 'Red', offset: 0, defaultValue: 0, type: 'color' },
        { name: 'Green', offset: 1, defaultValue: 0, type: 'color' },
        { name: 'Blue', offset: 2, defaultValue: 0, type: 'color' },
      ],
      strobeChannel: { enabled: false, offset: 0, value: 255 },
    });
  };

  const saveProfile = async () => {
    if (!editingProfile) return;
    try {
      await apiService.saveProfile(editingProfile);
      const profiles = await apiService.getProfiles();
      store.setProfiles(profiles);
      setEditingProfile(null);
    } catch (e: any) {
      store.setError(e.message || 'Erreur sauvegarde profil');
    }
  };

  const deleteProfile = async (id: string) => {
    if (!confirm('Supprimer ce modèle ?')) return;
    try {
      await apiService.deleteProfile(id);
      const profiles = await apiService.getProfiles();
      store.setProfiles(profiles);
    } catch (e: any) {
      store.setError(e.message || 'Erreur suppression profil');
    }
  };

  const setEditing = (prof: FixtureProfile) => setEditingProfile(prof);

  const applyPreset = (preset: string) => {
    if (!editingProfile) return;
    const channels = FIXTURE_PRESETS[preset];
    if (channels) {
      const channelCount = channels.length > 0 ? Math.max(...channels.map(c => c.offset)) + 1 : 1;
      setEditingProfile({
        ...editingProfile,
        type: preset,
        channels: channels.map(ch => ({ ...ch })),
        channelCount,
      });
    }
  };

  const updateChannel = (idx: number, updates: Partial<ChannelDefinition>) => {
    if (!editingProfile) return;
    const newChannels = [...editingProfile.channels];
    newChannels[idx] = { ...newChannels[idx], ...updates };
    const cc = newChannels.length > 0 ? Math.max(...newChannels.map(c => c.offset)) + 1 : 1;
    setEditingProfile({ ...editingProfile, channels: newChannels, channelCount: cc });
  };

  const addChannel = () => {
    if (!editingProfile) return;
    const nextOffset = editingProfile.channels.length > 0
      ? Math.max(...editingProfile.channels.map(c => c.offset)) + 1
      : 0;
    setEditingProfile({
      ...editingProfile,
      channels: [...editingProfile.channels, { name: 'Ch ' + (editingProfile.channels.length + 1), offset: nextOffset, defaultValue: 0, type: 'other' }],
      channelCount: Math.max(editingProfile.channelCount, nextOffset + 1),
    });
  };

  const removeChannel = (idx: number) => {
    if (!editingProfile) return;
    const newChannels = editingProfile.channels.filter((_, i) => i !== idx);
    const cc = newChannels.length > 0 ? Math.max(...newChannels.map(c => c.offset)) + 1 : 1;
    setEditingProfile({ ...editingProfile, channels: newChannels, channelCount: cc });
  };

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Catalogue de Projecteurs</h2>
        <Button onClick={handleNewProfile} className="flex items-center gap-2">
          <Plus size={20} /> Nouveau Modèle
        </Button>
      </div>

      {editingProfile && (
        <Card className="border border-purple-500">
          <h3 className="text-lg font-bold text-white mb-4">
            {editingProfile.id.startsWith('prof-') ? 'Nouveau Modèle' : 'Éditer Modèle'}
          </h3>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
            <Input
              label="Nom du Modèle"
              value={editingProfile.name}
              onChange={(e) => setEditingProfile({ ...editingProfile, name: e.target.value })}
            />
            <Select
              label="Charger un Préréglage"
              value={editingProfile.type}
              onChange={(e) => applyPreset(e.target.value)}
              options={[
                { value: '', label: '— Choisir un préréglage —' },
                { value: 'dimmer', label: 'Variateur (1ch)' },
                { value: 'rgb', label: 'RGB (3ch)' },
                { value: 'rgbw', label: 'RGBW (4ch)' },
                { value: 'moving_head', label: 'Lyre' },
              ]}
            />
            <Input
              label="Nombre de Canaux DMX"
              type="number"
              min={1}
              value={editingProfile.channelCount}
              onChange={(e) => setEditingProfile({ ...editingProfile, channelCount: parseInt(e.target.value) || 1 })}
            />
          </div>

          <div className="mb-4">
            <div className="flex items-center justify-between mb-2">
              <label className="text-sm font-medium text-slate-300">Canaux</label>
              <Button variant="outline" size="sm" onClick={addChannel} className="flex items-center gap-1">
                <Plus size={14} /> Ajouter un Canal
              </Button>
            </div>
            <div className="space-y-2 max-h-80 overflow-y-auto pr-1">
              {editingProfile.channels.map((ch, idx) => (
                <div key={idx} className="flex items-center gap-2 bg-slate-900 rounded p-2">
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
          
          <div className="mb-4 p-3 bg-slate-900 rounded border border-slate-700">
            <label className="flex items-center gap-2 text-sm text-slate-300 mb-2">
              <input
                type="checkbox"
                checked={editingProfile.strobeChannel?.enabled ?? false}
                onChange={(e) => setEditingProfile({ ...editingProfile, strobeChannel: { offset: editingProfile.strobeChannel?.offset ?? 0, value: editingProfile.strobeChannel?.value ?? 255, enabled: e.target.checked } })}
                className="accent-yellow-500"
              />
              Canal Stroboscope
            </label>
            {editingProfile.strobeChannel?.enabled && (
              <div className="flex items-center gap-4 mt-2">
                <Input
                  label="Offset DMX"
                  type="number"
                  min={0}
                  max={511}
                  value={editingProfile.strobeChannel.offset}
                  onChange={(e) => setEditingProfile({ ...editingProfile, strobeChannel: { ...editingProfile.strobeChannel!, offset: parseInt(e.target.value) || 0 } })}
                  className="w-24"
                />
                <Input
                  label="Valeur (0-255)"
                  type="number"
                  min={0}
                  max={255}
                  value={editingProfile.strobeChannel.value}
                  onChange={(e) => setEditingProfile({ ...editingProfile, strobeChannel: { ...editingProfile.strobeChannel!, value: parseInt(e.target.value) || 0 } })}
                  className="w-24"
                />
              </div>
            )}
          </div>

          <div className="flex gap-2">
            <Button variant="primary" onClick={saveProfile} className="flex-1 flex items-center justify-center gap-2">
              <Save size={20} /> Enregistrer le Modèle
            </Button>
            <Button variant="outline" onClick={() => setEditingProfile(null)} className="flex-1">
              Annuler
            </Button>
          </div>
        </Card>
      )}

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {store.profiles.map(prof => (
          <Card key={prof.id} className="flex flex-col">
            <div className="flex justify-between items-start mb-4">
              <div>
                <h3 className="text-xl font-bold text-white">{prof.name}</h3>
                <span className="text-sm text-slate-400 bg-slate-800 px-2 py-1 rounded inline-block mt-1">
                  Type: {prof.type}
                </span>
              </div>
              <div className="flex gap-2">
                <button
                  onClick={() => setEditing(prof)}
                  className="text-slate-400 hover:text-white transition-colors"
                >
                  <Edit2 size={20} />
                </button>
                <button
                  onClick={() => deleteProfile(prof.id)}
                  className="text-red-400 hover:text-red-300 transition-colors"
                >
                  <Trash2 size={20} />
                </button>
              </div>
            </div>
            <div className="flex items-center text-slate-300 text-sm mb-4">
              <span>{prof.channelCount} Canaux DMX</span>
            </div>
            <div className="flex flex-wrap gap-1 mt-auto">
              {prof.channels.map(ch => (
                <span key={ch.name} className="text-xs bg-slate-900 border border-slate-700 text-slate-300 px-1.5 py-0.5 rounded">
                  {ch.name}
                </span>
              ))}
            </div>
          </Card>
        ))}
        {store.profiles.length === 0 && (
          <div className="col-span-full p-8 text-center bg-slate-800/50 rounded-lg border border-slate-700/50">
             <p className="text-slate-400">Aucun modèle dans le catalogue. Cliquez sur "Nouveau Modèle" pour commencer.</p>
          </div>
        )}
      </div>
    </div>
  );
};
