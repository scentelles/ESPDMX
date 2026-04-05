import { useState } from 'react';
import { UserPage } from '@/pages/UserPage';
import { AdminPage } from '@/pages/AdminPage';
import { LoginModal } from '@/components/modals';
import { useAppStore } from '@/store';
import '@/index.css';

function App() {
  const store = useAppStore();
  const [page, setPage] = useState<'user' | 'admin'>('user');
  const [loginOpen, setLoginOpen] = useState(false);

  const handleAdminClick = () => {
    setLoginOpen(true);
  };

  const handleLogin = async (pin: string) => {
    // In a real app, you'd send this to the server for verification
    if (pin === '1234') {
      // Default PIN - should be changed in settings
      store.setAuthenticated(true);
      setPage('admin');
      setLoginOpen(false);
    } else {
      store.setError('PIN invalide');
    }
  };

  const handleLogout = () => {
    store.setAuthenticated(false);
    setPage('user');
  };

  return (
    <div className="min-h-screen bg-slate-950">
      {page === 'user' ? (
        <>
          <UserPage />
          <div className="fixed bottom-4 right-4 z-40">
            <button
              onClick={handleAdminClick}
              className="w-12 h-12 bg-slate-800 hover:bg-slate-700 rounded-full border-2 border-slate-700 flex items-center justify-center text-slate-400 hover:text-slate-300 transition-all"
              title="Panneau Admin"
            >
              ⚙️
            </button>
          </div>
        </>
      ) : (
        <AdminPage onLogout={handleLogout} />
      )}

      <LoginModal isOpen={loginOpen} onLogin={handleLogin} onCancel={() => setLoginOpen(false)} />
    </div>
  );
}

export default App;
