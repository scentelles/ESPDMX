import { useState } from 'react';
import { Plus, Edit2, Trash2, Save, Play, ChevronDown, ChevronUp } from 'lucide-react';
import { Button, Card, Input, Alert } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { ColorScene, ChannelDefinition } from '@/types';

function channelSliderColor(chName: string): string {
  const n = chName.toLowerCase();
  if (n === 'red') return 'accent-red-500';
  if (n === 'green') return 'accent-green-500';
  if (n === 'blue') return 'accent-blue-500';
  if (n === 'white') return 'accent-white';
  return 'accent-purple-600';
}

export const ScenesTab = () => {
  const store = useAppStore();
  const [editingScene, setEditingScene] = useState<ColorScene | null>(null);
  const [expandedFixtures, setExpandedFixtures] = useState<Set<string>>(new Set());

  const activeSetup = store.activeSetup;

  if (!activeSetup) {
    return <Alert variant="warning">Aucune configuration active. Allez dans "Setups" pour en activer une.</Alert>;
  }

  const handleNewScene = () => {
    setEditingScene({
      id: 'scene-' + Date.now(),
      name: 'Nouvelle Scène',
      description: '',
      icon: '🎨',
      fixtureValues: [],
    });
  };

  const saveScene = async () => {
    if (!editingScene) return;
    try {
      await apiService.saveScene(editingScene);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
      setEditingScene(null);
    } catch (e: any) {
      store.setError(e.message || 'Erreur sauvegarde scène');
    }
  };

  const deleteScene = async (id: string) => {
    if (!confirm('Supprimer cette scène ?')) return;
    try {
      await apiService.deleteScene(id);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
    } catch (e: any) {
      store.setError(e.message || 'Erreur suppression scène');
    }
  };

  const testScene = async (sceneId: string) => {
    try {
       await apiService.activateScene(sceneId);
    } catch (e: any) {
       store.setError(e.message || 'Erreur test scène');
    }
  };

  const testEditingScene = async () => {
     if (!editingScene) return;
     try {
       await apiService.saveScene(editingScene);
       await testScene(editingScene.id);
     } catch (e: any) {}
  };

  // Editing helpers
  const toggleFixture = (instId: string) => {
    const next = new Set(expandedFixtures);
    if (next.has(instId)) next.delete(instId);
    else next.add(instId);
    setExpandedFixtures(next);
  };

  const isInstInScene = (instId: string) => {
    return editingScene?.fixtureValues.some(fv => fv.fixtureId === instId);
  };

  const toggleInstInScene = (instId: string, channels: ChannelDefinition[]) => {
    if (!editingScene) return;
    if (isInstInScene(instId)) {
      setEditingScene({
        ...editingScene,
        fixtureValues: editingScene.fixtureValues.filter(fv => fv.fixtureId !== instId),
      });
    } else {
      const defaults: Record<string, number> = {};
      channels.forEach(ch => { defaults[ch.name] = ch.defaultValue; });
      setEditingScene({
         ...editingScene,
         fixtureValues: [...editingScene.fixtureValues, { fixtureId: instId, values: defaults }]
      });
      setExpandedFixtures(prev => new Set(prev).add(instId));
    }
  };

  const setChannelValue = (instId: string, channelName: string, val: number) => {
     if (!editingScene) return;
     setEditingScene({
        ...editingScene,
        fixtureValues: editingScene.fixtureValues.map(fv => 
           fv.fixtureId === instId ? { ...fv, values: { ...fv.values, [channelName]: val }} : fv
        )
     });
  };

  const getChannelValue = (instId: string, channelName: string) => {
     if (!editingScene) return 0;
     const fv = editingScene.fixtureValues.find(x => x.fixtureId === instId);
     return fv?.values[channelName] ?? 0;
  };

  return (
    <div className="space-y-6">
       <div className="flex justify-between items-center">
         <h2 className="text-2xl font-bold text-white">Scènes ({activeSetup.scenes.length})</h2>
         <Button onClick={handleNewScene} className="flex items-center gap-2">
            <Plus size={20} /> Nouvelle Scène
         </Button>
       </div>

       {editingScene && (
          <Card className="border border-purple-500">
             <h3 className="text-lg font-bold text-white mb-4">Éditer Scène</h3>
             <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
                <Input label="Nom" value={editingScene.name} onChange={e => setEditingScene({...editingScene, name: e.target.value})} />
                <Input label="Icône" value={editingScene.icon} onChange={e => setEditingScene({...editingScene, icon: e.target.value})} />
                <Input label="Description" value={editingScene.description} onChange={e => setEditingScene({...editingScene, description: e.target.value})} />
             </div>

             <div className="mb-4">
                <label className="text-sm font-medium text-slate-300 mb-2 block">Valeurs DMX (Projecteurs)</label>
                {activeSetup.instances.length === 0 && (
                   <Alert variant="warning">Aucun projecteur dans le setup !</Alert>
                )}
                
                <div className="space-y-2">
                   {activeSetup.instances.map(inst => {
                      const prof = store.profiles.find(p => p.id === inst.profileId);
                      if (!prof) return null;

                      const inScene = !!isInstInScene(inst.id);
                      const expanded = expandedFixtures.has(inst.id);

                      return (
                         <div key={inst.id} className="bg-slate-900 rounded-lg border border-slate-700">
                            <div className="flex items-center gap-3 p-3 cursor-pointer hover:bg-slate-800/50" onClick={() => inScene && toggleFixture(inst.id)}>
                               <input type="checkbox" checked={inScene} onChange={() => toggleInstInScene(inst.id, prof.channels)} onClick={e => e.stopPropagation()} className="accent-purple-600" />
                               <span className="font-medium text-white flex-1">{inst.name}</span>
                               <span className="text-xs text-slate-500">DMX {inst.dmxAddress} - {prof.name}</span>
                               {inScene && (expanded ? <ChevronUp size={16} /> : <ChevronDown size={16} />)}
                            </div>

                            {inScene && expanded && (
                               <div className="p-3 pt-0 space-y-3 border-t border-slate-700">
                                  {prof.channels.map(ch => (
                                     <div key={ch.name} className="flex items-center gap-3">
                                        <span className="text-sm text-slate-400 w-24 truncate">{ch.name}</span>
                                        <input
                                           type="range" min={0} max={255}
                                           value={getChannelValue(inst.id, ch.name)}
                                           onChange={(e) => setChannelValue(inst.id, ch.name, parseInt(e.target.value))}
                                           className={`flex-1 h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer ${channelSliderColor(ch.name)}`}
                                        />
                                        <input
                                           type="number" min={0} max={255}
                                           value={getChannelValue(inst.id, ch.name)}
                                           onChange={(e) => setChannelValue(inst.id, ch.name, parseInt(e.target.value)||0)}
                                           className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
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
                 <Button variant="primary" onClick={saveScene} className="flex-1 flex items-center justify-center gap-2">
                    <Save size={20} /> Enregistrer
                 </Button>
                 <Button variant="outline" onClick={testEditingScene} className="flex items-center gap-2" title="Test Live">
                    <Play size={20} /> Tester
                 </Button>
                 <Button variant="outline" onClick={() => setEditingScene(null)} className="flex-1">
                    Annuler
                 </Button>
             </div>
          </Card>
       )}

       <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {activeSetup.scenes.map(s => (
             <Card key={s.id}>
                <div className="flex justify-between items-start mb-2">
                   <div className="flex items-center gap-2">
                      <span className="text-2xl">{s.icon}</span>
                      <h3 className="text-xl font-bold text-white">{s.name}</h3>
                   </div>
                   <div className="flex gap-2">
                      <button onClick={() => testScene(s.id)} className="text-green-400 hover:text-green-300">
                         <Play size={20} />
                      </button>
                      <button onClick={() => setEditingScene(s)} className="text-slate-400 hover:text-white">
                         <Edit2 size={20} />
                      </button>
                      <button onClick={() => deleteScene(s.id)} className="text-red-400 hover:text-red-300">
                         <Trash2 size={20} />
                      </button>
                   </div>
                </div>
                <p className="text-sm text-slate-400">{s.description || 'Appuyez sur play pour tester.'}</p>
             </Card>
          ))}
          {activeSetup.scenes.length === 0 && (
             <div className="col-span-full p-8 text-center bg-slate-800/50 rounded-lg text-slate-400">
                Aucune scène. Créez-en une pour l'utiliser dans un show !
             </div>
          )}
       </div>
    </div>
  );
};
