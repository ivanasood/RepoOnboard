import { useState, useEffect, useRef } from 'react';
import type { AnalysisResponse, UserContext } from './types';
import { analyzeRepository, ApiError } from './api';
import Header from './components/Header';
import RepoInput from './components/RepoInput';
import LoadingState from './components/LoadingState';
import ErrorState from './components/ErrorState';
import AnalysisDashboard from './components/AnalysisDashboard';

type AppState =
  | { kind: 'idle' }
  | { kind: 'loading'; step: number }
  | { kind: 'error'; code: string; message: string }
  | { kind: 'done'; analysis: AnalysisResponse; context: UserContext };

// Purely UI-side progress steps — the C backend does not stream events.
const STEP_INTERVALS = [400, 900, 1500, 2500];

export default function App() {
  const [state, setState] = useState<AppState>({ kind: 'idle' });
  const stepTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  function clearStepTimer() {
    if (stepTimerRef.current !== null) {
      clearTimeout(stepTimerRef.current);
      stepTimerRef.current = null;
    }
  }

  useEffect(() => () => clearStepTimer(), []);

  function advanceStep(current: number) {
    if (current >= STEP_INTERVALS.length) return;
    stepTimerRef.current = setTimeout(() => {
      setState(prev => {
        if (prev.kind !== 'loading') return prev;
        return { kind: 'loading', step: current + 1 };
      });
      advanceStep(current + 1);
    }, STEP_INTERVALS[current]);
  }

  async function handleSubmit(githubUrl: string, context: UserContext) {
    clearStepTimer();
    setState({ kind: 'loading', step: 0 });
    advanceStep(0);
    try {
      const analysis = await analyzeRepository(githubUrl);
      clearStepTimer();
      setState({ kind: 'done', analysis, context });
    } catch (err) {
      clearStepTimer();
      if (err instanceof ApiError) {
        setState({ kind: 'error', code: err.code, message: err.message });
      } else {
        setState({
          kind: 'error',
          code: 'UNKNOWN_ERROR',
          message: err instanceof Error ? err.message : 'An unexpected error occurred.',
        });
      }
    }
  }

  function handleReset() { clearStepTimer(); setState({ kind: 'idle' }); }
  function handleRetry() { clearStepTimer(); setState({ kind: 'idle' }); }

  return (
    <div className="page">
      <Header backendConnected={state.kind !== 'error'} />

      <main style={{ flex: 1 }}>
        {state.kind === 'idle' && (
          <div className="container">
            <RepoInput onSubmit={handleSubmit} isLoading={false} />
          </div>
        )}

        {state.kind === 'loading' && (
          <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', minHeight: 'calc(100vh - 44px)' }}>
            <LoadingState step={state.step} />
          </div>
        )}

        {state.kind === 'error' && (
          <ErrorState code={state.code} message={state.message} onRetry={handleRetry} />
        )}

        {state.kind === 'done' && (
          <AnalysisDashboard
            analysis={state.analysis}
            context={state.context}
            onReset={handleReset}
          />
        )}
      </main>

      <footer style={{
        borderTop: '1px solid var(--border)',
        padding: '0.75rem 1.5rem',
        display: 'flex', alignItems: 'center', justifyContent: 'space-between',
        flexWrap: 'wrap', gap: '0.5rem',
      }}>
        <span style={{ fontSize: '0.72rem', color: 'var(--text-3)' }}>
          RepoOnboard — IBM Bob Hackathon
        </span>
        <span style={{ fontSize: '0.72rem', color: 'var(--text-3)', fontFamily: 'var(--mono)' }}>
          POST http://localhost:8080/api/analyze
        </span>
      </footer>
    </div>
  );
}
