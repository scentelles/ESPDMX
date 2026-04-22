import { useState } from 'react';
import { Card, Alert } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';

export const ConsoleVirtualTab = () => {
  const store = useAppStore();
  const [values, setValues] = useState<Record<string, number>>({});
  const [colorValues, setColorValues] = useState<Record<string, string>>({});
  const [activePicker, setActivePicker] = useState<string | null>(null);

  const PRESET_COLORS = [
    '#ffcdd2', '#ef5350', '#f44336', '#d32f2f', // Reds
    '#ffe0b2', '#ffa726', '#ff9800', '#f57c00', // Oranges
    '#fff9c4', '#fff176', '#ffeb3b', '#fbc02d', // Yellows
    '#dcedc8', '#aed581', '#8bc34a', '#689f38', // YellowGreens
    '#c8e6c9', '#66bb6a', '#4caf50', '#388e3c', // Greens
    '#b2dfdb', '#4db6ac', '#009688', '#00796b', // Teals
    '#b3e5fc', '#4fc3f7', '#03a9f4', '#0288d1', // LightBlues
    '#bbdefb', '#64b5f6', '#2196f3', '#1976d2', // Blues
    '#c5cae9', '#7986cb', '#3f51b5', '#303f9f', // Indigos
    '#e1bee7', '#ba68c8', '#9c27b0', '#7b1fa2', // Purples
    '#f8bbd0', '#f06292', '#e91e63', '#c2185b', // Pinks
    '#ffffff', '#bdbdbd', '#757575', '#000000'  // Monos
  ];

  const activeSetup = store.activeSetup;

  if (!activeSetup) {
    return <Alert variant="warning">Aucune configuration active. Allez dans "Setups" pour en activer une.</Alert>;
  }

  const handleChange = (vgId: string, val: number) => {
    setValues({ ...values, [vgId]: val });
    apiService.setVirtualGroupValue(vgId, val);
  };

  const handleColorChange = (vgId: string, hex: string) => {
     setColorValues({ ...colorValues, [vgId]: hex });
     
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

       const chR = profile.channels.find(c => /red|rouge|\br\b/i.test(c.name));
       const chG = profile.channels.find(c => /green|vert|\bg\b/i.test(c.name));
       const chB = profile.channels.find(c => /blue|bleu|\bb\b/i.test(c.name));

       if (chR) apiService.setDMXChannel(inst.dmxAddress + chR.offset, r);
       if (chG) apiService.setDMXChannel(inst.dmxAddress + chG.offset, g);
       if (chB) apiService.setDMXChannel(inst.dmxAddress + chB.offset, b);
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
             {activeSetup.virtualGroups.map(vg => (
               <div key={vg.id} className="flex flex-col items-center gap-3 min-w-[80px]">
                 <div className="font-medium text-white text-sm text-center line-clamp-2 h-10 w-24 leading-snug">
                    {vg.name}
                 </div>
                 <div className="mb-2">
                    <button
                       onClick={() => setActivePicker(vg.id)}
                       style={{ backgroundColor: colorValues[vg.id] || '#ffffff' }}
                       className="w-10 h-10 rounded-full cursor-pointer border-2 border-slate-700 shadow-sm outline-none transition-transform hover:scale-105" 
                       title="Couleur des projecteurs de ce groupe"
                    />
                 </div>
                 <div className="h-64 flex items-end justify-center w-12 bg-slate-900 rounded-full py-4 shadow-inner border border-slate-800">
                   <input
                     {...{ orient: "vertical" } as any}
                     type="range"
                     className="w-full h-full slider-vertical appearance-none bg-transparent cursor-pointer"
                     style={{ WebkitAppearance: 'slider-vertical' } as any}
                     min={0}
                     max={255}
                     value={values[vg.id] || 0}
                     onChange={e => handleChange(vg.id, parseInt(e.target.value) || 0)}
                   />
                 </div>
                 <input
                   type="number"
                   min={0}
                   max={255}
                   className="w-16 text-center bg-slate-800 text-white font-mono rounded border border-slate-700 mt-2"
                   value={values[vg.id] || 0}
                   onChange={e => handleChange(vg.id, parseInt(e.target.value) || 0)}
                 />
                 <div className="text-xs text-slate-500 mt-1">
                    {vg.assignments.length} proj.
                 </div>
               </div>
             ))}
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
        input[type=range].slider-vertical::-webkit-slider-thumb {
          -webkit-appearance: none;
          height: 20px;
          width: 32px;
          border-radius: 4px;
          background: #38bdf8; /* sky-400 */
          cursor: pointer;
          border: 2px solid #fff;
          box-shadow: 0 0 10px rgba(56, 189, 248, 0.5);
        }
      `}} />
    </div>
  );
};
