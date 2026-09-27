interface TechnologyListProps {
  technologies: string[];
}

export default function TechnologyList({ technologies }: TechnologyListProps) {
  if (technologies.length === 0) {
    return (
      <p style={{ fontSize: '0.825rem', color: 'var(--text-3)', fontStyle: 'italic' }}>
        No specific technologies detected.
      </p>
    );
  }

  return (
    <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.4rem' }}>
      {technologies.map(tech => (
        <span key={tech} className="badge" style={{ fontFamily: 'var(--mono)', fontSize: '0.75rem' }}>
          {tech}
        </span>
      ))}
    </div>
  );
}
