import { useState, useEffect } from 'react';
import { Save, AlertTriangle } from 'lucide-react';
import { Button, Card, Input } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import type { SystemConfig } from '@/types';

export const SettingsTab = () => {
  const store = useAppStore();
  const [config, setConfig] = useState<Partial<SystemConfig>>({});

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
