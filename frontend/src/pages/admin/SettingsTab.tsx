import { useState, useEffect } from 'react';
import { Save, AlertTriangle, Download, Upload, Trash2, Database, RefreshCw, Edit2, Check, X } from 'lucide-react';
import { Button, Card, Input } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { SystemConfig } from '@/types';

export const SettingsTab = () => {
  const store = useAppStore();
  const [config, setConfig] = useState<Partial<SystemConfig>>({});
  const [otaProgress, setOtaProgress] = useState<number>(-1);
  const [otaStatus, setOtaStatus] = useState<string>('');
  
  const [backups, setBackups] = useState<{ id: string; filename: string; timestamp: number; size: number; description?: string }[]>([]);
  const [editingDescId, setEditingDescId] = useState<string | null>(null);
  const [descInputValue, setDescInputValue] = useState("");
  const [isBackingUp, setIsBackingUp] = useState<boolean>(false);
  const [isRestoring, setIsRestoring] = useState<string | null>(null);
  const [isUploading, setIsUploading] = useState<boolean>(false);

  useEffect(() => {
    loadConfig();
    loadBackups();
  }, []);

  const loadConfig = async () => {
    try {
      const c = await apiService.getSystemConfig();
      setConfig(c || {});
    } catch (e: any) {
      store.setError(e.message);
    }
  };

  const handleSaveConfig = async () => {
    try {
      await apiService.saveSystemConfig(config);
      // Automatically create a backup in the list with date and time
      const nowTs = Math.floor(Date.now() / 1000);
      await apiService.createBackup(nowTs);
      store.setSuccess('Configuration enregistrée et Sauvegarde complète créée !');
      loadBackups();
    } catch (error: any) {
      store.setError(error.message);
    }
  };

  const handleReboot = async () => {
    if (!confirm('Redémarrer le contrôleur ESP32 ?')) return;
    try {
      await apiService.reboot();
      store.setSuccess('Redémarrage en cours...');
    } catch (error: any) {
      store.setError(error.message);
    }
  };

  const handleOTA = async (event: React.ChangeEvent<HTMLInputElement>, type: 'firmware' | 'spiffs') => {
    const file = event.target.files?.[0];
    if (!file) return;

    if (!confirm(`Êtes-vous sûr de vouloir flasher ce fichier (${file.name}) comme ${type} ? Le système redémarrera.`)) {
      event.target.value = '';
      return;
    }

    setOtaProgress(0);
    setOtaStatus('Téléversement...');
    try {
      await apiService.uploadOTA(file, type, (pct) => setOtaProgress(pct));
      setOtaStatus('Succès ! Redémarrage en cours...');
      setTimeout(() => window.location.reload(), 4000);
    } catch (e: any) {
      setOtaStatus('Erreur OTA : ' + e.message);
      setOtaProgress(-1);
    }
  };

  const handleSaveDescription = async (id: string) => {
    try {
      await apiService.updateBackupDescription(id, descInputValue);
      store.setSuccess('Description mise à jour !');
      loadBackups();
    } catch (e: any) {
      store.setError('Erreur lors de la mise à jour : ' + e.message);
    } finally {
      setEditingDescId(null);
    }
  };

  const loadBackups = async () => {
    try {
      const list = await apiService.getBackups();
      setBackups(list);
    } catch (e: any) {
      store.setError('Erreur lors du chargement des sauvegardes : ' + e.message);
    }
  };

  const handleCreateBackup = async () => {
    setIsBackingUp(true);
    try {
      const nowTs = Math.floor(Date.now() / 1000);
      await apiService.createBackup(nowTs);
      store.setSuccess('Sauvegarde créée avec succès');
      loadBackups();
    } catch (e: any) {
      store.setError('Erreur lors de la création de la sauvegarde : ' + e.message);
    } finally {
      setIsBackingUp(false);
    }
  };

  const handleDeleteBackup = async (id: string) => {
    if (!confirm('Êtes-vous sûr de vouloir supprimer cette sauvegarde ?')) return;
    try {
      await apiService.deleteBackup(id);
      store.setSuccess('Sauvegarde supprimée');
      loadBackups();
    } catch (e: any) {
      store.setError('Erreur lors du chargement ou suppression : ' + e.message);
    }
  };

  const handleRestoreBackup = async (id: string) => {
    if (!confirm('Êtes-vous sûr de vouloir restaurer cette sauvegarde ? L\'ensemble de la configuration actuelle sera remplacée et le système va être réinitialisé.')) return;
    setIsRestoring(id);
    try {
      await apiService.restoreBackup(id);
      store.setSuccess('Restauration en cours... Le système redémarre. Veuillez patienter.');
      // The ESP32 will reboot after restoring - wait and reload
      setTimeout(() => {
        window.location.reload();
      }, 8000);
    } catch (e: any) {
      // Even a Network Error here can be normal if the ESP rebooted before responding
      store.setSuccess('Restauration envoyée. Le système redémarre... Veuillez patienter.');
      setTimeout(() => {
        window.location.reload();
      }, 8000);
    } finally {
      setIsRestoring(null);
    }
  };

  const handleUploadBackup = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    if (!confirm('Êtes-vous sûr de vouloir importer ce fichier de sauvegarde ? L\'ensemble de la configuration actuelle sera écrasée et le système réinitialisé.')) {
      event.target.value = '';
      return;
    }

    setIsUploading(true);
    const reader = new FileReader();
    reader.onload = async (e) => {
      try {
        const jsonContent = JSON.parse(e.target?.result as string);
        if (!jsonContent.backupVersion && !jsonContent.profiles && !jsonContent.setupsList) {
          throw new Error('Le format du fichier de sauvegarde semble invalide.');
        }
        await apiService.uploadBackup(jsonContent);
        store.setSuccess('Sauvegarde importée et restaurée avec succès !');
        loadConfig();
        loadBackups();
      } catch (err: any) {
        store.setError('Erreur lors de l\'importation : ' + err.message);
      } finally {
        setIsUploading(false);
        event.target.value = '';
      }
    };
    reader.readAsText(file);
  };

  return (
    <div className="space-y-6">
      <h2 className="text-2xl font-bold text-white">Paramètres Système</h2>
      
      <Card>
        <h3 className="text-lg font-bold text-white mb-4">Réseau & Accès</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-4">
          <Input 
            label="Nom du réseau WiFi (SSID)" 
            value={config.wifiSSID ?? ''} 
            onChange={e => setConfig({...config, wifiSSID: e.target.value})} 
          />
          <Input 
            label="Mot de passe WiFi" 
            type="password" 
            value={config.wifiPassword ?? ''} 
            onChange={e => setConfig({...config, wifiPassword: e.target.value})} 
          />
          <Input 
            label="Code PIN Administrateur" 
            type="password" 
            maxLength={4} 
            value={config.adminPin ?? ''} 
            onChange={e => setConfig({...config, adminPin: e.target.value})} 
          />
        </div>
      </Card>

      <Card>
        <h3 className="text-lg font-bold text-white mb-4">Paramètres DMX</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-4">
          <Input 
            label="DMX Baud Rate (Standard: 250000)" 
            type="number" 
            value={config.dmxBaud ?? 250000} 
            onChange={e => setConfig({...config, dmxBaud: parseInt(e.target.value) || 250000})} 
          />
          <Input 
            label="Intervalle de mise à jour DMX (ms)" 
            type="number" 
            value={config.updateInterval ?? 40} 
            onChange={e => setConfig({...config, updateInterval: parseInt(e.target.value) || 40})} 
          />
        </div>
      </Card>


      <Card>
        <div className="flex justify-between items-center mb-4">
          <h3 className="text-lg font-bold text-white flex items-center gap-2">
            <Database className="text-purple-500" size={22} /> Sauvegardes & Restauration
          </h3>
          <Button 
            variant="outline" 
            size="sm" 
            onClick={loadBackups}
            className="flex items-center gap-1 text-xs"
          >
            <RefreshCw size={14} /> Rafraîchir
          </Button>
        </div>

        <p className="text-sm text-slate-300 mb-6">
          Sauvegardez l'intégralité du système (configurations WiFi, catalogue des projecteurs, configurations de scènes, shows, etc.). Les sauvegardes sont stockées sur la mémoire flash interne et peuvent être téléchargées sur votre PC.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
          <Button 
            variant="primary" 
            onClick={handleCreateBackup}
            disabled={isBackingUp}
            className="flex items-center justify-center gap-2"
          >
            <Database size={18} /> 
            {isBackingUp ? 'Création de la sauvegarde...' : 'Créer une sauvegarde interne'}
          </Button>
          
          <div className="relative">
            <input
              type="file"
              id="upload-backup-file"
              accept=".json"
              onChange={handleUploadBackup}
              disabled={isUploading}
              className="hidden"
            />
            <label
              htmlFor="upload-backup-file"
              className={`w-full flex items-center justify-center gap-2 font-semibold rounded-lg px-4 py-2 text-base border-2 border-dashed border-purple-600 text-purple-400 hover:bg-purple-600/10 transition-all duration-200 cursor-pointer text-center ${isUploading ? 'opacity-50 pointer-events-none' : ''}`}
            >
              <Upload size={18} />
              {isUploading ? 'Importation...' : 'Importer un fichier (.json)'}
            </label>
          </div>
        </div>

        <div>
          <h4 className="font-semibold text-white mb-3 text-sm">Sauvegardes disponibles</h4>
          {backups.length === 0 ? (
            <div className="border border-slate-800 rounded-lg p-6 text-center text-slate-500 text-sm">
              Aucune sauvegarde enregistrée sur l'appareil.
            </div>
          ) : (
            <div className="space-y-3 max-h-[300px] overflow-y-auto pr-1">
              {backups.map((bk) => {
                const date = new Date(bk.timestamp * 1000);
                const formattedDate = date.toLocaleString('fr-FR', {
                  day: '2-digit',
                  month: '2-digit',
                  year: 'numeric',
                  hour: '2-digit',
                  minute: '2-digit',
                  second: '2-digit'
                });
                const formattedSize = (bk.size / 1024).toFixed(1) + ' KB';

                return (
                  <div key={bk.id} className="flex flex-col sm:flex-row justify-between items-start sm:items-center p-3 rounded-lg bg-slate-800/50 border border-slate-800 hover:border-slate-700 transition-colors gap-3">
                    <div className="flex items-center gap-3 flex-1">
                      <div className="p-2 rounded bg-purple-600/10 text-purple-400 self-start mt-1">
                        <Database size={18} />
                      </div>
                      <div className="flex-1">
                        <div className="flex items-center gap-2">
                          <div className="text-white text-sm font-semibold">{formattedDate}</div>
                          <div className="text-xs text-slate-400">ID: {bk.id}</div>
                          <div className="text-xs text-slate-400 hidden sm:block">| Taille: {formattedSize}</div>
                        </div>
                        
                        {editingDescId === bk.id ? (
                          <div className="flex items-center gap-2 mt-1">
                            <Input 
                              value={descInputValue} 
                              onChange={(e) => setDescInputValue(e.target.value)}
                              placeholder="Description de la sauvegarde..."
                              className="h-7 text-sm py-0 w-full max-w-[300px]"
                              autoFocus
                              onKeyDown={(e) => {
                                if (e.key === 'Enter') handleSaveDescription(bk.id);
                                if (e.key === 'Escape') setEditingDescId(null);
                              }}
                            />
                            <button onClick={() => handleSaveDescription(bk.id)} className="text-emerald-400 hover:text-emerald-300 p-1 bg-slate-800 rounded">
                              <Check size={14} />
                            </button>
                            <button onClick={() => setEditingDescId(null)} className="text-rose-400 hover:text-rose-300 p-1 bg-slate-800 rounded">
                              <X size={14} />
                            </button>
                          </div>
                        ) : (
                          <div className="flex items-center gap-2 mt-1">
                            <div className="text-sm text-slate-300 italic">
                              {bk.description || "Aucune description"}
                            </div>
                            <button 
                              onClick={() => {
                                setEditingDescId(bk.id);
                                setDescInputValue(bk.description || "");
                              }}
                              className="text-slate-500 hover:text-slate-300 transition-colors p-1"
                              title="Modifier la description"
                            >
                              <Edit2 size={12} />
                            </button>
                          </div>
                        )}
                      </div>
                    </div>
                    
                    <div className="flex items-center gap-2 w-full sm:w-auto justify-end">
                      <a
                        href={`/api/backups/${bk.id}`}
                        download={bk.filename}
                        className="flex items-center gap-1.5 px-3 py-1.5 text-xs font-semibold rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 hover:text-white transition-all cursor-pointer"
                      >
                        <Download size={14} /> Télécharger
                      </a>
                      <Button
                        variant="secondary"
                        size="sm"
                        disabled={isRestoring !== null}
                        onClick={() => handleRestoreBackup(bk.id)}
                        className="flex items-center gap-1 px-3 py-1.5 text-xs font-semibold"
                      >
                        <RefreshCw size={14} className={isRestoring === bk.id ? 'animate-spin' : ''} />
                        {isRestoring === bk.id ? 'Restauration...' : 'Restaurer'}
                      </Button>
                      <Button
                        variant="danger"
                        size="sm"
                        onClick={() => handleDeleteBackup(bk.id)}
                        className="flex items-center gap-1 px-2.5 py-1.5 text-xs font-semibold"
                      >
                        <Trash2 size={14} />
                      </Button>
                    </div>
                  </div>
                );
              })}
            </div>
          )}
        </div>
      </Card>

      <Card>
        <h3 className="text-lg font-bold text-white mb-4">Mise à jour sans fil (OTA)</h3>
        <p className="text-sm text-slate-300 mb-4">Sélectionnez les fichiers compilés (.bin) pour flasher l'appareil par Wi-Fi.</p>
        
        {otaProgress >= 0 && (
          <div className="mb-4">
            <div className="flex justify-between text-sm text-slate-300 mb-1">
              <span>{otaStatus}</span>
              <span>{otaProgress}%</span>
            </div>
            <div className="w-full bg-slate-700 rounded-full h-2.5">
              <div className="bg-sky-500 h-2.5 rounded-full" style={{ width: `${otaProgress}%` }}></div>
            </div>
          </div>
        )}

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="border border-slate-600 rounded-lg p-4">
            <h4 className="font-semibold text-white mb-2">Mettre à jour le Firmware</h4>
            <p className="text-xs text-slate-400 mb-4">Programme principal C++ (firmware.bin)</p>
            <input 
              type="file" 
              accept=".bin" 
              onChange={(e) => handleOTA(e, 'firmware')} 
              className="text-sm text-slate-300 file:mr-4 file:py-2 file:px-4 file:rounded-full file:border-0 file:text-sm file:font-semibold file:bg-sky-50 file:text-sky-700 hover:file:bg-sky-100 cursor-pointer"
            />
          </div>
          <div className="border border-slate-600 rounded-lg p-4">
            <h4 className="font-semibold text-white mb-2">Mettre à jour l'Interface</h4>
            <p className="text-xs text-slate-400 mb-4">Fichiers Web React (spiffs.bin)</p>
            <input 
              type="file" 
              accept=".bin" 
              onChange={(e) => handleOTA(e, 'spiffs')} 
              className="text-sm text-slate-300 file:mr-4 file:py-2 file:px-4 file:rounded-full file:border-0 file:text-sm file:font-semibold file:bg-emerald-50 file:text-emerald-700 hover:file:bg-emerald-100 cursor-pointer"
            />
          </div>
        </div>
      </Card>

      <div className="flex gap-4">
        <Button variant="primary" onClick={handleSaveConfig} className="flex-1 flex items-center justify-center gap-2">
          <Save size={20} /> Enregistrer la Configuration
        </Button>
        <Button variant="danger" onClick={handleReboot} className="flex items-center justify-center gap-2">
          <AlertTriangle size={20} /> Redémarrer
        </Button>
      </div>
    </div>
  );
};
