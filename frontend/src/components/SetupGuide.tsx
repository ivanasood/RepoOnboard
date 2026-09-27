interface SetupGuideProps {
  readme: string;
  readmeAvailable: boolean;
}

export default function SetupGuide({ readme, readmeAvailable }: SetupGuideProps) {
  if (!readmeAvailable || readme === '(README unavailable)' || readme.trim() === '') {
    return (
      <p style={{ fontSize: '0.825rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
        No README was detected in this repository.
      </p>
    );
  }

  return (
    <div style={{
      background: 'var(--surface)',
      border: '1px solid var(--border)',
      borderRadius: 'var(--r-lg)',
      overflow: 'hidden',
    }}>
      {/* Bar */}
      <div style={{
        display: 'flex', alignItems: 'center', justifyContent: 'space-between',
        padding: '0.5rem 0.875rem',
        background: 'var(--surface-dim)',
        borderBottom: '1px solid var(--border)',
      }}>
        <span style={{ fontSize: '0.72rem', fontWeight: 600, color: 'var(--text-3)', fontFamily: 'var(--mono)', letterSpacing: '0.04em' }}>
          README
        </span>
        <span style={{ fontSize: '0.7rem', color: 'var(--text-3)' }}>
          {readme.length.toLocaleString()} chars (bounded)
        </span>
      </div>

      {/* Content */}
      <div style={{
        padding: '1.25rem',
        fontFamily: 'var(--font)',
        fontSize: '0.85rem',
        lineHeight: 1.8,
        color: 'var(--text-2)',
        maxHeight: 480,
        overflowY: 'auto',
        whiteSpace: 'pre-wrap',
        wordBreak: 'break-word',
        maxWidth: '72ch',
      }}>
        {readme}
      </div>
    </div>
  );
}
