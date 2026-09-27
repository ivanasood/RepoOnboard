import { useState } from 'react';
import type { RepoStructure } from '../types';

interface CodebaseStructureProps {
  structure: RepoStructure;
}

/** How many paths to show before offering "Show more". */
const INITIAL_VISIBLE = 8;

interface CategoryProps {
  label: string;
  paths: string[];
}

function DirectoryCategory({ label, paths }: CategoryProps) {
  const [expanded, setExpanded] = useState(false);

  // De-duplicate in the unlikely case the backend sends repeated entries.
  const unique = Array.from(new Set(paths));

  if (unique.length === 0) {
    return null;
  }

  const visible = expanded ? unique : unique.slice(0, INITIAL_VISIBLE);
  const hidden  = unique.length - INITIAL_VISIBLE;

  return (
    <div style={{ marginBottom: '1.25rem' }}>
      {/* Category label */}
      <div style={{
        fontSize: '0.67rem',
        fontWeight: 700,
        letterSpacing: '0.08em',
        textTransform: 'uppercase',
        color: 'var(--text-3)',
        marginBottom: '0.4rem',
        fontFamily: 'var(--mono)',
      }}>
        {label}
        <span style={{ fontWeight: 400, marginLeft: '0.5rem', color: 'var(--text-3)' }}>
          ({unique.length})
        </span>
      </div>

      {/* Path list */}
      <div style={{
        border: '1px solid var(--border)',
        borderRadius: 'var(--r)',
        background: 'var(--surface)',
        overflow: 'hidden',
      }}>
        {visible.map((dir, i) => (
          <div
            key={dir}
            style={{
              display: 'flex',
              alignItems: 'center',
              padding: '0.35rem 0.75rem',
              borderBottom: i < visible.length - 1 || (!expanded && hidden > 0)
                ? '1px solid var(--border-dim)'
                : 'none',
            }}
          >
            <span style={{
              fontSize: '0.78rem',
              fontFamily: 'var(--mono)',
              color: 'var(--text)',
              lineHeight: 1.5,
              wordBreak: 'break-all',
            }}>
              {dir}
            </span>
          </div>
        ))}

        {/* Show more / show less toggle */}
        {unique.length > INITIAL_VISIBLE && (
          <button
            onClick={() => setExpanded(e => !e)}
            style={{
              display: 'flex',
              alignItems: 'center',
              width: '100%',
              padding: '0.35rem 0.75rem',
              background: 'var(--surface-dim)',
              border: 'none',
              cursor: 'pointer',
              font: 'inherit',
              fontSize: '0.75rem',
              color: 'var(--green)',
              fontWeight: 500,
              textAlign: 'left',
              gap: '0.3rem',
            }}
          >
            {expanded
              ? '↑ Show less'
              : `↓ Show ${hidden} more director${hidden === 1 ? 'y' : 'ies'}`}
          </button>
        )}
      </div>
    </div>
  );
}

export default function CodebaseStructure({ structure }: CodebaseStructureProps) {
  const hasAny =
    structure.top_level_directories.length > 0 ||
    structure.source_directories.length > 0 ||
    structure.test_directories.length > 0 ||
    structure.documentation_directories.length > 0 ||
    structure.configuration_directories.length > 0;

  if (!hasAny) {
    return (
      <p style={{ fontSize: '0.825rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
        No directory structure information was returned for this repository.
      </p>
    );
  }

  return (
    <div>
      <p style={{ fontSize: '0.75rem', color: 'var(--text-3)', marginBottom: '1rem', lineHeight: 1.5 }}>
        Detected directories from the repository tree. Each path is taken
        directly from the GitHub API response — no architectural labels are
        inferred.
      </p>

      <DirectoryCategory
        label="Top-level directories"
        paths={structure.top_level_directories}
      />
      <DirectoryCategory
        label="Source directories"
        paths={structure.source_directories}
      />
      <DirectoryCategory
        label="Test directories"
        paths={structure.test_directories}
      />
      <DirectoryCategory
        label="Documentation directories"
        paths={structure.documentation_directories}
      />
      <DirectoryCategory
        label="Configuration directories"
        paths={structure.configuration_directories}
      />
    </div>
  );
}
