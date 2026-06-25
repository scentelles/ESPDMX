import { useState } from 'react';
import { Card, Alert } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';

export const ConsoleVirtualTab = () => {
  const store = useAppStore();
  const [activePicker, setActivePicker] = useState<string | null>(null);

  const PRESET_COLORS = [
    '#ff0000', '#ff8c00', '#ffd700', '#ffff00', // Red, Orange, Gold, Yellow
    '#7fff00', '#00ff00', '#00fa9a', '#00ffff', // Chartreuse, Green, MediumSpringGreen, Cyan
    '#00bfff', '#0000ff', '#8a2be2', '#ff00ff', // DeepSkyBlue, Blue, BlueViolet, Magenta
    '#ff1493', '#ff69b4', '#ffffff', '#ffebcd'  // DeepPink, HotPink, White, BlanchedAlmond
  ];

  const activeSetup = store.activeSetup;

  if (!activeSetup) {
    return <Alert variant="warning">Aucune configuration active. Allez dans "Setups" pour en activer une.</Alert>;
  }

  const groupHasRGB = (vg: typeof activeSetup.virtualGroups[0]): boolean => {
    const instanceIds = Array.from(new Set(vg.assignments.map(a => a.instanceId)));
    for (const instId of instanceIds) {
      const inst = activeSetup.instances.find(i => i.id === instId);
      if (!inst) continue;
      const profile = store.profiles.find(p => p.id === inst.profileId);
      if (!profile) continue;
      const hasR = profile.channels.some(c => /red|rouge|\br\b/i.test(c.name));
      const hasG = profile.channels.some(c => /green|vert|\bg\b/i.test(c.name));
      const hasB = profile.channels.some(c => /blue|bleu|\bb\b/i.test(c.name));
      if (hasR && hasG && hasB) return true;
    }
    return false;
  };

  const handleChange = (vgId: string, val: number) => {
    store.setGroupValues({ ...store.groupValues, [vgId]: val });
    apiService.setVirtualGroupValue(vgId, val);
  };

  const handleColorChange = (vgId: string, hex: string) => {
     store.setGroupColors({ ...store.groupColors, [vgId]: hex });
     
     if (!activeSetup) return;
     const r = parseInt(hex.substring(1, 3), 16);
     const g = parseInt(hex.substring(3, 5), 16);
     const b = parseInt(hex.substring(5, 7), 16);

     const vg = activeSetup.virtualGroups.find(v => v.id === vgId);
     if (!vg) return;

     const instanceIds = Array.from(new Set(vg.assignments.map(a => a.instanceId)));

     for (const instId of instanceIds) {
       const inst = activeSetup.instances.find(i => i.id === instId);
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
  };

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center bg-slate-900 p-4 rounded-lg">
        <h2 className="text-xl font-bold text-white">Console Virtuelle (Groupes)</h2>
      </div>

      {activeSetup.virtualGroups.length === 0 ? (
         <Alert variant="info">Aucun groupe virtuel n'est défini dans cette configuration. Allez dans l'onglet "Groupes Virtuels" pour en créer.</Alert>
      ) : (
         <Card className="p-6">
           <div className="flex gap-6 overflow-x-auto pb-4">
             {activeSetup.virtualGroups.map(vg => {
               const hasRGB = groupHasRGB(vg);
               return (
               <div key={vg.id} className="flex flex-col items-center gap-3 min-w-[100px] h-full">
                 <div className="font-medium text-white text-sm text-center line-clamp-2 h-10 w-28 leading-snug">
                    {vg.name}
                 </div>
                 
                 <div className="flex flex-col items-center mt-auto gap-3">
                   <div className="h-72 flex items-end justify-center w-20 bg-slate-900 rounded-full py-4 shadow-inner border border-slate-800">
                     <input
                       {...{ orient: "vertical" } as any}
                       type="range"
                       className="w-full h-full slider-virtual-vg appearance-none bg-transparent cursor-pointer"
                       style={{ WebkitAppearance: 'slider-vertical' } as any}
                       min={0}
                       max={255}
                       value={store.groupValues[vg.id] || 0}
                       onChange={e => handleChange(vg.id, parseInt(e.target.value) || 0)}
                     />
                   </div>
                   <input
                     type="number"
                     min={0}
                     max={255}
                     className="w-16 text-center bg-slate-800 text-white font-mono rounded border border-slate-700 mt-2"
                     value={store.groupValues[vg.id] || 0}
                     onChange={e => handleChange(vg.id, parseInt(e.target.value) || 0)}
                   />
                   <div className="text-xs text-slate-500 mt-1 mb-2">
                      {vg.assignments.length} proj.
                   </div>
                   {hasRGB && (
                   <div className="mt-1">
                       <button
                         onClick={() => setActivePicker(vg.id)}
                         style={{ backgroundColor: store.groupColors[vg.id] || '#ffffff' }}
                         className="w-8 h-8 rounded-full cursor-pointer border-2 border-slate-700 shadow-sm outline-none transition-transform hover:scale-105" 
                         title="Couleur des projecteurs de ce groupe"
                      />
                   </div>
                   )}
                 </div>
               </div>
               );
             })}
           </div>
         </Card>
      )}

      {activePicker && (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/70 p-4" onClick={() => setActivePicker(null)}>
           <div className="bg-slate-800 border border-slate-600 rounded-xl p-4 shadow-2xl max-w-sm w-full" onClick={e => e.stopPropagation()}>
              <h3 className="text-white font-bold mb-4 text-center">Couleur du Groupe</h3>
              <div className="grid grid-cols-4 gap-2">
                {PRESET_COLORS.map((color, i) => (
                  <button
                    key={i}
                    className="w-full aspect-square rounded cursor-pointer border border-slate-700 hover:scale-110 hover:border-white transition-all focus:outline-none focus:ring-2 focus:ring-sky-500"
                    style={{ backgroundColor: color }}
                    onClick={() => {
                       handleColorChange(activePicker, color);
                       setActivePicker(null);
                    }}
                  />
                ))}
              </div>
           </div>
        </div>
      )}

      <style dangerouslySetInnerHTML={{__html: `
        input[type=range].slider-virtual-vg::-webkit-slider-thumb {
          -webkit-appearance: none;
          height: 36px;
          width: 56px;
          border-radius: 8px;
          background: linear-gradient(135deg, #38bdf8, #818cf8);
          cursor: pointer;
          border: 3px solid #fff;
          box-shadow: 0 0 14px rgba(56, 189, 248, 0.5);
        }
        input[type=range].slider-virtual-vg::-moz-range-thumb {
          height: 36px;
          width: 56px;
          border-radius: 8px;
          background: linear-gradient(135deg, #38bdf8, #818cf8);
          cursor: pointer;
          border: 3px solid #fff;
          box-shadow: 0 0 14px rgba(56, 189, 248, 0.5);
        }
      `}} />
    </div>
  );
};
