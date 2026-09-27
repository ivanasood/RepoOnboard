interface ImportantFilesProps {
  files: string[];
}

function fileCategory(path: string): string {
  if (/readme/i.test(path)) return 'readme';
  if (/\.(test|spec)\./i.test(path) || /test[s]?\//i.test(path)) return 'test';
  if (/dockerfile|\.yml|\.yaml|\.toml|\.ini|package\.json|cargo\.toml|go\.mod/i.test(path)) return 'config';
  if (/\.(md|mdx|rst)$/i.test(path) || /docs\//i.test(path)) return 'docs';
  if (/\.(ts|tsx|js|jsx|py|go|rs|java|c|h|cpp|hpp|rb|php|sh|swift|kt)$/i.test(path)) return 'source';
  return 'file';
}

const catLabel: Record<string, string> = {
  readme: 'README', test: 'Test', config: 'Config',
  docs: 'Docs', source: 'Source', file: 'File',
};

const catNote: Record<string, string> = {
  readme: 'Project documentation entry point',
  test:   'Test suite — read to understand expected behaviour',
  config: 'Build / dependency manifest',
  docs:   'Additional documentation',
  source: 'Primary source file',
  file:   'Identified as relevant by path scoring',
};

export default function ImportantFiles({ files }: ImportantFilesProps) {
  if (files.length === 0) {
    return (
      <p style={{ fontSize: '0.825rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
        No important files were identified.
      </p>
    );
  }

  return (
    <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--r-lg)', overflow: 'hidden', background: 'var(--surface)' }}>
      {/* Table header */}
      <div style={{
        display: 'grid', gridTemplateColumns: '1fr 70px 1fr',
        padding: '0.4rem 0.875rem',
        background: 'var(--surface-dim)',
        borderBottom: '1px solid var(--border)',
        fontSize: '0.67rem', fontWeight: 600, letterSpacing: '0.07em',
        textTransform: 'uppercase', color: 'var(--text-3)',
      }}>
        <span>File</span>
        <span>Type</span>
        <span>Why it matters</span>
      </div>

      {files.map((file, i) => {
        const cat = fileCategory(file);
        return (
          <div key={file} style={{
            display: 'grid', gridTemplateColumns: '1fr 70px 1fr',
            padding: '0.55rem 0.875rem',
            borderBottom: i < files.length - 1 ? '1px solid var(--border-dim)' : 'none',
            alignItems: 'center',
            gap: '0.5rem',
          }}>
            <span style={{
              fontFamily: 'var(--mono)', fontSize: '0.78rem',
              color: 'var(--text)', wordBreak: 'break-all',
            }}>
              {file}
            </span>
            <span className="badge">{catLabel[cat]}</span>
            <span style={{ fontSize: '0.78rem', color: 'var(--text-2)' }}>
              {catNote[cat]}
            </span>
          </div>
        );
      })}
    </div>
  );
}
