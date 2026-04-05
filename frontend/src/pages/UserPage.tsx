import { useEffect, useState, useRef, useCallback } from 'react';
import { Volume2, Mic } from 'lucide-react';
import { Card, Slider, LoadingSpinner, ErrorNotification } from '@/components/ui';
import {
  EffectButtons,
  SceneGrid,
  AmbianceGrid,
  ShowGrid,
  ConnectionIndicator,
  SoundPanel,
} from '@/components/index';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';

export const UserPage: React.FC = () => {
  const store = useAppStore();
  const [loading, setLoading] = useState(true);
  const [connected, setConnected] = useState(false);
  const strobeHeld = useRef(false);
  const smokeHeld = useRef(false);

  useEffect(() => {
    loadUserData();
    const interval = setInterval(loadUserData, 2000);
    return () => clearInterval(interval);
  }, []);

  const loadUserData = async () => {
    try {
      const [scenes, ambiances, shows, state] = await Promise.all([
        apiService.getScenes(),
        apiService.getAmbiances(),
        apiService.getShows(),
        apiService.getLightingState(),
      ]);

      store.setScenes(scenes);
      store.setAmbiances(ambiances);
      store.setShows(shows);
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

  const handleSceneSelect = async (sceneId: string) => {
    try {
      await apiService.activateScene(sceneId);
      await loadUserData();
    } catch (error) {
      store.setError('Erreur d\'activation de la scène');
    }
  };

  const handleShowStart = async (showId: string) => {
    try {
      await apiService.startShow(showId);
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
      await apiService.setSoundMode(store.lightingState.soundMode ?? 0, sens);
      store.setLightingState({ ...store.lightingState, soundSensitivity: sens });
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
      <ErrorNotification />

      {/* Header */}
      <div className="max-w-6xl mx-auto mb-8">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-4xl md:text-5xl font-bold text-white mb-2">✨ Contrôle Éclairage</h1>
            <p className="text-slate-400">Créez l'ambiance parfaite pour votre événement</p>
          </div>
          <ConnectionIndicator connected={connected} />
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

        {/* Sound-Reactive Mode */}
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
            audio={store.lightingState.audio}
            onModeChange={handleSoundModeChange}
            onSensitivityChange={handleSoundSensitivity}
          />
        </Card>

        {/* Color Scenes */}
        {store.scenes.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🎨 Scènes Couleur</h2>
            <SceneGrid
              scenes={store.scenes}
              onSceneSelect={handleSceneSelect}
              activeSceneId={store.lightingState.activeScene}
            />
          </Card>
        )}

        {/* Ambiances */}
        {store.ambiances.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🌙 Ambiances</h2>
            <AmbianceGrid
              ambiances={store.ambiances}
              onAmbianceSelect={(id) => console.log('Ambiance selected:', id)}
              activeAmbianceId={store.lightingState.activeAmbiance}
            />
          </Card>
        )}

        {/* Dynamic Shows */}
        {store.shows.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🎆 Shows Dynamiques</h2>
            <ShowGrid
              shows={store.shows}
              onShowStart={handleShowStart}
              activeShowId={store.lightingState.activeShow}
              isRunning={!!store.lightingState.activeShow}
            />
          </Card>
        )}
      </div>
    </div>
  );
};
