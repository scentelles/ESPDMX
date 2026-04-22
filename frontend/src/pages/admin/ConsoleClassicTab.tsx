import { useState, useEffect, useMemo } from 'react';
import { ChevronLeft, ChevronRight } from 'lucide-react';
import { Card, Button } from '@/components/ui';

import { apiService } from '@/services/api';

export const ConsoleClassicTab = () => {
  const [page, setPage] = useState(0);
  const [channelsPerPage, setChannelsPerPage] = useState(16); // 8 portrait, 16 landscape
  const [values, setValues] = useState<number[]>(new Array(512).fill(0));

  useEffect(() => {
    const handleResize = () => {
      setChannelsPerPage(window.innerWidth < 768 ? 8 : 16);
    };
    handleResize();
    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, []);

  // Sync with store lightingState periodically or just use local state + update?
  // We'll write to apiService.setDMXChannel on change
  const handleChange = (ch: number, val: number) => {
    const newValues = [...values];
    newValues[ch - 1] = val;
    setValues(newValues);
    apiService.setDMXChannel(ch, val);
  };

  const totalPages = Math.ceil(512 / channelsPerPage);
  const currentChannels = useMemo(() => {
    const start = page * channelsPerPage;
    const end = Math.min(start + channelsPerPage, 512);
    return Array.from({ length: end - start }, (_, i) => start + i + 1);
  }, [page, channelsPerPage]);

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center bg-slate-900 p-4 rounded-lg">
        <h2 className="text-xl font-bold text-white">Console DMX Classique</h2>
        
        <div className="flex items-center gap-4">
          <Button 
            variant="outline" 
            onClick={() => setPage(p => Math.max(0, p - 1))}
            disabled={page === 0}
          >
            <ChevronLeft size={20} />
          </Button>
          <span className="text-white font-mono">
            Page {page + 1}/{totalPages} <span className="text-slate-400 text-sm hidden md:inline">(Ch {page * channelsPerPage + 1} - {Math.min((page + 1) * channelsPerPage, 512)})</span>
          </span>
          <Button 
            variant="outline" 
            onClick={() => setPage(p => Math.min(totalPages - 1, p + 1))}
            disabled={page === totalPages - 1}
          >
            <ChevronRight size={20} />
          </Button>
        </div>
      </div>

      <Card className="p-6">
        <div className={`grid gap-4 ${channelsPerPage === 8 ? 'grid-cols-4 sm:grid-cols-8' : 'grid-cols-8 md:grid-cols-16'}`}>
          {currentChannels.map(ch => (
             <div key={ch} className="flex flex-col items-center gap-2">
                <span className="text-xs text-slate-400 font-mono">CH {ch}</span>
                <div className="h-48 flex items-end justify-center w-8 bg-slate-900 rounded-full py-2 shadow-inner border border-slate-800">
                   <input
                     {...{ orient: "vertical" } as any}
                     type="range"
                     className="w-full h-full slider-vertical appearance-none bg-transparent cursor-pointer"
                     style={{ WebkitAppearance: 'slider-vertical' } as any}
                     min={0}
                     max={255}
                     value={values[ch - 1]}
                     onChange={e => handleChange(ch, parseInt(e.target.value) || 0)}
                   />
                </div>
                <input
                   type="number"
                   min={0}
                   max={255}
                   className="w-12 text-center bg-slate-800 text-white text-xs py-1 rounded border border-slate-700 font-mono"
                   value={values[ch - 1]}
                   onChange={e => handleChange(ch, parseInt(e.target.value) || 0)}
                />
             </div>
          ))}
        </div>
      </Card>
      
      <style dangerouslySetInnerHTML={{__html: `
        input[type=range].slider-vertical::-webkit-slider-thumb {
          -webkit-appearance: none;
          height: 16px;
          width: 24px;
          border-radius: 4px;
          background: #a855f7;
          cursor: pointer;
          border: 2px solid #fff;
        }
      `}} />
    </div>
  );
};
