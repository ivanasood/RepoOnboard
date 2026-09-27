import type { AnalysisResponse, UserContext } from '../types';
import StatCard from './StatCard';
import TechnologyList from './TechnologyList';
import CodebaseStructure from './CodebaseStructure';
import ImportantFiles from './ImportantFiles';
import SetupGuide from './SetupGuide';
import SelectedFiles from './SelectedFiles';
import LearningPath from './LearningPath';
import FirstContribution from './FirstContribution';

interface AnalysisDashboardProps {
  analysis: AnalysisResponse;
  context: UserContext;
  onReset: () => void;
}

function SecLabel({ children }: { children: React.ReactNode }) {
  return <div className="sec-label">{children}</div>;
}

function Divider() {
  return <div style={{ height: 1, background: 'var(--border)', margin: '1.5rem 0' }} />;
}

export default function AnalysisDashboard({ analysis, context, onReset }: AnalysisDashboardProps) {
  const { repository, c_analysis, bounded_snapshot } = analysis;
  const stats = c_analysis.file_statistics;

  return (
    <div style={{ paddingBottom: '4rem' }}>

      {/* ── Repository identity bar ── */}
      <div style={{
        background: 'var(--surface)',
        borderBottom: '1px solid var(--border)',
      }}>
        <div className="container" style={{ padding: '1rem 1.5rem' }}>
          <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', gap: '1rem', flexWrap: 'wrap' }}>
            <div>
              {/* Owner / name */}
              <div style={{ display: 'flex', alignItems: 'baseline', gap: '0.3rem', flexWrap: 'wrap', marginBottom: '0.3rem' }}>
                <span style={{ fontSize: '0.9rem', color: 'var(--text-2)' }}>{repository.owner}</span>
                <span style={{ color: 'var(--border)', fontWeight: 300 }}>/</span>
                <span style={{ fontSize: '1rem', fontWeight: 600, color: 'var(--text)' }}>{repository.name}</span>
                <a
                  href={repository.url}
                  target="_blank"
                  rel="noopener noreferrer"
                  style={{
                    fontSize: '0.72rem', color: 'var(--text-3)',
                    border: '1px solid var(--border)',
                    borderRadius: 'var(--r)',
                    padding: '1px 6px',
                    marginLeft: '0.25rem',
                    textDecoration: 'none',
                    fontFamily: 'var(--mono)',
                  }}
                >
                  ↗ github
                </a>
              </div>

              {/* Description */}
              {repository.description && (
                <p style={{ fontSize: '0.825rem', color: 'var(--text-2)', margin: '0 0 0.4rem', maxWidth: 560 }}>
                  {repository.description}
                </p>
              )}

              {/* Meta */}
              <div style={{ display: 'flex', gap: '1.25rem', flexWrap: 'wrap' }}>
                <span style={{ fontSize: '0.75rem', color: 'var(--text-3)', fontFamily: 'var(--mono)' }}>
                  branch: {repository.default_branch}
                </span>
                <span style={{ fontSize: '0.75rem', color: 'var(--text-3)' }}>
                  {repository.stars.toLocaleString()} stars
                </span>
              </div>
            </div>

            <button
              className="btn btn-secondary"
              onClick={onReset}
              style={{ flexShrink: 0, fontSize: '0.8rem' }}
            >
              ← Analyze another
            </button>
          </div>
        </div>
      </div>

      {/* ── Statistics ── */}
      <div style={{ background: 'var(--bg)', borderBottom: '1px solid var(--border)', padding: '1rem 0' }}>
        <div className="container">
          <StatCard stats={stats} />
        </div>
      </div>

      {/* ── Two-column dashboard ── */}
      <div className="container" style={{ paddingTop: '2rem' }}>
        <div style={{
          display: 'grid',
          gridTemplateColumns: '340px 1fr',
          gap: '2.5rem',
          alignItems: 'start',
        }}
        className="dashboard-grid"
        >
          {/* ── LEFT COLUMN ── */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1.75rem' }}>
            {/* Technology stack */}
            {c_analysis.technologies.length > 0 && (
              <div>
                <SecLabel>Technology Stack</SecLabel>
                <TechnologyList technologies={c_analysis.technologies} />
              </div>
            )}

            {/* Codebase structure */}
            {c_analysis.structure && (
              <div>
                <SecLabel>Codebase Structure</SecLabel>
                <CodebaseStructure structure={c_analysis.structure} />
              </div>
            )}

            {/* Important files */}
            <div>
              <SecLabel>Important Files</SecLabel>
              <p style={{ fontSize: '0.75rem', color: 'var(--text-3)', marginBottom: '0.6rem', lineHeight: 1.5 }}>
                Identified by the C static analyzer using path and extension scoring.
              </p>
              <ImportantFiles files={c_analysis.important_files} />
            </div>
          </div>

          {/* ── RIGHT COLUMN ── */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1.75rem' }}>
            {/* README */}
            <div>
              <SecLabel>README / Setup</SecLabel>
              <SetupGuide readme={bounded_snapshot.readme} readmeAvailable={c_analysis.readme_available} />
            </div>

            {/* Selected files */}
            {bounded_snapshot.selected_files.length > 0 && (
              <div>
                <SecLabel>Selected Files</SecLabel>
                <SelectedFiles files={bounded_snapshot.selected_files} />
              </div>
            )}

            <Divider />

            {/* Onboarding path */}
            <div>
              <SecLabel>Onboarding Path</SecLabel>
              <LearningPath analysis={analysis} context={context} />
            </div>

            <Divider />

            {/* First contribution */}
            <div>
              <SecLabel>First Contribution</SecLabel>
              <FirstContribution analysis={analysis} context={context} />
            </div>
          </div>
        </div>
      </div>

      <style>{`
        @media (max-width: 860px) {
          .dashboard-grid {
            grid-template-columns: 1fr !important;
            gap: 1.5rem !important;
          }
        }
      `}</style>
    </div>
  );
}
