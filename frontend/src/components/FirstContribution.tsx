import type { AnalysisResponse, UserContext } from '../types';

interface FirstContributionProps {
  analysis: AnalysisResponse;
  context: UserContext;
}

interface Suggestion {
  title: string;
  /** What the API actually tells us that motivates this suggestion. */
  evidenceFacts: string[];
  /** Conservative guidance — does NOT invent defects or missing items. */
  guidance: string;
  steps: string[];
}

function buildSuggestion(analysis: AnalysisResponse, context: UserContext): Suggestion {
  const { c_analysis, repository } = analysis;
  const tech  = c_analysis.technologies;
  const stats = c_analysis.file_statistics;
  const { experience } = context;

  // ── Option A: Documentation improvement ───────────────────────────────────
  // Condition: the API confirms a README or documentation files exist.
  // We do NOT claim the docs are outdated, broken, or wrong.
  if (c_analysis.readme_available || stats.documentation_files > 0) {
    const docFacts: string[] = [];
    if (c_analysis.readme_available)
      docFacts.push(`A README was found in ${repository.name}.`);
    if (stats.documentation_files > 0)
      docFacts.push(`${stats.documentation_files} documentation file${stats.documentation_files !== 1 ? 's' : ''} were detected.`);

    return {
      title: 'Improve or extend the documentation',
      evidenceFacts: docFacts,
      guidance:
        'After understanding the existing project structure, look for documentation that could be made clearer or more complete. ' +
        'Only change what you can verify — do not invent technical details.',
      steps: [
        'Fork the repository and clone your fork.',
        'Read the documentation files listed in the Important Files table.',
        'Identify one specific, verifiable improvement (a missing step, an ambiguous sentence, an outdated instruction you can confirm).',
        'Make the change in a new branch.',
        'Open a pull request with a factual description of what you changed and why.',
      ],
    };
  }

  // ── Option B: Small source change (tests present, non-beginner) ───────────
  // Condition: test files exist (detected by the analyzer).
  // We do NOT claim a specific test is missing; we say to review an existing one first.
  if (stats.test_files > 0 && experience !== 'beginner') {
    return {
      title: 'Make a small, targeted source change',
      evidenceFacts: [
        `${stats.test_files} test file${stats.test_files !== 1 ? 's' : ''} were detected in this repository.`,
        `${stats.source_files} source file${stats.source_files !== 1 ? 's' : ''} were detected.`,
      ],
      guidance:
        'Because test files are present, you can use them to understand the expected behaviour of the code before making a change. ' +
        'Choose one well-understood source file; read the test that exercises it before touching anything.',
      steps: [
        'Fork the repository and clone your fork.',
        'Choose one small source file from the Important Files or Selected Files sections.',
        'Read the file and find a corresponding test file that exercises it.',
        'Understand what the test verifies before writing any new code.',
        'Make the smallest change you can justify from the existing evidence.',
        'Run the existing test suite to confirm no regressions.',
        'Open a pull request explaining what you changed and how you verified it.',
      ],
    };
  }

  // ── Option C: React UI component (technology confirmed by the analyzer) ───
  // We do NOT claim the component has a bug, missing prop, or accessibility issue.
  if (tech.some(t => /react/i.test(t))) {
    return {
      title: 'Make a small improvement to an existing component',
      evidenceFacts: [
        `The analyzer detected React in this repository's technology stack.`,
        `${stats.source_files} source file${stats.source_files !== 1 ? 's' : ''} were detected.`,
      ],
      guidance:
        'Read the components shown in the Selected Files section. Choose one you fully understand, then make a single, minimal, verifiable change. ' +
        'Do not assume the existence of bugs — only change what you can confirm after reading the code.',
      steps: [
        'Fork the repository and clone your fork.',
        'Read the React component files in the Selected Files section.',
        'Choose a single component you understand completely.',
        'Make the smallest change you can justify from reading the code.',
        'Verify the change behaves as expected in the browser.',
        'Open a pull request with a clear, factual description.',
      ],
    };
  }

  // ── Option D: Generic source improvement ──────────────────────────────────
  // Last resort — no invented defects. Only references confirmed API facts.
  const importantCount = c_analysis.important_files.length;
  const topFiles = c_analysis.important_files.slice(0, 2).join(' and ');
  return {
    title: 'Make a small, well-understood source change',
    evidenceFacts: [
      `${stats.files} file${stats.files !== 1 ? 's' : ''} were detected across ${stats.directories} director${stats.directories !== 1 ? 'ies' : 'y'}.`,
      importantCount > 0
        ? `${importantCount} file${importantCount !== 1 ? 's' : ''} were scored as important by the analyzer${topFiles ? ` (including ${topFiles})` : ''}.`
        : 'No files were specifically scored as important by the analyzer.',
    ],
    guidance:
      'Choose one file you can read and understand fully. Make the smallest verifiable change you can justify from the code itself. ' +
      'Do not invent problems — only work on what you can confirm by reading the source.',
    steps: [
      'Fork the repository and clone your fork.',
      'Read through the Important Files listed in the table above.',
      'Choose one file you can understand completely.',
      'Make a single, minimal, well-justified change in a new branch.',
      'Open a pull request with a clear explanation of what you changed and why.',
    ],
  };
}

export default function FirstContribution({ analysis, context }: FirstContributionProps) {
  const s = buildSuggestion(analysis, context);

  return (
    <div>
      {/* Provenance note */}
      <p style={{ fontSize: '0.78rem', color: 'var(--text-3)', marginBottom: '1rem' }}>
        Suggested first contribution — based only on the repository information available to RepoOnboard.
        No GitHub Issues, missing tests, or unverified defects are referenced.
      </p>

      <div style={{
        background: 'var(--surface)',
        border: '1px solid var(--border)',
        borderRadius: 'var(--r-lg)',
        overflow: 'hidden',
      }}>
        {/* Header bar */}
        <div style={{
          padding: '0.5rem 0.875rem',
          background: 'var(--green-tint)',
          borderBottom: '1px solid var(--green-mid)',
          display: 'flex', alignItems: 'center',
        }}>
          <span style={{ fontSize: '0.7rem', fontWeight: 600, letterSpacing: '0.07em', textTransform: 'uppercase', color: 'var(--green)' }}>
            Suggested first contribution
          </span>
        </div>

        <div style={{ padding: '1rem 1.25rem', display: 'flex', flexDirection: 'column', gap: '1rem' }}>
          {/* Title */}
          <h4 style={{ color: 'var(--text)', margin: 0 }}>{s.title}</h4>

          {/* Evidence facts */}
          <div style={{
            background: 'var(--surface-dim)',
            border: '1px solid var(--border)',
            borderRadius: 'var(--r)',
            padding: '0.625rem 0.875rem',
          }}>
            <div style={{
              fontSize: '0.62rem', fontWeight: 700, letterSpacing: '0.07em',
              textTransform: 'uppercase', color: 'var(--green)',
              fontFamily: 'var(--mono)', marginBottom: '0.35rem',
            }}>
              Repository evidence
            </div>
            {s.evidenceFacts.map((fact, i) => (
              <p key={i} style={{ fontSize: '0.8rem', color: 'var(--text)', lineHeight: 1.6, margin: '0 0 0.1rem' }}>
                {fact}
              </p>
            ))}
          </div>

          {/* Guidance */}
          <p style={{ fontSize: '0.825rem', color: 'var(--text-2)', margin: 0, lineHeight: 1.7 }}>
            {s.guidance}
          </p>

          {/* Steps */}
          <div>
            <div style={{
              fontSize: '0.67rem', fontWeight: 600, letterSpacing: '0.07em',
              textTransform: 'uppercase', color: 'var(--text-3)', marginBottom: '0.5rem',
            }}>
              Steps
            </div>
            <ol style={{ paddingLeft: '1.1rem', margin: 0, display: 'flex', flexDirection: 'column', gap: '0.3rem' }}>
              {s.steps.map((step, i) => (
                <li key={i} style={{ fontSize: '0.825rem', color: 'var(--text-2)', lineHeight: 1.6 }}>
                  {/* Show git/shell commands in monospace */}
                  {/^(Fork|Clone|git |npm |Run )/.test(step)
                    ? <code style={{ display: 'inline', fontSize: '0.78rem' }}>{step}</code>
                    : step
                  }
                </li>
              ))}
            </ol>
          </div>

          {/* Closing disclaimer */}
          <p style={{
            fontSize: '0.75rem', color: 'var(--text-3)',
            borderTop: '1px solid var(--border)', paddingTop: '0.75rem', margin: 0,
          }}>
            RepoOnboard does not have access to open issues, pull requests, or code review history.
            This suggestion is derived solely from the file statistics and structure returned by the C analysis backend.
          </p>
        </div>
      </div>
    </div>
  );
}
