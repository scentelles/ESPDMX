import { useState, useEffect } from 'react';
import { Footprints, Save, RefreshCw } from 'lucide-react';
import { Button, Card } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { PedalAction, PedalButtonConfig } from '@/types';

const ACTION_LABELS: Record<PedalAction, string> = {
  none: '— Désactivé —',
  smoke: '💨 Machine à fumée',
  strobe: '⚡ Strobe',
  scene: '🎨 Activer une scène',
  show: '🎬 Lancer / Stopper un show',
  scene_sequence: '🔄 Séquence : Scène suivante (Toutes)',
  scene_sequence_g1: '🔄 Séquence : Scène suivante (Groupe 1)',
  scene_sequence_g2: '🔄 Séquence : Scène suivante (Groupe 2)',
  show_sequence: '🔄 Séquence : Show suivant',
  sound_volume: '📊 Son : Volume',
  sound_beat: '🥁 Son : Beat',
  sound_color: '🌈 Son : Couleur',
  sound_vu: '📶 Son : VU-Mètre',
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

const BUTTON_LABELS = ['Bouton 1 (D3 seul)', 'Bouton 2 (D4 seul)', 'Bouton 3 (D3 + D4)'];
const BUTTON_COLORS = ['border-rose-500/30 bg-rose-500/5', 'border-amber-500/30 bg-amber-500/5', 'border-violet-500/30 bg-violet-500/5'];
const BUTTON_ICONS = ['🔴', '🟠', '🟣'];

export const PedalConfigTab = () => {
  const store = useAppStore();
  const [pedalConfig, setPedalConfig] = useState<PedalButtonConfig[]>([
    { action: 'none', targetId: '' },
    { action: 'none', targetId: '' },
    { action: 'none', targetId: '' },
  ]);
  const [saving, setSaving] = useState<number | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadPedalConfig();
  }, []);

  const loadPedalConfig = async () => {
    setLoading(true);
    try {
      const config = await apiService.getPedalConfig();
      if (config && config.length === 3) {
        setPedalConfig(config.map(c => ({
          action: (c.action || 'none') as PedalAction,
          targetId: c.targetId || '',
        })));
      }
    } catch (e: any) {
      store.setError('Erreur chargement config pédalier: ' + e.message);
    }
    setLoading(false);
  };

  const handleActionChange = (index: number, action: PedalAction) => {
    const newConfig = [...pedalConfig];
    newConfig[index] = { action, targetId: '' };
    setPedalConfig(newConfig);
  };

  const handleTargetChange = (index: number, targetId: string) => {
    const newConfig = [...pedalConfig];
    newConfig[index] = { ...newConfig[index], targetId };
    setPedalConfig(newConfig);
  };

  const handleSave = async (index: number) => {
    setSaving(index);
    try {
      const cfg = pedalConfig[index];
      await apiService.savePedalConfig(index + 1, cfg.action, cfg.targetId);
      store.setSuccess(`Bouton ${index + 1} configuré : ${ACTION_LABELS[cfg.action]}`);
    } catch (e: any) {
      store.setError('Erreur sauvegarde: ' + e.message);
    }
    setSaving(null);
  };

  const handleSaveAll = async () => {
    setSaving(-1);
    try {
      for (let i = 0; i < 3; i++) {
        const cfg = pedalConfig[i];
        await apiService.savePedalConfig(i + 1, cfg.action, cfg.targetId);
      }
      store.setSuccess('Configuration pédalier enregistrée !');
    } catch (e: any) {
      store.setError('Erreur sauvegarde: ' + e.message);
    }
    setSaving(null);
  };

  const scenes = store.activeSetup?.scenes || [];
  const shows = store.activeSetup?.shows || [];

  const needsTarget = (action: PedalAction) => action === 'scene' || action === 'show';

  const getBehaviorDescription = (action: PedalAction): string => {
    switch (action) {
      case 'smoke': return 'Maintenir appuyé = fumée active, relâcher = arrêt';
      case 'strobe': return 'Maintenir appuyé = strobe actif, relâcher = arrêt';
      case 'scene': return 'Appuyer = applique la scène sélectionnée (désactive l\'audio)';
      case 'show': return 'Appuyer = démarre le show. Ré-appuyer = arrête le show (désactive l\'audio)';
      case 'scene_sequence': return 'Appuyer = passe à la scène suivante du setup (désactive l\'audio)';
      case 'scene_sequence_g1': return 'Appuyer = passe à la scène suivante du Groupe 1 (désactive l\'audio)';
      case 'scene_sequence_g2': return 'Appuyer = passe à la scène suivante du Groupe 2 (désactive l\'audio)';
      case 'show_sequence': return 'Appuyer = passe au show suivant du setup (désactive l\'audio)';
      case 'sound_volume': return 'Toggle : la luminosité suit le volume du micro';
      case 'sound_beat': return 'Toggle : flash synchronisé sur les basses';
      case 'sound_color': return 'Toggle : couleur Bass=R, Mid=V, Aigu=B';
      case 'sound_vu': return 'Toggle : mode VU-Mètre global';
      case 'sound_scene_g1': return 'Toggle : scène suivante (Groupe 1) sur beat';
      case 'sound_scene_g2': return 'Toggle : scène suivante (Groupe 2) sur beat';
      case 'sound_scene_seq': return 'Toggle : scène suivante (Tous) sur beat';
      case 'sound_scene_rnd': return 'Toggle : scène aléatoire (Tous) sur beat';
      default: return 'Aucune action assignée';
    }
  };

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
            <Footprints className="text-purple-500" size={28} />
            Configuration Pédalier
          </h2>
          <p className="text-sm text-slate-400 mt-1">
            Assignez une action à chacun des 3 boutons du pédalier ESP8266.
            La configuration est liée au setup actif.
          </p>
        </div>
        <Button 
          variant="primary" 
          onClick={handleSaveAll} 
          disabled={saving !== null}
          className="flex items-center gap-2"
        >
          <Save size={18} />
          {saving === -1 ? 'Enregistrement...' : 'Tout enregistrer'}
        </Button>
      </div>

      {store.activeSetup && (
        <div className="text-sm text-slate-400 bg-slate-800/50 border border-slate-700 rounded-lg px-4 py-2">
          Setup actif : <span className="text-white font-semibold">{store.activeSetup.name}</span>
          {' · '}
          {scenes.length} scène{scenes.length !== 1 ? 's' : ''}
          {' · '}
          {shows.length} show{shows.length !== 1 ? 's' : ''}
        </div>
      )}

      <div className="grid grid-cols-1 gap-4">
        {pedalConfig.map((cfg, idx) => (
          <Card key={idx} className={`border ${BUTTON_COLORS[idx]} transition-all`}>
            <div className="flex items-start gap-4">
              {/* Button icon */}
              <div className="text-3xl mt-1">{BUTTON_ICONS[idx]}</div>
              
              <div className="flex-1 space-y-3">
                {/* Header */}
                <div className="flex items-center justify-between">
                  <h3 className="text-lg font-bold text-white">{BUTTON_LABELS[idx]}</h3>
                  <Button
                    variant="outline"
                    size="sm"
                    onClick={() => handleSave(idx)}
                    disabled={saving !== null}
                    className="flex items-center gap-1 text-xs"
                  >
                    <Save size={14} />
                    {saving === idx ? '...' : 'Sauver'}
                  </Button>
                </div>

                {/* Action selector */}
                <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
                  <div>
                    <label className="block text-xs font-semibold text-slate-400 uppercase tracking-wider mb-1">
                      Action
                    </label>
                    <select
                      value={cfg.action}
                      onChange={(e) => handleActionChange(idx, e.target.value as PedalAction)}
                      className="w-full bg-slate-800 text-white border border-slate-600 rounded-lg px-3 py-2 text-sm focus:outline-none focus:ring-2 focus:ring-purple-500 focus:border-transparent"
                    >
                      {ACTION_GROUPS.map((group) => (
                        <optgroup key={group.label} label={group.label}>
                          {group.actions.map((value) => (
                            <option key={value} value={value}>{ACTION_LABELS[value]}</option>
                          ))}
                        </optgroup>
                      ))}
                    </select>
                  </div>

                  {/* Target selector (scene or show) */}
                  {needsTarget(cfg.action) && (
                    <div>
                      <label className="block text-xs font-semibold text-slate-400 uppercase tracking-wider mb-1">
                        {cfg.action === 'scene' ? 'Scène' : 'Show'}
                      </label>
                      <select
                        value={cfg.targetId}
                        onChange={(e) => handleTargetChange(idx, e.target.value)}
                        className="w-full bg-slate-800 text-white border border-slate-600 rounded-lg px-3 py-2 text-sm focus:outline-none focus:ring-2 focus:ring-purple-500 focus:border-transparent"
                      >
                        <option value="">— Choisir —</option>
                        {cfg.action === 'scene' && scenes.map(s => (
                          <option key={s.id} value={s.id}>{s.icon} {s.name}</option>
                        ))}
                        {cfg.action === 'show' && shows.map(s => (
                          <option key={s.id} value={s.id}>{s.icon} {s.name}</option>
                        ))}
                      </select>
                    </div>
                  )}
                </div>

                {/* Behavior description */}
                <p className="text-xs text-slate-500 italic">
                  {getBehaviorDescription(cfg.action)}
                </p>
              </div>
            </div>
          </Card>
        ))}
      </div>

      {/* Wiring reminder */}
      <Card className="border border-slate-700 bg-slate-800/30">
        <h3 className="text-sm font-bold text-slate-300 mb-2">📐 Rappel câblage</h3>
        <div className="grid grid-cols-3 gap-4 text-xs text-slate-400">
          <div>
            <span className="text-rose-400 font-bold">Bouton 1</span> : D3 seul vers GND
          </div>
          <div>
            <span className="text-amber-400 font-bold">Bouton 2</span> : D4 seul vers GND
          </div>
          <div>
            <span className="text-violet-400 font-bold">Bouton 3</span> : D3 + D4 vers GND
          </div>
        </div>
      </Card>
    </div>
  );
};
