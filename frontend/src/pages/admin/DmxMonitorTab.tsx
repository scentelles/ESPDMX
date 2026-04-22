import { useState, useEffect, useMemo } from 'react';
import { Card } from '@/components/ui';
import { apiService } from '@/services/api';
import { useAppStore } from '@/store';
import type { FixtureInstance } from '@/types';

export const DmxMonitorTab = () => {
  const store = useAppStore();
  const [dmxValues, setDmxValues] = useState<number[]>(new Array(512).fill(0));

  useEffect(() => {
    // Poll API every 500ms for live DMX values
    let isMounted = true;
    const fetchDMX = async () => {
      try {
        const state = await apiService.getLightingState();
        if (state && state.dmxOutput && isMounted) {
          // Pad with 0s if less than 512
          const fullArray = new Array(512).fill(0);
          for (let i = 0; i < state.dmxOutput.length && i < 512; i++) {
            fullArray[i] = state.dmxOutput[i];
          }
          setDmxValues(fullArray);
        }
      } catch (e) {
        // Ignore silent polling errors
      }
    };

    fetchDMX();
    const interval = setInterval(fetchDMX, 500);
    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, []);

  // Compute a map of channel -> Fixture Instance for highlighting
  const channelMap = useMemo(() => {
    const map: (FixtureInstance | null)[] = new Array(512).fill(null);
    const activeSetup = store.activeSetup;
    const profiles = store.profiles;

    if (!activeSetup || !profiles) return map;

    for (const instance of activeSetup.instances) {
      if (!instance.enabled) continue;
      const profile = profiles.find(p => p.id === instance.profileId);
      if (!profile) continue;

      const address = instance.dmxAddress;
      const count = profile.channelCount;
      
      for (let i = address; i < address + count && i <= 512; i++) {
        map[i - 1] = instance;
      }
    }
    return map;
  }, [store.activeSetup, store.profiles]);

  // Generate distinct colors based on instance ID to differentiate adjacent fixtures
  const getFixtureColor = (id: string) => {
    let hash = 0;
    for (let i = 0; i < id.length; i++) {
      hash = id.charCodeAt(i) + ((hash << 5) - hash);
    }
    const hue = Math.abs(hash) % 360;
    return `hsla(${hue}, 70%, 30%, 0.5)`; // Soft dark tint
  };

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center bg-slate-900 p-4 rounded-lg border border-slate-800">
        <h2 className="text-xl font-bold text-white">Moniteur DMX</h2>
        <div className="flex items-center gap-2 text-sm">
           <span className="w-3 h-3 rounded-full bg-green-500 animate-pulse"></span>
           <span className="text-slate-400">En direct (Rafraîchissement 500ms)</span>
        </div>
      </div>

      <Card className="p-6 overflow-x-auto">
        <div className="min-w-[800px]">
          <div className="grid grid-cols-[repeat(16,minmax(0,1fr))] gap-2">
            {dmxValues.map((val, index) => {
              const channel = index + 1;
              const instance = channelMap[index];
              const isFixtureStart = instance && (channel === instance.dmxAddress);

              return (
                <div 
                  key={index} 
                  className={`flex flex-col items-center justify-center p-2 rounded border border-slate-700/50 relative group transition-colors`}
                  style={{ backgroundColor: instance ? undefined : '', ...(instance ? { backgroundColor: getFixtureColor(instance.id) } : {}) }}
                  title={instance ? `${instance.name} (Ch ${channel - instance.dmxAddress + 1})` : `Canal ${channel}`}
                >
                  <div className="text-[10px] text-slate-400 absolute top-1 left-1 opacity-70">
                    {channel}
                  </div>
                  <div className="text-lg font-mono font-bold text-white mt-1">
                    {val}
                  </div>
                  
                  {isFixtureStart && (
                    <div className="absolute -top-3 left-0 bg-slate-950 text-[9px] text-slate-300 font-bold px-1 rounded whitespace-nowrap overflow-hidden max-w-[80px] truncate border border-slate-700 z-10">
                      {instance.name}
                    </div>
                  )}
                  {/* Tooltip on hover */}
                  {instance && (
                     <div className="hidden group-hover:block absolute z-20 bottom-full left-1/2 -translate-x-1/2 mb-1 bg-slate-900 text-white text-xs px-2 py-1 rounded border border-slate-700 whitespace-nowrap shadow-xl">
                       {instance.name} - Ch {channel - instance.dmxAddress + 1}
                     </div>
                  )}
                </div>
              );
            })}
          </div>
        </div>
      </Card>
    </div>
  );
};
