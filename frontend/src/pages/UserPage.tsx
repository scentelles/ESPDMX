import { useEffect, useState, useRef, useCallback } from 'react';
import { Volume2, Mic } from 'lucide-react';
import { Card, Slider, LoadingSpinner } from '@/components/ui';
import {
  EffectButtons,
  SceneGrid,
  ShowGrid,
  ConnectionIndicator,
  SoundPanel,
} from '@/components/index';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';
import { ConsoleVirtualTab } from './admin/ConsoleVirtualTab';

export const UserPage: React.FC = () => {
  const store = useAppStore();
  const [loading, setLoading] = useState(true);
  const [connected, setConnected] = useState(false);
  const strobeHeld = useRef(false);
  const smokeHeld = useRef(false);

  useEffect(() => {
    const init = async () => {
      try {
        const profiles = await apiService.getProfiles();
        store.setProfiles(profiles);
      } catch (e) {
        console.error('Failed to load profiles', e);
      }
      loadUserData();
    };
    init();
    const interval = setInterval(loadUserData, 2000);
    return () => clearInterval(interval);
  }, []);

  const loadUserData = async () => {
    try {
      const [setup, state] = await Promise.all([
        apiService.getActiveSetup(),
        apiService.getLightingState(),
      ]);

      store.setActiveSetup(setup);
      store.setLightingState(state);
      setConnected(true);
      setLoading(false);
    } catch (error) {
      setConnected(false);
      store.setError('Erreur de chargement des données');
      console.error(error);
      setLoading(false);
    }
  };

  const reapplyVirtualConsole = () => {
    if (!store.activeSetup) return;
    for (const vg of store.activeSetup.virtualGroups) {
      const dimmerVal = store.groupValues[vg.id];
      if (dimmerVal !== undefined) {
        apiService.setVirtualGroupValue(vg.id, dimmerVal);
      }
      
      const hex = store.groupColors[vg.id];
      if (hex) {
        const r = parseInt(hex.substring(1, 3), 16);
        const g = parseInt(hex.substring(3, 5), 16);
        const b = parseInt(hex.substring(5, 7), 16);
        const instanceIds = Array.from(new Set(vg.assignments.map(a => a.instanceId)));
        
        for (const instId of instanceIds) {
          const inst = store.activeSetup.instances.find(i => i.id === instId);
          if (!inst) continue;
          const profile = store.profiles.find(p => p.id === inst.profileId);
          if (!profile) continue;
          
          const chRs = profile.channels.filter(c => /red|rouge|\br\b/i.test(c.name)).sort((a,b) => a.offset - b.offset);
          const chGs = profile.channels.filter(c => /green|vert|\bg\b/i.test(c.name)).sort((a,b) => a.offset - b.offset);
          const chBs = profile.channels.filter(c => /blue|bleu|\bb\b/i.test(c.name)).sort((a,b) => a.offset - b.offset);
          
          for (let idx = 0; idx < Math.max(chRs.length, chGs.length, chBs.length); idx++) {
            if (chRs[idx]) apiService.setDMXChannel(inst.dmxAddress + chRs[idx].offset, r);
            if (chGs[idx]) apiService.setDMXChannel(inst.dmxAddress + chGs[idx].offset, g);
            if (chBs[idx]) apiService.setDMXChannel(inst.dmxAddress + chBs[idx].offset, b);
          }
        }
      }
    }
  };

  const handleSceneSelect = async (sceneId: string) => {
    try {
      const isDeactivating = store.lightingState.activeScene === sceneId || store.lightingState.activeSceneG2 === sceneId;
      await apiService.activateScene(sceneId);
      if (isDeactivating) {
        setTimeout(reapplyVirtualConsole, 50);
      }
      await loadUserData();
    } catch (error) {
      store.setError('Erreur d\'activation de la scène');
    }
  };

  const handleShowStart = async (showId: string) => {
    try {
      const isDeactivating = store.lightingState.activeShow === showId;
      await apiService.startShow(showId);
      if (isDeactivating) {
        setTimeout(reapplyVirtualConsole, 50);
      }
      await loadUserData();
    } catch (error) {
      store.setError('Erreur de démarrage du show');
    }
  };

  const handleBrightnessChange = async (brightness: number) => {
    try {
      await apiService.setMasterBrightness(brightness);
      store.setLightingState({
        ...store.lightingState,
        masterBrightness: brightness,
      });
    } catch (error) {
      store.setError('Erreur de réglage de la luminosité');
    }
  };

  const handleStrobeDown = useCallback(async () => {
    if (strobeHeld.current) return;
    strobeHeld.current = true;
    try {
      await apiService.triggerStrobe(5);
      store.setLightingState({ ...store.lightingState, strobeActive: true });
    } catch { store.setError('Erreur stroboscope'); }
  }, [store.lightingState]);

  const handleStrobeUp = useCallback(async () => {
    if (!strobeHeld.current) return;
    strobeHeld.current = false;
    try {
      await apiService.stopStrobe();
      store.setLightingState({ ...store.lightingState, strobeActive: false });
    } catch { /* ignore */ }
  }, [store.lightingState]);

  const handleSmokeDown = useCallback(async () => {
    if (smokeHeld.current) return;
    smokeHeld.current = true;
    try {
      await apiService.triggerSmoke(60000); // long duration, will be cut on release
      store.setLightingState({ ...store.lightingState, smokeActive: true });
    } catch { store.setError('Erreur fumée'); }
  }, [store.lightingState]);

  const handleSmokeUp = useCallback(async () => {
    if (!smokeHeld.current) return;
    smokeHeld.current = false;
    try {
      await apiService.stopSmoke();
      store.setLightingState({ ...store.lightingState, smokeActive: false });
    } catch { /* ignore */ }
  }, [store.lightingState]);

  const handleSoundModeChange = useCallback(async (mode: number) => {
    try {
      await apiService.setSoundMode(mode);
      store.setLightingState({ ...store.lightingState, soundMode: mode });
    } catch {
      store.setError('Erreur mode son');
    }
  }, [store.lightingState]);

  const handleSoundSensitivity = useCallback(async (sens: number) => {
    try {
      await apiService.setSoundMode(store.lightingState.soundMode ?? 0, sens, store.lightingState.soundDynamics ?? 0);
      store.setLightingState({ ...store.lightingState, soundSensitivity: sens });
    } catch { /* ignore */ }
  }, [store.lightingState]);

  const handleSoundDynamics = useCallback(async (dyn: number) => {
    try {
      await apiService.setSoundMode(store.lightingState.soundMode ?? 0, store.lightingState.soundSensitivity ?? 5, dyn);
      store.setLightingState({ ...store.lightingState, soundDynamics: dyn });
    } catch { /* ignore */ }
  }, [store.lightingState]);

  if (loading) {
    return (
      <div className="flex items-center justify-center min-h-screen">
        <LoadingSpinner size="lg" />
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-950 via-purple-900 to-slate-950 p-4 md:p-8">
      {/* Header */}
      <div className="max-w-6xl mx-auto mb-8">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-4xl md:text-5xl font-bold text-white mb-2">✨ SUD SHOW CONTROL</h1>
            <p className="text-slate-400">Créez l'ambiance parfaite pour votre événement</p>
          </div>
          <div className="flex flex-col items-end gap-2">
            <ConnectionIndicator connected={connected} label={connected ? 'ESP32 Connecté' : 'ESP32 Déconnecté'} />
            {connected && (
              <ConnectionIndicator 
                connected={store.lightingState.pedalConnected ?? false} 
                label={store.lightingState.pedalConnected ? 'Pédalier Connecté' : 'Pédalier Déconnecté'} 
              />
            )}
          </div>
        </div>
      </div>

      <div className="max-w-6xl mx-auto space-y-8">
        {/* Master Brightness */}
        <Card>
          <div className="flex items-center gap-4 mb-4">
            <Volume2 size={24} className="text-purple-400" />
            <h2 className="text-xl font-bold text-white">Luminosité Générale</h2>
          </div>
          <Slider
            label="Intensité Lumineuse"
            value={store.lightingState.masterBrightness}
            onChange={handleBrightnessChange}
          />
        </Card>

        {/* Virtual Console */}
        {store.activeSetup?.virtualGroups && store.activeSetup.virtualGroups.length > 0 && (
           <ConsoleVirtualTab />
        )}

        {/* Effect Buttons */}
        <Card>
          <h2 className="text-xl font-bold text-white mb-4">Effets Rapides</h2>
          <EffectButtons
            onStrobeDown={handleStrobeDown}
            onStrobeUp={handleStrobeUp}
            onSmokeDown={handleSmokeDown}
            onSmokeUp={handleSmokeUp}
            strobeActive={store.lightingState.strobeActive}
            smokeActive={store.lightingState.smokeActive}
          />
        </Card>


        {/* Color Scenes */}
        {store.activeSetup?.scenes && store.activeSetup.scenes.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🎨 Scènes Couleur</h2>
            <SceneGrid
              scenes={store.activeSetup.scenes}
              onSceneSelect={handleSceneSelect}
              activeSceneIds={[store.lightingState.activeScene, store.lightingState.activeSceneG2].filter(Boolean) as string[]}
            />
          </Card>
        )}

        {/* Dynamic Shows */}
        {store.activeSetup?.shows && store.activeSetup.shows.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🎆 Shows Dynamiques</h2>
            <ShowGrid
              shows={store.activeSetup.shows}
              onShowStart={handleShowStart}
              activeShowId={store.lightingState.activeShow}
              isRunning={!!store.lightingState.activeShow}
            />
          </Card>
        )}

        {/* Sound-Reactive Mode (moved) */}
        <Card>
          <div className="flex items-center gap-4 mb-4">
            <Mic size={24} className="text-cyan-400" />
            <h2 className="text-xl font-bold text-white">Mode Son Réactif</h2>
            {(store.lightingState.soundMode ?? 0) > 0 && (
              <div className="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse ml-auto" />
            )}
          </div>
          <SoundPanel
            soundMode={store.lightingState.soundMode ?? 0}
            sensitivity={store.lightingState.soundSensitivity ?? 5}
            dynamics={store.lightingState.soundDynamics ?? 0}
            audio={store.lightingState.audio}
            onModeChange={handleSoundModeChange}
            onSensitivityChange={handleSoundSensitivity}
            onDynamicsChange={handleSoundDynamics}
          />
        </Card>
      </div>
    </div>
  );
};
