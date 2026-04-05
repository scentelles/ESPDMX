import { useState } from 'react';
import { Volume2, Lock } from 'lucide-react';
import { Button, Input } from '@/components/ui';

interface LoginModalProps {
  isOpen: boolean;
  onLogin: (pin: string) => void;
  onCancel: () => void;
}

export const LoginModal: React.FC<LoginModalProps> = ({ isOpen, onLogin, onCancel }) => {
  const [pin, setPin] = useState('');
  const [error, setError] = useState('');

  const handleSubmit = () => {
    if (pin.length < 4) {
      setError('Le PIN doit contenir au moins 4 chiffres');
      return;
    }
    onLogin(pin);
    setPin('');
    setError('');
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50">
      <div className="bg-slate-900 rounded-xl border border-slate-800 p-8 shadow-2xl max-w-md w-full mx-4">
        <div className="flex items-center justify-center mb-6">
          <Lock className="w-8 h-8 text-purple-600 mr-2" />
          <h2 className="text-2xl font-bold text-white">Accès Admin</h2>
        </div>

        <p className="text-slate-400 text-center mb-6">Entrez votre PIN administrateur pour continuer</p>

        <Input
          type="password"
          inputMode="numeric"
          placeholder="Entrez le PIN"
          value={pin}
          onChange={(e) => {
            setPin(e.target.value);
            setError('');
          }}
          maxLength={10}
          className="mb-4"
        />

        {error && <p className="text-red-400 text-sm mb-4">{error}</p>}

        <div className="flex gap-3">
          <Button variant="outline" className="flex-1" onClick={onCancel}>
            Annuler
          </Button>
          <Button variant="primary" className="flex-1" onClick={handleSubmit}>
            Connexion
          </Button>
        </div>
      </div>
    </div>
  );
};

interface VolumeControlProps {
  volume: number;
  onVolumeChange: (volume: number) => void;
}

export const VolumeControl: React.FC<VolumeControlProps> = ({ volume, onVolumeChange }) => {
  return (
    <div className="flex items-center gap-3 bg-slate-900 rounded-lg p-3 border border-slate-800">
      <Volume2 size={20} className="text-purple-400" />
      <input
        type="range"
        min="0"
        max="100"
        value={volume}
        onChange={(e) => onVolumeChange(parseInt(e.target.value))}
        className="flex-1 h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-purple-600"
      />
      <span className="text-sm text-slate-300 min-w-[2rem]">{volume}%</span>
    </div>
  );
};
