import { useState } from 'react';
import type { SelectedFile } from '../types';

interface SelectedFilesProps {
  files: SelectedFile[];
}

function FileRow({ file }: { file: SelectedFile }) {
  const [open, setOpen] = useState(false);

  return (
    <div style={{ borderBottom: '1px solid var(--border-dim)' }}>
      <button
        onClick={() => setOpen(o => !o)}
        style={{
          display: 'flex', alignItems: 'center', gap: '0.5rem',
          width: '100%', padding: '0.55rem 0.875rem',
          background: 'transparent', border: 'none', cursor: 'pointer',
          textAlign: 'left', font: 'inherit',
        }}
        aria-expanded={open}
      >
        <span style={{
          fontSize: '0.65rem', color: 'var(--text-3)',
          fontFamily: 'var(--mono)', width: 8, flexShrink: 0, userSelect: 'none',
        }}>
          {open ? '▾' : '▸'}
        </span>
        <span style={{ fontFamily: 'var(--mono)', fontSize: '0.8rem', color: 'var(--text)', flex: 1, wordBreak: 'break-all' }}>
          {file.path}
        </span>
        <span style={{ fontSize: '0.7rem', color: 'var(--text-3)', flexShrink: 0, fontFamily: 'var(--mono)' }}>
          {file.content.length.toLocaleString()} chars
        </span>
      </button>

      {open && (
        <div style={{
          borderTop: '1px solid var(--border-dim)',
          background: 'var(--surface-dim)',
          padding: '0.875rem 1rem',
          maxHeight: 320,
          overflowY: 'auto',
        }}>
          {file.content.trim() === '' ? (
            <span style={{ fontSize: '0.78rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
              File content not available.
            </span>
          ) : (
            <pre style={{
              fontFamily: 'var(--mono)',
              fontSize: '0.76rem',
              lineHeight: 1.75,
              color: 'var(--text-2)',
              whiteSpace: 'pre-wrap',
              wordBreak: 'break-word',
              margin: 0,
            }}>
              {file.content}
            </pre>
          )}
        </div>
      )}
    </div>
  );
}

export default function SelectedFiles({ files }: SelectedFilesProps) {
  if (files.length === 0) {
    return (
      <p style={{ fontSize: '0.825rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
        No additional files were selected for this repository.
      </p>
    );
  }

  return (
    <div style={{ border: '1px solid var(--border)', borderRadius: 'var(--r-lg)', overflow: 'hidden', background: 'var(--surface)' }}>
      <div style={{
        padding: '0.4rem 0.875rem',
        background: 'var(--surface-dim)',
        borderBottom: '1px solid var(--border)',
        fontSize: '0.67rem', fontWeight: 600, letterSpacing: '0.07em',
        textTransform: 'uppercase', color: 'var(--text-3)',
      }}>
        {files.length} file{files.length !== 1 ? 's' : ''} selected
      </div>
      {files.map(f => <FileRow key={f.path} file={f} />)}
    </div>
  );
}
