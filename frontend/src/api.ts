/// <reference types="vite/client" />
import type { AnalysisResponse, ErrorResponse } from './types';

const BACKEND_URL =
  import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080';

export class ApiError extends Error {
  constructor(
    public readonly code: string,
    message: string,
    public readonly httpStatus?: number,
  ) {
    super(message);
    this.name = 'ApiError';
  }
}

export async function analyzeRepository(
  githubUrl: string,
): Promise<AnalysisResponse> {
  let response: Response;

  try {
    response = await fetch(`${BACKEND_URL}/api/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ github_url: githubUrl }),
    });
  } catch (networkError) {
    throw new ApiError(
      'NETWORK_ERROR',
      'Could not reach the RepoOnboard backend. Make sure ./backend/bin/repoonboard is running on port 8080.',
    );
  }

  let body: unknown;
  try {
    body = await response.json();
  } catch {
    throw new ApiError(
      'MALFORMED_RESPONSE',
      `Backend returned a non-JSON response (HTTP ${response.status}).`,
      response.status,
    );
  }

  if (!response.ok) {
    // Try to parse the structured error the C server returns.
    if (
      body !== null &&
      typeof body === 'object' &&
      'error' in body
    ) {
      const err = (body as ErrorResponse).error;
      throw new ApiError(err.code, err.message, response.status);
    }
    throw new ApiError(
      'HTTP_ERROR',
      `Request failed with HTTP ${response.status}.`,
      response.status,
    );
  }

  // Basic shape validation — guards against an unexpected response.
  if (
    body === null ||
    typeof body !== 'object' ||
    !('repository' in body) ||
    !('c_analysis' in body) ||
    !('bounded_snapshot' in body)
  ) {
    throw new ApiError(
      'UNEXPECTED_RESPONSE',
      'Backend response was missing expected fields.',
    );
  }

  return body as AnalysisResponse;
}

export async function checkHealth(): Promise<boolean> {
  try {
    const res = await fetch(`${BACKEND_URL}/api/health`);
    return res.ok;
  } catch {
    return false;
  }
}
