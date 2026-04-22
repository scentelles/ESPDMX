import { useState, useEffect } from 'react';
import { Save, AlertTriangle } from 'lucide-react';
import { Button, Card, Input } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { SystemConfig } from '@/types';

export const SettingsTab = () => {
  const store = useAppStore();
  const [config, setConfig] = useState<Partial<SystemConfig>>({});
  const [otaProgress, setOtaProgress] = useState<number>(-1);
  const [otaStatus, setOtaStatus] = useState<string>('');

  useEffect(() => {
    loadConfig();
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
      store.setError('Configuration système enregistrée (redémarrage peut être nécessaire)');
    } catch (error: any) {
      store.setError(error.message);
    }
  };

  const handleReboot = async () => {
    if (!confirm('Redémarrer le contrôleur ESP32 ?')) return;
    try {
      await apiService.reboot();
      store.setError('Redémarrage en cours...');
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
