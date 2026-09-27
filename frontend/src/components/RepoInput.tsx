import { useState } from 'react';
import type { Role, Experience, UserContext } from '../types';

interface RepoInputProps {
  onSubmit: (githubUrl: string, context: UserContext) => void;
  isLoading: boolean;
}

function isValidGitHubUrl(url: string): boolean {
  return /^https?:\/\/github\.com\/[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+\/?$/.test(url.trim());
}

const ROLES: { value: Role; label: string }[] = [
  { value: 'student',   label: 'Student' },
  { value: 'frontend',  label: 'Frontend Developer' },
  { value: 'backend',   label: 'Backend Developer' },
  { value: 'fullstack', label: 'Full Stack Developer' },
  { value: 'data_ml',   label: 'Data / ML Developer' },
  { value: 'other',     label: 'Other' },
];

const EXPERIENCES: { value: Experience; label: string }[] = [
  { value: 'beginner',     label: 'Beginner' },
  { value: 'intermediate', label: 'Intermediate' },
  { value: 'advanced',     label: 'Advanced' },
];

export default function RepoInput({ onSubmit, isLoading }: RepoInputProps) {
  const [url, setUrl]          = useState('');
  const [role, setRole]        = useState<Role>('student');
  const [experience, setExp]   = useState<Experience>('beginner');
  const [urlError, setUrlError] = useState('');
  const [touched, setTouched]  = useState(false);

  function validate(value: string): string {
    if (value.trim() === '') return 'A GitHub repository URL is required.';
    if (!isValidGitHubUrl(value)) return 'Enter a valid URL: https://github.com/owner/repository';
    return '';
  }

  function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    setTouched(true);
    const err = validate(url);
    setUrlError(err);
    if (err) return;
    onSubmit(url.trim(), { role, experience });
  }

  function handleUrlChange(e: React.ChangeEvent<HTMLInputElement>) {
    setUrl(e.target.value);
    if (touched) setUrlError(validate(e.target.value));
  }

  return (
    <div style={{
      display: 'grid',
      gridTemplateColumns: '1fr 1fr',
      gap: '4rem',
      alignItems: 'start',
      padding: '4rem 0',
      maxWidth: 960,
      margin: '0 auto',
      width: '100%',
    }}
    className="landing-grid"
    >
      {/* Left: editorial copy */}
      <div>
        <p style={{
          fontSize: '0.68rem',
          fontWeight: 700,
          letterSpacing: '0.1em',
          textTransform: 'uppercase',
          color: 'var(--green)',
          marginBottom: '1rem',
        }}>
          Repository Onboarding
        </p>

        <h1 style={{ marginBottom: '1rem', lineHeight: 1.2 }}>
          Understand a codebase before you change it.
        </h1>

        <p style={{ fontSize: '0.925rem', color: 'var(--text-2)', maxWidth: 380, lineHeight: 1.75 }}>
          Analyze a public GitHub repository and turn its structure, technologies
          and documentation into a practical onboarding guide.
        </p>

        {/* Technical info block */}
        <div style={{
          marginTop: '2rem',
          borderTop: '1px solid var(--border)',
          paddingTop: '1.25rem',
          display: 'flex',
          flexDirection: 'column',
          gap: '0.4rem',
        }}>
          {[
            'C ANALYSIS ENGINE',
            'MCP ENABLED',
            'PUBLIC GITHUB REPOSITORIES',
          ].map(label => (
            <div key={label} style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
              <span style={{
                width: 5, height: 5, borderRadius: '50%',
                background: 'var(--green)', flexShrink: 0,
              }} />
              <span style={{ fontSize: '0.7rem', fontWeight: 600, letterSpacing: '0.07em', color: 'var(--text-3)', fontFamily: 'var(--mono)' }}>
                {label}
              </span>
            </div>
          ))}
        </div>
      </div>

      {/* Right: form */}
      <div className="panel" style={{ padding: '1.5rem' }}>
        <h3 style={{ marginBottom: '1.25rem', fontWeight: 600, color: 'var(--text)' }}>
          Analyze a repository
        </h3>

        <form onSubmit={handleSubmit} noValidate style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
          <div className="field">
            <label className="field-label" htmlFor="repo-url">GitHub Repository URL</label>
            <input
              id="repo-url"
              type="url"
              className={`field-input${urlError ? ' error' : ''}`}
              placeholder="https://github.com/owner/repository"
              value={url}
              onChange={handleUrlChange}
              onBlur={() => { setTouched(true); setUrlError(validate(url)); }}
              disabled={isLoading}
              autoComplete="off"
              spellCheck={false}
            />
            {urlError && <span className="field-error">{urlError}</span>}
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.75rem' }}>
            <div className="field">
              <label className="field-label" htmlFor="role">Role</label>
              <select
                id="role"
                className="field-select"
                value={role}
                onChange={e => setRole(e.target.value as Role)}
                disabled={isLoading}
              >
                {ROLES.map(r => <option key={r.value} value={r.value}>{r.label}</option>)}
              </select>
            </div>
            <div className="field">
              <label className="field-label" htmlFor="experience">Experience</label>
              <select
                id="experience"
                className="field-select"
                value={experience}
                onChange={e => setExp(e.target.value as Experience)}
                disabled={isLoading}
              >
                {EXPERIENCES.map(ex => <option key={ex.value} value={ex.value}>{ex.label}</option>)}
              </select>
            </div>
          </div>

          <div style={{ paddingTop: '0.25rem' }}>
            <button
              type="submit"
              className="btn btn-primary btn-full"
              disabled={isLoading}
            >
              {isLoading ? 'Analyzing…' : 'Analyze repository'}
            </button>
          </div>
        </form>
      </div>

      <style>{`
        @media (max-width: 720px) {
          .landing-grid {
            grid-template-columns: 1fr !important;
            gap: 2rem !important;
            padding: 2rem 0 !important;
          }
        }
      `}</style>
    </div>
  );
}
