import { useState } from 'react';
import { Plus, Edit2, Trash2, Save } from 'lucide-react';
import { Button, Card, Input, Select, Alert } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { VirtualGroup, VirtualGroupAssignment } from '@/types';

export const VirtualGroupsTab = () => {
  const store = useAppStore();
  const [editingGroup, setEditingGroup] = useState<VirtualGroup | null>(null);

  const activeSetup = store.activeSetup;

  if (!activeSetup) {
    return <Alert variant="warning">Aucune configuration active. Allez dans "Setups" pour en activer une.</Alert>;
  }

  const handleNewGroup = () => {
    setEditingGroup({
      id: 'vg-' + Date.now(),
      name: 'Nouveau Groupe Virtuel',
      assignments: [],
    });
  };

  const saveGroup = async () => {
    if (!editingGroup) return;
    try {
      await apiService.saveVirtualGroup(editingGroup);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
      setEditingGroup(null);
    } catch (e: any) {
      store.setError(e.message || 'Erreur sauvegarde groupe virtuel');
    }
  };

  const deleteGroup = async (id: string) => {
    if (!confirm('Supprimer ce groupe virtuel ?')) return;
    try {
      await apiService.deleteVirtualGroup(id);
      const updated = await apiService.getActiveSetup();
      store.setActiveSetup(updated);
    } catch (e: any) {
      store.setError(e.message || 'Erreur suppression groupe');
    }
  };

  const addAssignment = () => {
    if (!editingGroup) return;
    if (activeSetup.instances.length === 0) return;
    const firstInst = activeSetup.instances[0];
    
    setEditingGroup({
      ...editingGroup,
      assignments: [...editingGroup.assignments, { instanceId: firstInst.id, channelName: 'ALL' }]
    });
  };

  const updateAssignment = (idx: number, updates: Partial<VirtualGroupAssignment>) => {
    if (!editingGroup) return;
    const newAss = [...editingGroup.assignments];
    // If instance changes, we should ideally reset the channel to a valid one, but we'll let the user pick
    newAss[idx] = { ...newAss[idx], ...updates };
    setEditingGroup({ ...editingGroup, assignments: newAss });
  };

  const removeAssignment = (idx: number) => {
    if (!editingGroup) return;
    setEditingGroup({
       ...editingGroup,
       assignments: editingGroup.assignments.filter((_, i) => i !== idx)
    });
  };

  const getInstanceName = (instId: string) => {
    const inst = activeSetup.instances.find(x => x.id === instId);
    return inst ? inst.name : 'Introuvable';
  }

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <h2 className="text-2xl font-bold text-white">Groupes Virtuels</h2>
        <Button onClick={handleNewGroup} disabled={activeSetup.instances.length === 0} className="flex items-center gap-2">
          <Plus size={20} /> Nouveau Groupe
        </Button>
      </div>

      {editingGroup && (
        <Card className="border border-purple-500">
          <h3 className="text-lg font-bold text-white mb-4">Éditer Groupe Virtuel</h3>
          <div className="mb-4">
             <Input
               label="Nom du Groupe"
               value={editingGroup.name}
               onChange={(e) => setEditingGroup({ ...editingGroup, name: e.target.value })}
             />
          </div>

          <div className="mb-4">
            <div className="flex items-center justify-between mb-2">
              <label className="text-sm font-medium text-slate-300">Projecteurs liés</label>
              <Button variant="outline" size="sm" onClick={addAssignment} className="flex items-center gap-1">
                <Plus size={14} /> Lier un Projecteur
              </Button>
            </div>
            
            <div className="space-y-2 max-h-80 overflow-y-auto pr-1">
               {editingGroup.assignments.map((ass, idx) => (
                  <div key={idx} className="flex items-center gap-2 bg-slate-900 rounded p-2">
                     <Select
                       value={ass.instanceId}
                       onChange={(e) => updateAssignment(idx, { instanceId: e.target.value, channelName: 'ALL' })}
                       options={activeSetup.instances.map(inst => ({ value: inst.id, label: inst.name }))}
                     />
                     <button onClick={() => removeAssignment(idx)} className="text-red-400 hover:text-red-300 p-1 ml-auto">
                        <Trash2 size={16} />
                     </button>
                  </div>
               ))}
               {editingGroup.assignments.length === 0 && (
                 <p className="text-sm text-slate-500 italic">Aucune assignation. Ce groupe ne contrôlera rien.</p>
               )}
            </div>
          </div>

          <div className="flex gap-2">
            <Button variant="primary" onClick={saveGroup} className="flex-1 flex items-center justify-center gap-2">
              <Save size={20} /> Enregistrer
            </Button>
            <Button variant="outline" onClick={() => setEditingGroup(null)} className="flex-1">
              Annuler
            </Button>
          </div>
        </Card>
      )}

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {activeSetup.virtualGroups.map(vg => (
           <Card key={vg.id} className="flex flex-col">
             <div className="flex justify-between items-start mb-4">
                <h3 className="text-xl font-bold text-white">{vg.name}</h3>
                <div className="flex gap-2">
                   <button onClick={() => setEditingGroup(vg)} className="text-slate-400 hover:text-white">
                      <Edit2 size={20} />
                   </button>
                   <button onClick={() => deleteGroup(vg.id)} className="text-red-400 hover:text-red-300">
                      <Trash2 size={20} />
                   </button>
                </div>
             </div>
             
             <div className="flex flex-col text-sm text-slate-300 mb-4 flex-1">
                <p className="text-xs text-slate-400 mb-2 uppercase tracking-wide">Assignations:</p>
                {vg.assignments.slice(0, 5).map((ass, i) => (
                  <div key={i} className="flex justify-between py-1 border-t border-slate-700/50">
                     <span>{getInstanceName(ass.instanceId)}</span>
                     <span className="text-purple-300 font-medium">{ass.channelName}</span>
                  </div>
                ))}
                {vg.assignments.length > 5 && (
                  <div className="text-xs text-slate-500 italic mt-2">+ {vg.assignments.length - 5} autres</div>
                )}
             </div>
           </Card>
        ))}
        {activeSetup.virtualGroups.length === 0 && (
           <div className="col-span-full p-8 text-center bg-slate-800/50 border border-slate-700/50 rounded-lg text-slate-400">
               Aucun groupe virtuel défini. Créez des groupes pour simplifier le pilotage manuel.
           </div>
        )}
      </div>
    </div>
  );
};
