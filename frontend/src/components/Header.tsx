interface HeaderProps {
  backendConnected?: boolean;
}

export default function Header({ backendConnected = true }: HeaderProps) {
  return (
    <header style={{
      background: 'var(--surface)',
      borderBottom: '1px solid var(--border)',
      position: 'sticky',
      top: 0,
      zIndex: 50,
    }}>
      <div className="container" style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        height: 44,
      }}>
        {/* Wordmark */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <svg width="18" height="18" viewBox="0 0 18 18" fill="none" aria-hidden="true" style={{ flexShrink: 0 }}>
            <rect width="18" height="18" rx="3" fill="var(--green)" />
            <path d="M4 6h10M4 9h7M4 12h8" stroke="#fff" strokeWidth="1.5" strokeLinecap="round" />
          </svg>
          <span style={{ fontWeight: 600, fontSize: '0.9rem', letterSpacing: '-0.01em', color: 'var(--text)' }}>
            RepoOnboard
          </span>
        </div>

        {/* Centre label */}
        <span style={{ fontSize: '0.75rem', color: 'var(--text-3)', fontWeight: 500, letterSpacing: '0.02em' }}>
          Repository Analyzer
        </span>

        {/* Status */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.4rem' }}>
          <span style={{
            width: 6, height: 6, borderRadius: '50%',
            background: backendConnected ? '#22C55E' : '#EF4444',
            flexShrink: 0,
          }} />
          <span style={{ fontSize: '0.72rem', color: 'var(--text-3)', fontFamily: 'var(--mono)' }}>
            {backendConnected ? 'C backend connected' : 'backend offline'}
          </span>
        </div>
      </div>
    </header>
  );
}
