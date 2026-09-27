// TypeScript interfaces matching the C backend JSON response exactly.

export interface FileStatistics {
  files: number;
  directories: number;
  source_files: number;
  test_files: number;
  documentation_files: number;
  configuration_files: number;
}

/** Deterministic directory organisation derived from the GitHub tree walk. */
export interface RepoStructure {
  top_level_directories: string[];
  source_directories: string[];
  test_directories: string[];
  documentation_directories: string[];
  configuration_directories: string[];
}

export interface CAnalysis {
  technologies: string[];
  important_files: string[];
  file_statistics: FileStatistics;
  readme_available: boolean;
  /** Present when the backend is >= the version that added structure analysis. */
  structure?: RepoStructure;
}

export interface SelectedFile {
  path: string;
  content: string;
}

export interface BoundedSnapshot {
  readme: string;
  selected_files: SelectedFile[];
  max_readme_characters: number;
  max_selected_files: number;
}

export interface Repository {
  owner: string;
  name: string;
  url: string;
  default_branch: string;
  description: string;
  stars: number;
}

export interface AnalysisResponse {
  repository: Repository;
  c_analysis: CAnalysis;
  bounded_snapshot: BoundedSnapshot;
}

export interface BackendError {
  code: string;
  message: string;
}

export interface ErrorResponse {
  success: false;
  error: BackendError;
}

// User-provided inputs that influence dashboard presentation only.
export type Role =
  | 'student'
  | 'frontend'
  | 'backend'
  | 'fullstack'
  | 'data_ml'
  | 'other';

export type Experience = 'beginner' | 'intermediate' | 'advanced';

export interface UserContext {
  role: Role;
  experience: Experience;
}
