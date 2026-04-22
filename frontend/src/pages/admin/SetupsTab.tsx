import { useState } from 'react';
import { Plus, Trash2, Save, Edit2, Info } from 'lucide-react';
import { Button, Card, Input, Select } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { FixtureInstance } from '@/types';

export const SetupsTab = () => {
  const store = useAppStore();
  const [newSetupName, setNewSetupName] = useState('');
  const [editingInstance, setEditingInstance] = useState<FixtureInstance | null>(null);

  const fetchSetups = async () => {
    try {
      const list = await apiService.getSetupsList();
      store.setSetupsList(list);
    } catch (e: any) {
      store.setError(e.message || 'Erreur fetch configs');
    }
  };

  const handleCreateSetup = async () => {
    if (!newSetupName) return;
    const id = 'setup-' + Date.now();
    try {
      await apiService.createSetup(id, newSetupName);
      setNewSetupName('');
      fetchSetups();
    } catch (e: any) {
      store.setError(e.message || 'Erreur création config');
    }
  };

  const handleDeleteSetup = async (id: string) => {
    if (!confirm('Supprimer cette configuration ?')) return;
    try {
      await apiService.deleteSetup(id);
      fetchSetups();
    } catch (e: any) {
      store.setError(e.message || 'Erreur (impossible de supprimer config active)');
    }
  };

  const activateSetup = async (id: string) => {
    if (!confirm('Activer cette config ? Cela réinitialisera le DMX et arrêtera les shows.')) return;
    try {
      await apiService.activateSetup(id);
      const active = await apiService.getActiveSetup();
      store.setActiveSetup(active);
      fetchSetups();
    } catch (e: any) {
      store.setError(e.message || 'Erreur activation');
    }
  };

  const handleNewInstance = () => {
    if (store.profiles.length === 0) {
      store.setError('Créez d\'abord un modèle dans le catalogue');
      return;
    }
    setEditingInstance({
      id: 'inst-' + Date.now(),
      profileId: store.profiles[0].id,
      name: 'Projecteur 1',
      dmxAddress: 1,
      enabled: true,
    });
  };

  const saveInstance = async () => {
    if (!editingInstance) return;

    // DMX Validation
    if (editingInstance.dmxAddress < 1) {
      store.setError("L'adresse DMX doit être supérieure ou égale à 1.");
      return;
    }

    const newProfile = store.profiles.find(p => p.id === editingInstance.profileId);
    const newLength = newProfile ? newProfile.channelCount : 1;
    const newEnd = editingInstance.dmxAddress + newLength - 1;

    if (newEnd > 512) {
      store.setError(`L'adresse DMX hors limite : le projecteur se termine au canal ${newEnd} (max 512).`);
      return;
    }

    if (store.activeSetup) {
      const overlap = store.activeSetup.instances.find(inst => {
        if (inst.id === editingInstance.id) return false;
        const p = store.profiles.find(x => x.id === inst.profileId);
        const l = p ? p.channelCount : 1;
        const e = inst.dmxAddress + l - 1;
        return (editingInstance.dmxAddress <= e && inst.dmxAddress <= newEnd);
      });

      if (overlap) {
        const p = store.profiles.find(x => x.id === overlap.profileId);
        const l = p ? p.channelCount : 1;
        store.setError(`Chevauchement DMX : Le projecteur '${overlap.name}' utilise les canaux ${overlap.dmxAddress} à ${overlap.dmxAddress + l - 1}.`);
        return;
      }
    }

    try {
      await apiService.saveInstance(editingInstance);
      const active = await apiService.getActiveSetup();
      store.setActiveSetup(active);
      setEditingInstance(null);
    } catch (e: any) {
      store.setError(e.message || 'Erreur sauvegarde instance');
    }
  };

  const deleteInstance = async (id: string) => {
    if (!confirm('Supprimer cette instance ?')) return;
    try {
      await apiService.deleteInstance(id);
      const active = await apiService.getActiveSetup();
      store.setActiveSetup(active);
    } catch (e: any) {
      store.setError(e.message || 'Erreur suppression instance');
    }
  };

  const getProfileName = (pid: string) => {
    const p = store.profiles.find(x => x.id === pid);
    return p ? p.name : 'Inconnu';
  };

  return (
    <div className="space-y-8">
      {/* Setups List Section */}
      <Card>
        <div className="flex items-start gap-4 mb-6 p-4 bg-sky-900/30 text-sky-200 rounded-lg border border-sky-800">
           <Info className="shrink-0 mt-0.5 text-sky-400" size={20} />
           <p className="text-sm">
              <strong>Info :</strong> Pour assigner des projecteurs, configurer les adresses DMX, ou créer des scènes sur une configuration, 
              vous devez d'abord <strong>l'Activer</strong>. Une seule configuration ne peut être chargée en mémoire pour édition/lecture à la fois.
           </p>
        </div>
        <h2 className="text-xl font-bold text-white mb-4">Configurations (Setups)</h2>
        <div className="flex gap-2 mb-6">
          <Input 
            placeholder="Nom de la configuration" 
            value={newSetupName}
            onChange={e => setNewSetupName(e.target.value)}
            className="flex-1"
          />
          <Button onClick={handleCreateSetup} disabled={!newSetupName}>
            Créer
          </Button>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          {store.setupsList.map(s => (
            <div key={s.id} className={`p-4 rounded-lg border ${s.active ? 'bg-purple-900 border-purple-500' : 'bg-slate-800 border-slate-700'}`}>
              <div className="flex justify-between items-start mb-4">
                <div>
                  <h3 className="font-bold text-white">{s.name}</h3>
                  {s.active && <span className="text-xs text-purple-300">Configuration Active</span>}
                </div>
                {!s.active && (
                   <button onClick={() => handleDeleteSetup(s.id)} className="text-red-400 hover:text-red-300">
                     <Trash2 size={18} />
                   </button>
                )}
              </div>
              {!s.active && (
                <Button variant="outline" className="w-full text-sm" onClick={() => activateSetup(s.id)}>
                   Mettre en ligne pour Éditer
                </Button>
              )}
            </div>
          ))}
        </div>
      </Card>

      {/* Instances in Active Setup */}
      {store.activeSetup ? (
        <div>
          <div className="flex justify-between items-center mb-4">
            <h2 className="text-xl font-bold text-white">Projecteurs assignés à "{store.activeSetup.name}"</h2>
            <Button onClick={handleNewInstance} className="flex items-center gap-2" size="sm">
              <Plus size={16} /> Ajouter Projecteur
            </Button>
          </div>

          {editingInstance && (
            <Card className="mb-6 border border-purple-500">
              <h3 className="font-bold text-white mb-4">Éditer Projecteur</h3>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-4">
                <Input
                  label="Nom (ex: Front Wash Gauche)"
                  value={editingInstance.name}
                  onChange={(e) => setEditingInstance({ ...editingInstance, name: e.target.value })}
                />
                <Input
                  label="Adresse DMX"
                  type="number"
                  min={1} max={512}
                  value={editingInstance.dmxAddress}
                  onChange={(e) => setEditingInstance({ ...editingInstance, dmxAddress: parseInt(e.target.value) || 1 })}
                />
                <Select
                  label="Modèle (Depuis Catalogue)"
                  value={editingInstance.profileId}
                  onChange={(e) => setEditingInstance({ ...editingInstance, profileId: e.target.value })}
                  options={store.profiles.map(p => ({ value: p.id, label: p.name + ` (${p.channelCount}ch)` }))}
                />
              </div>
              <label className="flex items-center gap-2 text-sm text-slate-300 mb-4">
                <input
                  type="checkbox"
                  checked={editingInstance.enabled}
                  onChange={(e) => setEditingInstance({ ...editingInstance, enabled: e.target.checked })}
                  className="accent-purple-600"
                />
                Activé
              </label>
              <div className="flex gap-2">
                <Button variant="primary" onClick={saveInstance} className="flex-1 flex items-center justify-center gap-2">
                  <Save size={20} /> Enregistrer
                </Button>
                <Button variant="outline" onClick={() => setEditingInstance(null)} className="flex-1">
                  Annuler
                </Button>
              </div>
            </Card>
          )}

          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
             {store.activeSetup.instances.map(inst => (
               <Card key={inst.id} className={!inst.enabled ? 'opacity-60' : ''}>
                 <div className="flex justify-between items-start mb-2">
                   <div>
                     <h3 className="font-bold text-white">{inst.name}</h3>
                     <p className="text-xs text-slate-400">Modèle: {getProfileName(inst.profileId)}</p>
                   </div>
                   <div className="flex gap-2">
                     <button onClick={() => setEditingInstance(inst)} className="text-slate-400 hover:text-white" title="Modifier">
                       <Edit2 size={18} />
                     </button>
                     <button onClick={() => deleteInstance(inst.id)} className="text-red-400 hover:text-red-300">
                       <Trash2 size={18} />
                     </button>
                   </div>
                 </div>
                 <div className="mt-4 pt-4 border-t border-slate-700/50 flex justify-between items-center text-sm text-slate-300">
                    <span>DMX Adresse:</span>
                    <span className="font-mono bg-slate-800 px-2 py-0.5 rounded">{inst.dmxAddress}</span>
                 </div>
               </Card>
             ))}
             {store.activeSetup.instances.length === 0 && (
               <div className="col-span-full p-6 text-center text-slate-400 border border-slate-700/50 rounded-lg">
                 Aucun projecteur assigné dans cette configuration.
               </div>
             )}
          </div>
        </div>
      ) : store.setupsList.some(s => s.active) ? (
        <Card className="bg-red-900/30 border-red-500">
          <h2 className="text-xl font-bold text-white mb-2">Erreur de chargement</h2>
          <p className="text-red-200">La configuration a bien été activée (marquée "Active" ci-dessus), mais les données complètes n'ont pas pu être chargées depuis le contrôleur DMX. Cela est dû à une réponse vide ou invalide de l'ESP32.</p>
          <div className="mt-4 p-4 bg-black/50 text-red-300 font-mono text-sm break-words overflow-hidden" id="debug-box">
             Cliquez sur "Diagnostiquer" pour analyser la réponse...
          </div>
          <div className="flex gap-4 mt-4">
            <Button variant="outline" onClick={() => fetchSetups()}>Forcer le rechargement</Button>
            <Button variant="danger" onClick={async () => {
              try {
                const res = await fetch('/api/setups/active');
                const text = await res.text();
                document.getElementById('debug-box')!.innerText = "HTTP " + res.status + " | RAW BODY: " + (text ? text : "[CORPS VIDE]");
              } catch(e) {
                document.getElementById('debug-box')!.innerText = "FETCH FAILED: " + String(e);
              }
            }}>Diagnostiquer</Button>
          </div>
        </Card>
      ) : null}
    </div>
  );
};
