interface ErrorStateProps {
  code: string;
  message: string;
  onRetry: () => void;
}

const errorHints: Record<string, string> = {
  NETWORK_ERROR:        'Make sure the C backend is running: ./backend/bin/repoonboard',
  INVALID_GITHUB_URL:   'Use the format: https://github.com/owner/repository',
  REPOSITORY_NOT_FOUND: 'Check that the repository exists and is public on GitHub.',
  GITHUB_REQUEST_FAILED:'GitHub could not be reached. You may be hitting the rate limit (60 req/hr unauthenticated).',
  INVALID_JSON:         'The request body was malformed.',
  INVALID_INPUT:        'The github_url field was missing or empty.',
  HTTP_ERROR:           'The server returned an unexpected status code.',
  MALFORMED_RESPONSE:   'The backend returned a response that could not be parsed.',
};

export default function ErrorState({ code, message, onRetry }: ErrorStateProps) {
  const hint = errorHints[code];
  return (
    <div style={{ display: 'flex', justifyContent: 'center', padding: '3rem 1.5rem' }}>
      <div style={{
        maxWidth: 480, width: '100%',
        background: 'var(--surface)',
        border: '1px solid var(--error-border)',
        borderRadius: 'var(--r-lg)',
        overflow: 'hidden',
      }}>
        {/* Error header */}
        <div style={{
          padding: '0.6rem 1rem',
          background: 'var(--error-tint)',
          borderBottom: '1px solid var(--error-border)',
          display: 'flex', alignItems: 'center', justifyContent: 'space-between',
        }}>
          <span style={{ fontSize: '0.8rem', fontWeight: 600, color: 'var(--error)' }}>
            Analysis failed
          </span>
          <code style={{ fontSize: '0.72rem', background: 'transparent', border: 'none', color: 'var(--error)', padding: 0 }}>
            {code}
          </code>
        </div>

        <div style={{ padding: '1.25rem', display: 'flex', flexDirection: 'column', gap: '0.875rem' }}>
          <p style={{ fontSize: '0.875rem', color: 'var(--text)', margin: 0, lineHeight: 1.6 }}>
            {message}
          </p>

          {hint && (
            <div style={{
              padding: '0.625rem 0.875rem',
              background: 'var(--surface-dim)',
              border: '1px solid var(--border)',
              borderRadius: 'var(--r)',
              fontSize: '0.8rem', color: 'var(--text-2)',
            }}>
              {hint}
            </div>
          )}

          <button className="btn btn-secondary" onClick={onRetry} style={{ alignSelf: 'flex-start' }}>
            ← Try again
          </button>
        </div>
      </div>
    </div>
  );
}
