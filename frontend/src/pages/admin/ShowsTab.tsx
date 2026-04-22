import { useState } from 'react';
import { Plus, Edit2, Trash2, Save, Play, ChevronDown, ChevronUp, GripVertical } from 'lucide-react';
import { Button, Card, Input, Alert } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { DynamicShow, ShowStep } from '@/types';

export const ShowsTab = () => {
  const store = useAppStore();
  const [editingShow, setEditingShow] = useState<DynamicShow | null>(null);

  const activeSetup = store.activeSetup;

  if (!activeSetup) {
    return <Alert variant="warning">Aucune configuration active. Allez dans "Setups" pour en activer une.</Alert>;
  }

  const handleNewShow = () => {
    setEditingShow({
      id: 'show-' + Date.now(),
      name: 'Nouveau Show',
      description: '',
      icon: '🎆',
      loop: true,
      steps: [],
    });
  };

  const saveShow = async () => {
    if (!editingShow) return;
    try {
      await apiService.saveShow(editingShow);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
      setEditingShow(null);
    } catch (e: any) {
      store.setError(e.message || 'Erreur sauvegarde show');
    }
  };

  const deleteShow = async (id: string) => {
    if (!confirm('Supprimer ce show ?')) return;
    try {
      await apiService.deleteShow(id);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
    } catch (e: any) {
      store.setError(e.message || 'Erreur suppression show');
    }
  };

  const testShow = async (showId: string) => {
    try {
       await apiService.startShow(showId);
    } catch (e: any) {
       store.setError(e.message || 'Erreur démarrage show');
    }
  };

  const testScene = async (sceneId: string) => {
     try {
       await apiService.activateScene(sceneId);
     } catch (e: any) {}
  };

  // Editing helpers
  const addStep = () => {
     if (!editingShow || activeSetup.scenes.length === 0) return;
     setEditingShow({
       ...editingShow,
       steps: [...editingShow.steps, { sceneId: activeSetup.scenes[0].id, duration: 5000, transitionTime: 1000, smoothTransition: false }]
     });
  };

  const updateStep = (idx: number, updates: Partial<ShowStep>) => {
     if (!editingShow) return;
     const newSteps = [...editingShow.steps];
     newSteps[idx] = { ...newSteps[idx], ...updates };
     setEditingShow({ ...editingShow, steps: newSteps });
  };

  const moveStep = (idx: number, dir: number) => {
     if (!editingShow) return;
     const newIdx = idx + dir;
     if (newIdx < 0 || newIdx >= editingShow.steps.length) return;
     const newSteps = [...editingShow.steps];
     const tmp = newSteps[idx];
     newSteps[idx] = newSteps[newIdx];
     newSteps[newIdx] = tmp;
     setEditingShow({ ...editingShow, steps: newSteps });
  };

  const removeStep = (idx: number) => {
     if (!editingShow) return;
     setEditingShow({ ...editingShow, steps: editingShow.steps.filter((_, i) => i !== idx) });
  };

  return (
    <div className="space-y-6">
       <div className="flex justify-between items-center">
         <h2 className="text-2xl font-bold text-white">Shows Dynamiques ({activeSetup.shows.length})</h2>
         <Button onClick={handleNewShow} className="flex items-center gap-2">
            <Plus size={20} /> Nouveau Show
         </Button>
       </div>

       {editingShow && (
          <Card className="border border-purple-500">
             <h3 className="text-lg font-bold text-white mb-4">Éditer Show</h3>
             <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
                <Input label="Nom" value={editingShow.name} onChange={e => setEditingShow({...editingShow, name: e.target.value})} />
                <Input label="Icône" value={editingShow.icon} onChange={e => setEditingShow({...editingShow, icon: e.target.value})} />
                <Input label="Description" value={editingShow.description} onChange={e => setEditingShow({...editingShow, description: e.target.value})} />
             </div>
             
             <label className="flex items-center gap-2 text-sm text-slate-300 mb-4">
                <input type="checkbox" checked={editingShow.loop} onChange={e => setEditingShow({...editingShow, loop: e.target.checked})} className="accent-purple-600" />
                Jouer en boucle
             </label>

             <div className="mb-4">
                <div className="flex items-center justify-between mb-2">
                   <label className="text-sm font-medium text-slate-300">Étapes de la séquence</label>
                   <Button variant="outline" size="sm" onClick={addStep} disabled={activeSetup.scenes.length === 0} className="flex items-center gap-1">
                      <Plus size={14} /> Ajouter Étape
                   </Button>
                </div>
                {activeSetup.scenes.length === 0 && <Alert variant="warning">Créez d'abord des scènes.</Alert>}

                <div className="space-y-2 max-h-96 overflow-y-auto pr-1">
                   {editingShow.steps.map((step, idx) => (
                      <div key={idx} className="bg-slate-900 rounded-lg border border-slate-700 p-3">
                         <div className="flex items-center gap-2 mb-2">
                            <GripVertical size={16} className="text-slate-600" />
                            <span className="text-xs text-slate-500 font-mono w-6">#{idx + 1}</span>
                            <select
                               value={step.sceneId}
                               onChange={(e) => updateStep(idx, { sceneId: e.target.value })}
                               className="flex-1 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white"
                            >
                               {activeSetup.scenes.map(s => <option key={s.id} value={s.id}>{s.icon} {s.name}</option>)}
                            </select>
                            <button onClick={() => testScene(step.sceneId)} className="text-green-400 hover:text-green-300 p-1" title="Tester scène"><Play size={14}/></button>
                            <button onClick={() => moveStep(idx, -1)} disabled={idx === 0} className="text-slate-400 p-1"><ChevronUp size={16}/></button>
                            <button onClick={() => moveStep(idx, 1)} disabled={idx === editingShow.steps.length-1} className="text-slate-400 p-1"><ChevronDown size={16}/></button>
                            <button onClick={() => removeStep(idx)} className="text-red-400 hover:text-red-300 p-1"><Trash2 size={14}/></button>
                         </div>
                         <div className="grid grid-cols-2 gap-3 pl-8">
                            <div>
                               <label className="text-xs text-slate-500">Durée Maintien (ms)</label>
                               <input type="number" value={step.duration} onChange={e => updateStep(idx, { duration: parseInt(e.target.value) || 0})} className="w-full bg-slate-800 rounded px-2 py-1 text-white border border-slate-700" />
                            </div>
                            <div>
                               <label className="text-xs text-slate-500">Durée Transition (ms)</label>
                               <input type="number" value={step.transitionTime} onChange={e => updateStep(idx, { transitionTime: parseInt(e.target.value) || 0})} className="w-full bg-slate-800 rounded px-2 py-1 text-white border border-slate-700" />
                            </div>
                         </div>
                      </div>
                   ))}
                </div>
             </div>

             <div className="flex gap-2">
                 <Button variant="primary" onClick={saveShow} className="flex-1 flex items-center justify-center gap-2">
                    <Save size={20} /> Enregistrer
                 </Button>
                 <Button variant="outline" onClick={() => setEditingShow(null)} className="flex-1">
                    Annuler
                 </Button>
             </div>
          </Card>
       )}

       <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {activeSetup.shows.map(s => (
             <Card key={s.id}>
                <div className="flex justify-between items-start mb-2">
                   <div className="flex items-center gap-2">
                      <span className="text-2xl">{s.icon}</span>
                      <h3 className="text-xl font-bold text-white">{s.name}</h3>
                   </div>
                   <div className="flex gap-2">
                      <button onClick={() => testShow(s.id)} className="text-green-400 hover:text-green-300">
                         <Play size={20} />
                      </button>
                      <button onClick={() => setEditingShow(s)} className="text-slate-400 hover:text-white">
                         <Edit2 size={20} />
                      </button>
                      <button onClick={() => deleteShow(s.id)} className="text-red-400 hover:text-red-300">
                         <Trash2 size={20} />
                      </button>
                   </div>
                </div>
                <p className="text-sm text-slate-400 mb-2">{s.description || 'Séquence dynamique.'}</p>
                <div className="text-xs text-slate-500">{s.steps.length} Étapes</div>
             </Card>
          ))}
          {activeSetup.shows.length === 0 && (
             <div className="col-span-full p-8 text-center bg-slate-800/50 rounded-lg text-slate-400">
                Aucun show.
             </div>
          )}
       </div>
    </div>
  );
};
