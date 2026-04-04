import React, { ReactNode } from 'react';
import { AlertCircle } from 'lucide-react';
import { useAppStore } from '@/store';

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'primary' | 'secondary' | 'danger' | 'outline';
  size?: 'sm' | 'md' | 'lg';
  children: ReactNode;
}

export const Button = React.forwardRef<HTMLButtonElement, ButtonProps>(
  (
    {
      variant = 'primary',
      size = 'md',
      className = '',
      disabled,
      children,
      ...props
    },
    ref
  ) => {
    const baseStyles =
      'font-semibold rounded-lg transition-all duration-200 disabled:opacity-50 disabled:cursor-not-allowed';

    const variants = {
      primary: 'bg-purple-600 hover:bg-purple-700 text-white shadow-lg hover:shadow-purple-500/50',
      secondary: 'bg-pink-600 hover:bg-pink-700 text-white shadow-lg hover:shadow-pink-500/50',
      danger: 'bg-red-600 hover:bg-red-700 text-white shadow-lg hover:shadow-red-500/50',
      outline: 'border-2 border-purple-600 text-purple-400 hover:bg-purple-600/10',
    };

    const sizes = {
      sm: 'px-3 py-1 text-sm',
      md: 'px-4 py-2 text-base',
      lg: 'px-6 py-3 text-lg',
    };

    return (
      <button
        ref={ref}
        disabled={disabled}
        className={`${baseStyles} ${variants[variant]} ${sizes[size]} ${className}`}
        {...props}
      >
        {children}
      </button>
    );
  }
);

Button.displayName = 'Button';

interface CardProps extends React.HTMLAttributes<HTMLDivElement> {
  children: ReactNode;
}

export const Card = React.forwardRef<HTMLDivElement, CardProps>(
  ({ className = '', children, ...props }, ref) => (
    <div
      ref={ref}
      className={`bg-slate-900 rounded-xl border border-slate-800 p-6 shadow-xl ${className}`}
      {...props}
    >
      {children}
    </div>
  )
);

Card.displayName = 'Card';

interface AlertProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'error' | 'warning' | 'success' | 'info';
  children: ReactNode;
}

export const Alert = React.forwardRef<HTMLDivElement, AlertProps>(
  ({ variant = 'info', className = '', children, ...props }, ref) => {
    const variants = {
      error: 'bg-red-900/20 border-red-600 text-red-200',
      warning: 'bg-yellow-900/20 border-yellow-600 text-yellow-200',
      success: 'bg-green-900/20 border-green-600 text-green-200',
      info: 'bg-blue-900/20 border-blue-600 text-blue-200',
    };

    return (
      <div
        ref={ref}
        className={`flex items-center gap-3 border rounded-lg p-4 ${variants[variant]} ${className}`}
        {...props}
      >
        <AlertCircle size={20} className="flex-shrink-0" />
        <div>{children}</div>
      </div>
    );
  }
);

Alert.displayName = 'Alert';

interface InputProps extends React.InputHTMLAttributes<HTMLInputElement> {
  label?: string;
}

export const Input = React.forwardRef<HTMLInputElement, InputProps>(
  ({ label, className = '', ...props }, ref) => (
    <div className="flex flex-col gap-2">
      {label && <label className="text-sm font-medium text-slate-300">{label}</label>}
      <input
        ref={ref}
        className={`bg-slate-800 border border-slate-700 rounded-lg px-4 py-2 text-white placeholder-slate-400 focus:outline-none focus:border-purple-600 focus:ring-2 focus:ring-purple-600/30 ${className}`}
        {...props}
      />
    </div>
  )
);

Input.displayName = 'Input';

interface SelectProps extends React.SelectHTMLAttributes<HTMLSelectElement> {
  label?: string;
  options: { value: string; label: string }[];
}

export const Select = React.forwardRef<HTMLSelectElement, SelectProps>(
  ({ label, options, className = '', ...props }, ref) => (
    <div className="flex flex-col gap-2">
      {label && <label className="text-sm font-medium text-slate-300">{label}</label>}
      <select
        ref={ref}
        className={`bg-slate-800 border border-slate-700 rounded-lg px-4 py-2 text-white focus:outline-none focus:border-purple-600 focus:ring-2 focus:ring-purple-600/30 ${className}`}
        {...props}
      >
        {options.map((opt) => (
          <option key={opt.value} value={opt.value}>
            {opt.label}
          </option>
        ))}
      </select>
    </div>
  )
);

Select.displayName = 'Select';

interface SliderProps {
  label?: string;
  value?: number;
  onChange?: (value: number) => void;
  className?: string;
}

export const Slider = React.forwardRef<HTMLInputElement, SliderProps>(
  ({ label, value = 50, onChange, className = '' }, ref) => (
    <div className="flex flex-col gap-3">
      {label && (
        <div className="flex justify-between">
          <label className="text-sm font-medium text-slate-300">{label}</label>
          <span className="text-sm text-purple-400">{value}%</span>
        </div>
      )}
      <input
        ref={ref}
        type="range"
        min="0"
        max="100"
        value={value}
        onChange={(e) => onChange?.(parseInt(e.target.value))}
        className={`w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-purple-600 ${className}`}
      />
    </div>
  )
);

Slider.displayName = 'Slider';

export interface ColorPickerProps {
  value?: string;
  onChange?: (color: string) => void;
  label?: string;
  className?: string;
}

export const ColorPicker = React.forwardRef<HTMLDivElement, ColorPickerProps>(
  ({ value = '#9333ea', onChange, label, className = '' }, ref) => (
    <div ref={ref} className={`flex flex-col gap-2 ${className}`}>
      {label && <label className="text-sm font-medium text-slate-300">{label}</label>}
      <input
        type="color"
        value={value}
        onChange={(e) => onChange?.(e.target.value)}
        className="h-12 rounded-lg cursor-pointer border-2 border-slate-700 hover:border-purple-600"
      />
    </div>
  )
);

ColorPicker.displayName = 'ColorPicker';

interface LoadingSpinnerProps {
  size?: 'sm' | 'md' | 'lg';
}

export const LoadingSpinner: React.FC<LoadingSpinnerProps> = ({ size = 'md' }) => {
  const sizes = {
    sm: 'w-4 h-4',
    md: 'w-8 h-8',
    lg: 'w-12 h-12',
  };

  return (
    <div className={`${sizes[size]} border-4 border-slate-700 border-t-purple-600 rounded-full animate-spin`} />
  );
};

interface ErrorNotificationProps {
  message?: string;
  onClose?: () => void;
}

export const ErrorNotification: React.FC<ErrorNotificationProps> = ({ onClose }) => {
  const { error, setError } = useAppStore();

  if (!error) return null;

  return (
    <div className="fixed top-4 right-4 z-50 animate-slide-in">
      <Alert variant="error" className="flex items-center justify-between">
        <div>{error}</div>
        <button
          onClick={() => {
            setError(null);
            onClose?.();
          }}
          className="text-2xl ml-4"
        >
          ×
        </button>
      </Alert>
    </div>
  );
};
