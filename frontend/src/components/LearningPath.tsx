import type { AnalysisResponse, UserContext } from '../types';
import type { Role } from '../types';

interface LearningPathProps {
  analysis: AnalysisResponse;
  context: UserContext;
}

interface Step {
  title: string;
  /** Sentence(s) directly derived from the API response, labelled FACT where needed. */
  facts: string[];
  /** General onboarding guidance — clearly separated from facts. */
  guidance?: string;
}

function getRoleLabel(role: Role): string {
  const labels: Record<Role, string> = {
    student:   'Student',
    frontend:  'Frontend Developer',
    backend:   'Backend Developer',
    fullstack: 'Full Stack Developer',
    data_ml:   'Data/ML Developer',
    other:     'Developer',
  };
  return labels[role];
}

function buildSteps(analysis: AnalysisResponse, context: UserContext): Step[] {
  const { repository, c_analysis, bounded_snapshot } = analysis;
  const { role, experience } = context;
  const tech  = c_analysis.technologies;
  const stats = c_analysis.file_statistics;
  const steps: Step[] = [];

  // ── 1. README ──────────────────────────────────────────────────────────────
  // Only claim what the API tells us: that a README was found and its length.
  if (c_analysis.readme_available) {
    const charCount = bounded_snapshot.readme.length;
    steps.push({
      title: 'Read the README',
      facts: [
        `The analyzer found a README in ${repository.name} (${charCount.toLocaleString()} characters shown in the section above).`,
      ],
      guidance: experience === 'beginner'
        ? 'Read it fully before looking at any source code. It is the intended entry point for new contributors.'
        : 'Read it before drawing conclusions about the codebase structure.',
    });
  }

  // ── 2. Technology stack ────────────────────────────────────────────────────
  // Use only c_analysis.technologies — no invented layers or architecture.
  if (tech.length > 0) {
    steps.push({
      title: 'Understand the technology stack',
      facts: [
        `The analyzer detected the following technologies: ${tech.join(', ')}.`,
      ],
      guidance: experience === 'advanced'
        ? `As a ${getRoleLabel(role)}, identify which of these are directly relevant to the work you plan to do.`
        : experience === 'beginner'
          ? 'If any of these are unfamiliar, research them before reading the source code.'
          : 'Note which of these you already know and which will need more attention.',
    });
  }

  // ── 3. Repository structure ────────────────────────────────────────────────
  // Only use the numeric statistics returned by the API.
  {
    const structFacts: string[] = [
      `${stats.files.toLocaleString()} file${stats.files !== 1 ? 's' : ''} across ${stats.directories.toLocaleString()} director${stats.directories !== 1 ? 'ies' : 'y'}.`,
    ];
    if (stats.source_files > 0)
      structFacts.push(`${stats.source_files.toLocaleString()} source file${stats.source_files !== 1 ? 's' : ''} detected.`);
    if (stats.documentation_files > 0)
      structFacts.push(`${stats.documentation_files.toLocaleString()} documentation file${stats.documentation_files !== 1 ? 's' : ''} detected.`);
    if (stats.configuration_files > 0)
      structFacts.push(`${stats.configuration_files.toLocaleString()} configuration file${stats.configuration_files !== 1 ? 's' : ''} detected.`);

    steps.push({
      title: 'Explore the project structure',
      facts: structFacts,
      guidance: experience === 'beginner'
        ? 'Browse the top-level directory layout before opening any individual files.'
        : 'Use the statistics to orient yourself — then locate entry points and module boundaries in the source.',
    });
  }

  // ── 4. Role-specific focus (only if the relevant technology is confirmed) ──
  if (role === 'frontend') {
    const uiTech = tech.filter(t => /react|vue|svelte|angular|css|html/i.test(t));
    if (uiTech.length > 0) {
      steps.push({
        title: 'Locate the UI layer',
        facts: [
          `The analyzer detected UI-related technologies in this repository: ${uiTech.join(', ')}.`,
        ],
        guidance: 'Look for the files in the Important Files and Selected Files sections that correspond to these technologies.',
      });
    }
  } else if (role === 'backend') {
    const serverTech = tech.filter(t => /node|python|go|rust|java|c\/c\+\+|c\b/i.test(t));
    if (serverTech.length > 0) {
      steps.push({
        title: 'Locate the server-side code',
        facts: [
          `The analyzer detected server-side technologies: ${serverTech.join(', ')}.`,
        ],
        guidance: 'Use the Important Files and Selected Files sections to find the primary server-side entry points.',
      });
    }
  } else if (role === 'data_ml') {
    const dataTech = tech.filter(t => /python|r\b|julia|numpy|pandas|tensorflow|torch/i.test(t));
    if (dataTech.length > 0) {
      steps.push({
        title: 'Identify relevant data technologies',
        facts: [
          `The analyzer detected the following potentially data-related technologies: ${dataTech.join(', ')}.`,
        ],
        guidance: 'Look for data-related files among the Important Files and Selected Files sections.',
      });
    }
  }

  // ── 5. Important files ─────────────────────────────────────────────────────
  if (c_analysis.important_files.length > 0) {
    const listed = c_analysis.important_files.slice(0, 4).join(', ');
    const more   = c_analysis.important_files.length > 4
      ? ` (and ${c_analysis.important_files.length - 4} more listed above)`
      : '';
    steps.push({
      title: 'Study the key files',
      facts: [
        `The C static analyzer scored ${c_analysis.important_files.length} file${c_analysis.important_files.length !== 1 ? 's' : ''} as important: ${listed}${more}.`,
      ],
      guidance: 'Read these files in the order they appear in the Important Files table above. They represent the highest-signal entry points identified by path and extension scoring.',
    });
  }

  // ── 6. Selected source files ───────────────────────────────────────────────
  if (bounded_snapshot.selected_files.length > 0) {
    const paths = bounded_snapshot.selected_files.map(f => f.path).slice(0, 3).join(', ');
    const more  = bounded_snapshot.selected_files.length > 3
      ? ` and ${bounded_snapshot.selected_files.length - 3} more`
      : '';
    steps.push({
      title: 'Read the selected source files',
      facts: [
        `The analyzer retrieved ${bounded_snapshot.selected_files.length} source file${bounded_snapshot.selected_files.length !== 1 ? 's' : ''} for review: ${paths}${more}.`,
        `Each file is shown in full (bounded by the C backend at ${bounded_snapshot.max_readme_characters.toLocaleString()} characters per file).`,
      ],
      guidance: experience === 'beginner'
        ? 'Read each file top-to-bottom. Do not try to understand every line — focus on recognising patterns and the overall structure.'
        : 'Pay attention to naming conventions, error handling patterns, and module boundaries across the files.',
    });
  }

  // ── 7. Tests ───────────────────────────────────────────────────────────────
  // Only state that test files were detected. Do NOT claim any test is missing.
  if (stats.test_files > 0) {
    steps.push({
      title: 'Review the test files',
      facts: [
        `The analyzer detected ${stats.test_files.toLocaleString()} test file${stats.test_files !== 1 ? 's' : ''} in this repository.`,
      ],
      guidance: 'Reading an existing test alongside the source file it covers is one reliable way to understand what a module is expected to do. Do not assume anything about test coverage from the count alone.',
    });
  }

  // ── 8. First change ────────────────────────────────────────────────────────
  // No repository-specific claim here — this is pure onboarding guidance.
  steps.push({
    title: 'Plan your first change',
    facts: [],
    guidance: 'Before making any change: choose a single, clearly scoped item, verify your understanding of the surrounding code, and confirm the project\'s contribution guidelines (usually in the README or a CONTRIBUTING file) before opening a pull request.',
  });

  return steps;
}

export default function LearningPath({ analysis, context }: LearningPathProps) {
  const steps = buildSteps(analysis, context);

  return (
    <div>
      {/* Context note */}
      <p style={{ fontSize: '0.78rem', color: 'var(--text-3)', marginBottom: '1.25rem' }}>
        Tailored for a <strong style={{ color: 'var(--text-2)' }}>{getRoleLabel(context.role)}</strong> at{' '}
        <strong style={{ color: 'var(--text-2)' }}>{context.experience}</strong> level.
        Repository facts are labelled; guidance is separated from those facts.
      </p>

      {/* Numbered steps */}
      <div style={{ position: 'relative' }}>
        {/* Vertical line */}
        <div style={{
          position: 'absolute',
          left: 15, top: 24, bottom: 8,
          width: 1, background: 'var(--border)',
          zIndex: 0,
        }} />

        <div style={{ display: 'flex', flexDirection: 'column', gap: '0' }}>
          {steps.map((step, i) => (
            <div key={i} style={{
              display: 'flex', gap: '1rem', alignItems: 'flex-start',
              paddingBottom: i < steps.length - 1 ? '1.4rem' : 0,
              position: 'relative',
            }}>
              {/* Step number */}
              <div style={{
                width: 30, height: 30, borderRadius: '50%',
                background: 'var(--surface)',
                border: '1px solid var(--border)',
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                flexShrink: 0, zIndex: 1,
              }}>
                <span style={{
                  fontSize: '0.68rem', fontWeight: 700,
                  fontFamily: 'var(--mono)', color: 'var(--text-2)',
                }}>
                  {String(i + 1).padStart(2, '0')}
                </span>
              </div>

              {/* Content */}
              <div style={{ paddingTop: '0.35rem', flex: 1 }}>
                <div style={{
                  fontSize: '0.85rem', fontWeight: 600,
                  color: 'var(--text)', marginBottom: '0.35rem',
                }}>
                  {step.title}
                </div>

                {/* Repository facts */}
                {step.facts.length > 0 && (
                  <div style={{ marginBottom: step.guidance ? '0.5rem' : 0 }}>
                    {step.facts.map((fact, fi) => (
                      <p key={fi} style={{
                        fontSize: '0.8rem', color: 'var(--text)',
                        lineHeight: 1.6, margin: '0 0 0.15rem',
                        fontWeight: 450,
                      }}>
                        <span style={{
                          fontSize: '0.62rem', fontWeight: 700,
                          letterSpacing: '0.07em', textTransform: 'uppercase',
                          color: 'var(--green)', marginRight: '0.4rem',
                          fontFamily: 'var(--mono)', verticalAlign: 'middle',
                        }}>
                          FACT
                        </span>
                        {fact}
                      </p>
                    ))}
                  </div>
                )}

                {/* General guidance */}
                {step.guidance && (
                  <p style={{
                    fontSize: '0.8rem', color: 'var(--text-2)',
                    lineHeight: 1.65, margin: 0,
                    borderLeft: step.facts.length > 0 ? '2px solid var(--border)' : 'none',
                    paddingLeft: step.facts.length > 0 ? '0.6rem' : 0,
                  }}>
                    {step.guidance}
                  </p>
                )}
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
