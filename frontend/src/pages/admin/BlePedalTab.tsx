import { useState, useEffect } from 'react';
import { Bluetooth, Save, RefreshCw } from 'lucide-react';
import { Button, Card } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { PedalAction, BlePedalActionDef } from '@/types';

const ACTION_LABELS: Record<PedalAction, string> = {
  none: '❌ Désactivé ❌',
  smoke: '💨 Machine à fumée',
  strobe: '⚡ Strobe',
  scene: '🎭 Activer une scène',
  show: '🎬 Lancer / Stopper un show',
  scene_sequence: '⏭️ Séquence : Scène suivante (Toutes)',
  scene_sequence_g1: '⏭️ Séquence : Scène suivante (Groupe 1)',
  scene_sequence_g2: '⏭️ Séquence : Scène suivante (Groupe 2)',
  show_sequence: '⏭️ Séquence : Show suivant',
  sound_volume: '🔊 Son : Volume',
  sound_beat: '🥁 Son : Beat',
  sound_color: '🎨 Son : Couleur',
  sound_vu: '📊 Son : VU-Mètre',
  sound_scene_g1: '🎵 Son : Scène Suivante (Groupe 1)',
  sound_scene_g2: '🎵 Son : Scène Suivante (Groupe 2)',
  sound_scene_seq: '🎵 Son : Scène Suivante (Tous Groupes)',
  sound_scene_rnd: '🎵 Son : Scène Aléatoire',
};

const ACTION_GROUPS = [
  { label: 'Effets', actions: ['none', 'smoke', 'strobe'] as PedalAction[] },
  { label: 'Scènes & Shows', actions: ['scene', 'show', 'scene_sequence', 'scene_sequence_g1', 'scene_sequence_g2', 'show_sequence'] as PedalAction[] },
  { label: 'Audio Réactif', actions: ['sound_volume', 'sound_beat', 'sound_color', 'sound_vu', 'sound_scene_g1', 'sound_scene_g2', 'sound_scene_seq', 'sound_scene_rnd'] as PedalAction[] },
];

export const BlePedalTab = () => {
  const store = useAppStore();
  const [pedalConfig, setPedalConfig] = useState<BlePedalActionDef[]>([]);
  const [saving, setSaving] = useState(false);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadPedalConfig();
  }, []);

  const loadPedalConfig = async () => {
    setLoading(true);
    try {
      const config = await apiService.getBlePedalConfig();
      if (config && config.length === 16) {
        setPedalConfig(config);
      } else {
        setPedalConfig(Array(16).fill(null).map((_, i) => ({ ccId: i, action: 'none', param: '' })));
      }
    } catch (e: any) {
      store.setError('Erreur chargement config BLE: ' + e.message);
      setPedalConfig(Array(16).fill(null).map((_, i) => ({ ccId: i, action: 'none', param: '' })));
    } finally {
      setLoading(false);
    }
  };

  const handleSaveAll = async () => {
    setSaving(true);
    try {
      await apiService.saveBlePedalConfig(pedalConfig);
      store.setSuccess('Configuration pédalier BLE enregistrée !');
    } catch (e: any) {
      store.setError('Erreur sauvegarde: ' + e.message);
    } finally {
      setSaving(false);
    }
  };

  const handleActionChange = (index: number, action: string) => {
    const newConfig = [...pedalConfig];
    newConfig[index] = { ...newConfig[index], action, param: '' };
    setPedalConfig(newConfig);
  };

  const handleTargetChange = (index: number, param: string) => {
    const newConfig = [...pedalConfig];
    newConfig[index] = { ...newConfig[index], param };
    setPedalConfig(newConfig);
  };

  const scenes = store.activeSetup?.scenes || [];
  const shows = store.activeSetup?.shows || [];

  const needsTarget = (action: string) => action === 'scene' || action === 'show';

  if (loading) {
    return (
      <div className="flex items-center justify-center py-16">
        <RefreshCw className="animate-spin text-purple-500" size={32} />
      </div>
    );
  }

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-2xl font-bold text-white flex items-center gap-3">
            <Bluetooth className="text-blue-500" size={28} />
            Pédalier Bluetooth (BLE MIDI)
          </h2>
          <p className="text-sm text-slate-400 mt-1">
            Configurez les 16 entrées (Control Change 1 à 16) de votre pédalier BLE MIDI.
            La configuration est liée au setup actif.
          </p>
        </div>
        <Button 
          variant="primary" 
          onClick={handleSaveAll} 
          disabled={saving}
          className="flex items-center gap-2"
        >
          <Save size={18} />
          {saving ? 'Enregistrement...' : 'Tout enregistrer'}
        </Button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        {pedalConfig.map((cfg, idx) => (
          <Card key={idx} className="border border-slate-700/50 bg-slate-800/50 transition-all">
            <div className="flex flex-col gap-3">
                <div className="flex items-center justify-between mb-2">
                <span className="text-sm font-semibold text-slate-300">Slot {idx + 1} (CC {idx})</span>
              </div>
              
              <div className="flex-1 space-y-3">
                <div className="flex flex-col gap-1">
                  <label className="text-xs font-medium text-slate-400 uppercase tracking-wider">Action déclenchée</label>
                  <select 
                    className="w-full bg-slate-900 border border-slate-700 rounded-md px-3 py-2 text-sm text-white focus:outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500"
                    value={cfg.action}
                    onChange={(e) => handleActionChange(idx, e.target.value)}
                  >
                    {ACTION_GROUPS.map((group, i) => (
                      <optgroup key={i} label={group.label}>
                        {group.actions.map(a => (
                          <option key={a} value={a}>{ACTION_LABELS[a]}</option>
                        ))}
                      </optgroup>
                    ))}
                  </select>
                </div>

                {needsTarget(cfg.action) && (
                  <div className="flex flex-col gap-1 animate-in fade-in slide-in-from-top-1">
                    <label className="text-xs font-medium text-slate-400 uppercase tracking-wider">Cible ({cfg.action === 'scene' ? 'Scène' : 'Show'})</label>
                    <select
                      className="w-full bg-slate-900 border border-slate-700 rounded-md px-3 py-2 text-sm text-white focus:outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500"
                      value={cfg.param}
                      onChange={(e) => handleTargetChange(idx, e.target.value)}
                    >
                      <option value="" disabled>-- Sélectionner une {cfg.action === 'scene' ? 'scène' : 'show'} --</option>
                      {cfg.action === 'scene' ? (
                        scenes.map(s => <option key={s.id} value={s.id}>{s.name}</option>)
                      ) : (
                        shows.map(s => <option key={s.id} value={s.id}>{s.name}</option>)
                      )}
                    </select>
                  </div>
                )}
              </div>
            </div>
          </Card>
        ))}
      </div>
    </div>
  );
};
