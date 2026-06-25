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
      groupId: 1,
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

  const getEffect = (instId: string) => {
     if (!editingScene) return null;
     const fv = editingScene.fixtureValues.find(x => x.fixtureId === instId);
     return fv?.effect;
  };

  const updateEffect = (instId: string, updates: Partial<import('@/types').FixtureEffect>) => {
     if (!editingScene) return;
     setEditingScene({
        ...editingScene,
        fixtureValues: editingScene.fixtureValues.map(fv => {
           if (fv.fixtureId !== instId) return fv;
           const currentEffect = fv.effect || { type: 'none', speed: 128, colorHex: '#ff0000' };
           return { ...fv, effect: { ...currentEffect, ...updates } };
        })
     });
  };

  const isVgInScene = (groupId: string) => {
    return editingScene?.virtualGroupValues?.some(v => v.groupId === groupId);
  };

  const toggleVgInScene = (groupId: string) => {
    if (!editingScene) return;
    const current = editingScene.virtualGroupValues || [];
    if (isVgInScene(groupId)) {
      setEditingScene({
        ...editingScene,
        virtualGroupValues: current.filter(v => v.groupId !== groupId),
      });
    } else {
      setEditingScene({
         ...editingScene,
         virtualGroupValues: [...current, { groupId, dimmer: 255 }]
      });
    }
  };

  const setVgValue = (groupId: string, field: 'dimmer' | 'colorHex', val: any) => {
     if (!editingScene) return;
     const current = editingScene.virtualGroupValues || [];
     setEditingScene({
        ...editingScene,
        virtualGroupValues: current.map(v => 
           v.groupId === groupId ? { ...v, [field]: val } : v
        )
     });
  };

  const getVgValue = (groupId: string) => {
     if (!editingScene) return null;
     const current = editingScene.virtualGroupValues || [];
     return current.find(x => x.groupId === groupId);
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
                <label className="text-sm font-medium text-slate-300 mb-2 block">Groupe de la Scène</label>
                <div className="flex gap-4">
                  <label className="flex items-center gap-2 text-white cursor-pointer">
                    <input type="radio" name="groupId" value={1} checked={editingScene.groupId === 1 || !editingScene.groupId} onChange={() => setEditingScene({...editingScene, groupId: 1})} className="accent-purple-600" />
                    Groupe 1
                  </label>
                  <label className="flex items-center gap-2 text-white cursor-pointer">
                    <input type="radio" name="groupId" value={2} checked={editingScene.groupId === 2} onChange={() => setEditingScene({...editingScene, groupId: 2})} className="accent-purple-600" />
                    Groupe 2
                  </label>
                </div>
             </div>

             <div className="mb-4">
                <label className="text-sm font-medium text-slate-300 mb-2 block">Valeurs DMX (Groupes Virtuels)</label>
                {activeSetup.virtualGroups && activeSetup.virtualGroups.length === 0 && (
                   <Alert variant="info">Aucun groupe virtuel n'est configuré.</Alert>
                )}
                
                <div className="space-y-2 mb-6">
                   {activeSetup.virtualGroups?.map(vg => {
                      const inScene = !!isVgInScene(vg.id);
                      const vgVal = getVgValue(vg.id);

                      return (
                         <div key={vg.id} className="bg-slate-900 rounded-lg border border-slate-700 p-3">
                            <div className="flex items-center gap-3">
                               <input type="checkbox" checked={inScene} onChange={() => toggleVgInScene(vg.id)} className="accent-purple-600" />
                               <span className="font-medium text-white flex-1">{vg.name}</span>
                            </div>

                            {inScene && (
                               <div className="mt-3 space-y-3 pl-6 border-l-2 border-slate-700 ml-2">
                                  <div className="flex items-center gap-3">
                                     <span className="text-sm text-slate-400 w-24">Couleur (Hex)</span>
                                     <input 
                                        type="color" 
                                        value={vgVal?.colorHex || '#000000'} 
                                        onChange={(e) => setVgValue(vg.id, 'colorHex', e.target.value)}
                                        className="w-10 h-8 rounded cursor-pointer bg-slate-800 border-none"
                                     />
                                     <button onClick={() => setVgValue(vg.id, 'colorHex', undefined)} className="text-xs text-slate-500 hover:text-white ml-2">Effacer</button>
                                  </div>
                                  <div className="flex items-center gap-3">
                                     <span className="text-sm text-slate-400 w-24">Intensité</span>
                                     <input
                                        type="range" min={0} max={255}
                                        value={vgVal?.dimmer ?? -1}
                                        onChange={(e) => setVgValue(vg.id, 'dimmer', parseInt(e.target.value))}
                                        className={`flex-1 h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-purple-600`}
                                     />
                                     <input
                                        type="number" min={-1} max={255}
                                        value={vgVal?.dimmer ?? -1}
                                        onChange={(e) => setVgValue(vg.id, 'dimmer', parseInt(e.target.value)||0)}
                                        className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
                                     />
                                     <button onClick={() => setVgValue(vg.id, 'dimmer', -1)} className="text-xs text-slate-500 hover:text-white ml-2">Non défini (-1)</button>
                                  </div>
                               </div>
                            )}
                         </div>
                      );
                   })}
                </div>

                <label className="text-sm font-medium text-slate-300 mb-2 block">Valeurs DMX (Projecteurs individuels)</label>
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
                                  <div className="space-y-3 mb-4">
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
                                  <div className="pt-3 border-t border-slate-700">
                                     <h4 className="text-xs font-bold uppercase tracking-wider text-slate-400 mb-2">Effets Dynamiques (FX)</h4>
                                     <div className="flex flex-col gap-3">
                                        <div className="flex gap-3 items-end">
                                           <div className="flex-1">
                                              <label className="text-xs text-slate-500 mb-1 block">Type d'effet</label>
                                              <select
                                                 value={getEffect(inst.id)?.type || 'none'}
                                                 onChange={(e) => updateEffect(inst.id, { type: e.target.value })}
                                                 className="w-full bg-slate-800 border border-slate-700 rounded px-2 py-2 text-sm text-white"
                                              >
                                                 <option value="none">Aucun effet</option>
                                                 <option value="chaser">Chaser (Chenillard)</option>
                                                 <option value="sparkle">Scintillant Aléatoire</option>
                                                 <option value="up">Montée</option>
                                                 <option value="sine">Sinus (Couleur Synchro)</option>
                                                 <option value="sine2">Sinus (Chenillard)</option>
                                              </select>
                                           </div>
                                           {getEffect(inst.id)?.type && getEffect(inst.id)?.type !== 'none' && (
                                              <div className="w-16 flex flex-col">
                                                 <label className="text-xs text-slate-500 mb-1">Couleur</label>
                                                 <input 
                                                    type="color" 
                                                    value={getEffect(inst.id)?.colorHex || '#ff0000'} 
                                                    onChange={(e) => updateEffect(inst.id, { colorHex: e.target.value })}
                                                    className="w-full h-[36px] rounded cursor-pointer bg-slate-800 border-none"
                                                 />
                                              </div>
                                           )}
                                        </div>
                                        {getEffect(inst.id)?.type && getEffect(inst.id)?.type !== 'none' && (
                                           <div className="flex flex-col gap-2">
                                              <div className="flex items-center gap-3">
                                                 <span className="text-sm text-slate-400 w-24 truncate">Vitesse</span>
                                                 <input
                                                    type="range" min={1} max={100}
                                                    value={getEffect(inst.id)?.speed ?? 50}
                                                    onChange={(e) => updateEffect(inst.id, { speed: parseInt(e.target.value)||1 })}
                                                    className="flex-1 h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-yellow-500"
                                                 />
                                                 <input
                                                    type="number" min={1} max={100}
                                                    value={getEffect(inst.id)?.speed ?? 50}
                                                    onChange={(e) => updateEffect(inst.id, { speed: parseInt(e.target.value)||1 })}
                                                    className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
                                                 />
                                              </div>
                                              {['chaser', 'up', 'sine2'].includes(getEffect(inst.id)?.type || '') && (
                                                 <div className="flex items-center gap-2 pl-27">
                                                    <input 
                                                       type="checkbox" 
                                                       id={`reverse-${inst.id}`}
                                                       checked={getEffect(inst.id)?.reverse || false}
                                                       onChange={(e) => updateEffect(inst.id, { reverse: e.target.checked })}
                                                       className="accent-purple-600 cursor-pointer"
                                                    />
                                                    <label htmlFor={`reverse-${inst.id}`} className="text-sm text-slate-400 cursor-pointer">Sens inverse (Reverse)</label>
                                                 </div>
                                              )}
                                           </div>
                                        )}
                                     </div>
                                  </div>
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

       <div className="space-y-8">
         {/* Groupe 1 */}
         <div>
            <h3 className="text-xl font-bold text-white mb-4 border-b border-slate-700 pb-2">Groupe 1 (Tâches de fond)</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
               {activeSetup.scenes.filter(s => s.groupId === 1 || !s.groupId).map(s => (
                  <Card key={s.id}>
                     <div className="flex justify-between items-start mb-2">
                         <div className="flex flex-col">
                            <div className="flex items-center gap-2">
                               <span className="text-2xl">{s.icon}</span>
                               <h3 className="text-xl font-bold text-white">{s.name}</h3>
                            </div>
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
               {activeSetup.scenes.filter(s => s.groupId === 1 || !s.groupId).length === 0 && (
                  <div className="col-span-full p-8 text-center bg-slate-800/50 rounded-lg text-slate-400">
                     Aucune scène dans le Groupe 1.
                  </div>
               )}
            </div>
         </div>

         {/* Groupe 2 */}
         <div>
            <h3 className="text-xl font-bold text-white mb-4 border-b border-slate-700 pb-2">Groupe 2 (Prioritaires)</h3>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
               {activeSetup.scenes.filter(s => s.groupId === 2).map(s => (
                  <Card key={s.id}>
                     <div className="flex justify-between items-start mb-2">
                         <div className="flex flex-col">
                            <div className="flex items-center gap-2">
                               <span className="text-2xl">{s.icon}</span>
                               <h3 className="text-xl font-bold text-white">{s.name}</h3>
                            </div>
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
               {activeSetup.scenes.filter(s => s.groupId === 2).length === 0 && (
                  <div className="col-span-full p-8 text-center bg-slate-800/50 rounded-lg text-slate-400">
                     Aucune scène dans le Groupe 2.
                  </div>
               )}
            </div>
         </div>
       </div>
    </div>
  );
};
