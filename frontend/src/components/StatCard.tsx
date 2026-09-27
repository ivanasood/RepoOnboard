interface StatBlockProps {
  stats: {
    files: number;
    directories: number;
    source_files: number;
    test_files: number;
    documentation_files: number;
    configuration_files: number;
  };
}

interface MetricProps { value: number; label: string; }

function Metric({ value, label }: MetricProps) {
  return (
    <div style={{ textAlign: 'center', padding: '0.75rem 0.5rem' }}>
      <div style={{
        fontSize: '1.35rem', fontWeight: 700, lineHeight: 1,
        color: 'var(--text)', fontVariantNumeric: 'tabular-nums',
        fontFamily: 'var(--font)',
      }}>
        {value.toLocaleString()}
      </div>
      <div style={{
        fontSize: '0.67rem', fontWeight: 600, letterSpacing: '0.07em',
        textTransform: 'uppercase', color: 'var(--text-3)', marginTop: '0.3rem',
      }}>
        {label}
      </div>
    </div>
  );
}

export default function StatCard({ stats }: StatBlockProps) {
  return (
    <div style={{
      display: 'grid',
      gridTemplateColumns: 'repeat(6, 1fr)',
      border: '1px solid var(--border)',
      borderRadius: 'var(--r-lg)',
      background: 'var(--surface)',
      overflow: 'hidden',
    }}>
      {[
        { value: stats.files,               label: 'Files' },
        { value: stats.directories,         label: 'Dirs' },
        { value: stats.source_files,        label: 'Source' },
        { value: stats.test_files,          label: 'Tests' },
        { value: stats.documentation_files, label: 'Docs' },
        { value: stats.configuration_files, label: 'Config' },
      ].map((m, i, arr) => (
        <div key={m.label} style={{
          borderRight: i < arr.length - 1 ? '1px solid var(--border)' : 'none',
        }}>
          <Metric value={m.value} label={m.label} />
        </div>
      ))}
      <style>{`
        @media (max-width: 600px) {
          .stat-grid { grid-template-columns: repeat(3, 1fr) !important; }
        }
      `}</style>
    </div>
  );
}
