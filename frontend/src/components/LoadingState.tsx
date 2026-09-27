interface LoadingStateProps {
  step: number;
}

const STEPS = [
  'Connecting to repository',
  'Fetching repository metadata',
  'Reading file structure',
  'Detecting technologies',
  'Building onboarding guide',
];

export default function LoadingState({ step }: LoadingStateProps) {
  return (
    <div style={{
      display: 'flex', flexDirection: 'column', alignItems: 'center',
      justifyContent: 'center', padding: '5rem 2rem', gap: '2rem',
    }}>
      {/* Minimal spinner */}
      <svg width="32" height="32" viewBox="0 0 32 32" style={{ animation: 'ro-spin 0.9s linear infinite' }}>
        <style>{`@keyframes ro-spin { to { transform: rotate(360deg); } }`}</style>
        <circle cx="16" cy="16" r="12" fill="none" stroke="var(--border)" strokeWidth="2.5" />
        <path d="M16 4 A12 12 0 0 1 28 16" fill="none" stroke="var(--green)" strokeWidth="2.5" strokeLinecap="round" />
      </svg>

      {/* Steps list */}
      <div style={{ display: 'flex', flexDirection: 'column', gap: '0', width: '100%', maxWidth: 320 }}>
        {STEPS.map((label, i) => {
          const done    = i < step;
          const current = i === step;
          return (
            <div key={i} style={{
              display: 'flex', alignItems: 'center', gap: '0.6rem',
              padding: '0.45rem 0',
              borderBottom: i < STEPS.length - 1 ? '1px solid var(--border-dim)' : 'none',
              opacity: i > step ? 0.35 : 1,
            }}>
              <span style={{
                width: 16, textAlign: 'center', fontSize: '0.7rem',
                color: done ? 'var(--green)' : current ? 'var(--text)' : 'var(--text-3)',
                fontFamily: 'var(--mono)', flexShrink: 0,
              }}>
                {done ? '✓' : String(i + 1).padStart(2, '0')}
              </span>
              <span style={{
                fontSize: '0.825rem',
                color: current ? 'var(--text)' : done ? 'var(--text-2)' : 'var(--text-3)',
                fontWeight: current ? 500 : 400,
              }}>
                {label}
              </span>
            </div>
          );
        })}
      </div>
    </div>
  );
}
