import { useState, useEffect } from 'react';
import { LogOut, Sliders, Layers, LayoutGrid, Palette, Play, Settings, Monitor, Mic } from 'lucide-react';
import { LoadingSpinner, ErrorNotification } from '@/components/ui';
import { useAppStore } from '@/store';
import { apiService } from '@/services/api';

// Desktop/Admin sub-pages
import { CatalogTab } from './admin/CatalogTab';
import { SetupsTab } from './admin/SetupsTab';
import { VirtualGroupsTab } from './admin/VirtualGroupsTab';
import { ScenesTab } from './admin/ScenesTab';
import { ShowsTab } from './admin/ShowsTab';
import { SettingsTab } from './admin/SettingsTab';
import { ConsoleClassicTab } from './admin/ConsoleClassicTab';
import { ConsoleVirtualTab } from './admin/ConsoleVirtualTab';
import { DmxMonitorTab } from './admin/DmxMonitorTab';

type AdminTab = 'catalog' | 'setups' | 'vgroups' | 'scenes' | 'shows' | 'console-classic' | 'console-virtual' | 'dmx-monitor' | 'settings';

interface AdminPageProps {
  onLogout: () => void;
}

export const AdminPage: React.FC<AdminPageProps> = ({ onLogout }) => {
  const store = useAppStore();
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<AdminTab>('setups');

  useEffect(() => {
    loadData();
  }, []);

  const loadData = async () => {
    try {
      const [profiles, setupsList, activeSetup] = await Promise.all([
        apiService.getProfiles(),
        apiService.getSetupsList(),
        apiService.getActiveSetup(),
      ]);
      store.setProfiles(profiles);
      store.setSetupsList(setupsList);
      store.setActiveSetup(activeSetup);
      setLoading(false);
    } catch (error) {
      store.setError('Erreur de chargement des données admin');
      console.error(error);
      setLoading(false);
    }
  };

  if (loading) {
     return <div className="flex h-screen items-center justify-center"><LoadingSpinner size="lg" /></div>;
  }

  const navItems = [
    { id: 'catalog', label: 'Catalogue', icon: <LayoutGrid size={18} /> },
    { id: 'setups', label: 'Setups & Fixtures', icon: <Layers size={18} /> },
    { id: 'vgroups', label: 'Groupes Virtuels', icon: <Sliders size={18} /> },
    { id: 'scenes', label: 'Scènes', icon: <Palette size={18} /> },
    { id: 'shows', label: 'Shows', icon: <Play size={18} /> },
    { id: 'console-classic', label: 'Console 512', icon: <Monitor size={18} /> },
    { id: 'console-virtual', label: 'Console Virtuelle', icon: <Mic size={18} /> }, // Using mic as placeholder or anything
    { id: 'dmx-monitor', label: 'Moniteur DMX', icon: <LayoutGrid size={18} /> },
    { id: 'settings', label: 'Paramètres', icon: <Settings size={18} /> },
  ];

  return (
    <div className="flex h-screen bg-slate-950 overflow-hidden">
      <ErrorNotification />

      {/* Sidebar sidebar */}
      <div className="w-64 bg-slate-900 border-r border-slate-800 flex flex-col">
        <div className="p-6 border-b border-slate-800">
           <h1 className="text-xl font-bold text-white flex items-center gap-2">
              <Settings className="text-purple-500" />
              SUD SHOW PRO
           </h1>
           <p className="text-xs text-slate-400 mt-1">Multi-Setup DMX Controller</p>
        </div>

        <nav className="flex-1 overflow-y-auto p-4 space-y-2">
           {navItems.map(item => (
              <button
                 key={item.id}
                 onClick={() => setActiveTab(item.id as AdminTab)}
                 className={`w-full flex items-center gap-3 px-4 py-3 rounded-lg text-sm font-medium transition-colors ${
                    activeTab === item.id 
                      ? 'bg-purple-600 text-white shadow-lg' 
                      : 'text-slate-400 hover:text-white hover:bg-slate-800'
                 }`}
              >
                 {item.icon}
                 {item.label}
              </button>
           ))}
        </nav>

        <div className="p-4 border-t border-slate-800">
           <button onClick={onLogout} className="w-full flex items-center justify-center gap-2 px-4 py-3 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-lg transition-colors text-sm font-medium">
              <LogOut size={16} /> Quitter Admin
           </button>
        </div>
      </div>

      {/* Main content */}
      <div className="flex-1 overflow-y-auto p-8">
         <div className="max-w-6xl mx-auto">
            {activeTab === 'catalog' && <CatalogTab />}
            {activeTab === 'setups' && <SetupsTab />}
            {activeTab === 'vgroups' && <VirtualGroupsTab />}
            {activeTab === 'scenes' && <ScenesTab />}
            {activeTab === 'shows' && <ShowsTab />}
            {activeTab === 'console-classic' && <ConsoleClassicTab />}
            {activeTab === 'console-virtual' && <ConsoleVirtualTab />}
            {activeTab === 'dmx-monitor' && <DmxMonitorTab />}
            {activeTab === 'settings' && <SettingsTab />}
         </div>
      </div>
    </div>
  );
};
