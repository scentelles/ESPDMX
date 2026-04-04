import { useEffect, useState } from 'react';
import { Volume2 } from 'lucide-react';
import { Card, Slider, LoadingSpinner, ErrorNotification } from '@/components/ui';
import {
  EffectButtons,
  SceneGrid,
  AmbianceGrid,
  ShowGrid,
} from '@/components/index';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';

export const UserPage: React.FC = () => {
  const store = useAppStore();
  const [loading, setLoading] = useState(true);

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
      setLoading(false);
    } catch (error) {
      store.setError('Failed to load data');
      console.error(error);
      setLoading(false);
    }
  };

  const handleSceneSelect = async (sceneId: string) => {
    try {
      await apiService.activateScene(sceneId);
      await loadUserData();
    } catch (error) {
      store.setError('Failed to activate scene');
    }
  };

  const handleShowStart = async (showId: string) => {
    try {
      await apiService.startShow(showId);
      await loadUserData();
    } catch (error) {
      store.setError('Failed to start show');
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
      store.setError('Failed to set brightness');
    }
  };

  const handleStrobePress = async () => {
    try {
      await apiService.triggerStrobe(5); // speed parameter
      store.setLightingState({
        ...store.lightingState,
        strobeActive: true,
      });
    } catch (error) {
      store.setError('Failed to trigger strobe');
    }
  };

  const handleSmokePress = async () => {
    try {
      await apiService.triggerSmoke(3000); // 3 second duration
      store.setLightingState({
        ...store.lightingState,
        smokeActive: true,
      });
    } catch (error) {
      store.setError('Failed to trigger smoke');
    }
  };

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
        <h1 className="text-4xl md:text-5xl font-bold text-white mb-2">✨ Lighting Control</h1>
        <p className="text-slate-400">Create the perfect atmosphere for your event</p>
      </div>

      <div className="max-w-6xl mx-auto space-y-8">
        {/* Master Brightness */}
        <Card>
          <div className="flex items-center gap-4 mb-4">
            <Volume2 size={24} className="text-purple-400" />
            <h2 className="text-xl font-bold text-white">Master Brightness</h2>
          </div>
          <Slider
            label="Overall Light Intensity"
            value={store.lightingState.masterBrightness}
            onChange={handleBrightnessChange}
          />
        </Card>

        {/* Effect Buttons */}
        <Card>
          <h2 className="text-xl font-bold text-white mb-4">Quick Effects</h2>
          <EffectButtons
            onStrobeClick={handleStrobePress}
            onSmokeClick={handleSmokePress}
            strobeActive={store.lightingState.strobeActive}
            smokeActive={store.lightingState.smokeActive}
          />
        </Card>

        {/* Color Scenes */}
        {store.scenes.length > 0 && (
          <Card>
            <h2 className="text-xl font-bold text-white mb-4">🎨 Color Scenes</h2>
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
            <h2 className="text-xl font-bold text-white mb-4">🎆 Dynamic Shows</h2>
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
