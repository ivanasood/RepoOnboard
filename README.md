# RepoOnboard / RepoPilot — C analysis + IBM Bob MCP

This repository contains the backend vertical slice for the IBM Bob
Hackathon MVP. The application keeps its working C + GitHub analysis API and
exposes the same capability to IBM Bob through a project-scoped MCP server.

IBM Bob is not called as a deployed LLM REST API. There are no Bob API
credentials, model variables, or invented inference endpoints in this
project. Bob connects locally to the C MCP server defined in `.bob/mcp.json`
and can call the repository-analysis tool when it needs repository context.

The service never clones or executes repository code. It fetches repository
metadata, the recursive file tree, a small set of important text files, and a
bounded README.

## Requirements

- GCC or Clang
- `libcurl`
- `cJSON`
- `pkg-config`

The native packages are installed in the Replit workspace. On another Linux
machine, install the development packages for those libraries using the
system package manager.

## Configuration

There is no Bob API configuration.

Optional environment variables:

```bash
export PORT=8080
export GITHUB_API_URL="https://api.github.com"
```

The backend reads environment variables directly; it does not load `.env`.
`.env.example` is provided as a reference.

## Build

```bash
make clean && make
```

This builds both executables:

```text
backend/bin/repoonboard
backend/bin/repoonboard-mcp
```

## Run the C HTTP API

```bash
./backend/bin/repoonboard
```

The server listens on `PORT` (default `8080`).

### Health

```bash
curl http://localhost:8080/api/health
```

### Analyze

```bash
curl -X POST http://localhost:8080/api/analyze \
  -H 'Content-Type: application/json' \
  -d '{"github_url":"https://github.com/octocat/Hello-World"}'
```

The response contains:

- `repository`: canonical repository identity and metadata
- `c_analysis`: deterministic C-side technology, file, and statistic analysis
- `bounded_snapshot`: bounded README and selected file contents for downstream
  analysis

The C analyzer:

- Validates and parses the GitHub URL
- Fetches repository metadata and the recursive tree
- Classifies source, test, documentation, and configuration files
- Detects common technologies and manifests
- Selects important files
- Caps README and selected-file content before returning it

## IBM Bob MCP integration

The project-level configuration is in `.bob/mcp.json`:

```json
{
  "mcpServers": {
    "repoonboard": {
      "command": "${workspaceFolder}/backend/bin/repoonboard-mcp",
      "cwd": "${workspaceFolder}",
      "alwaysAllow": [
        "analyze_github_repository"
      ],
      "disabled": false
    }
  }
}
```

IBM Bob's project-level MCP configuration is loaded from `.bob/mcp.json`.
The configured command starts `repoonboard-mcp` using STDIO transport. Bob
then communicates with the process using MCP JSON-RPC messages.

The server exposes one tool:

```text
analyze_github_repository
```

Input schema:

```json
{
  "github_url": "https://github.com/owner/repository"
}
```

The tool returns the same structured repository analysis as the HTTP API,
including the bounded snapshot. Bob can use it like this:

1. Bob loads the project MCP configuration.
2. Bob starts `backend/bin/repoonboard-mcp` as a local STDIO process.
3. Bob discovers `analyze_github_repository` through `tools/list`.
4. A user asks Bob to onboard them to a public repository.
5. Bob calls `tools/call` with the repository URL.
6. The C server fetches and analyzes the repository.
7. Bob receives the structured analysis and explains it to the user.

Bob supplies the interpretation and conversation. The C server supplies
verified repository evidence and deterministic analysis. No model inference is
performed by the deployed application.

### Enable it in IBM Bob

1. Build the project with `make`.
2. Open the project root in IBM Bob.
3. Ensure MCP servers are enabled in Bob's MCP settings.
4. Reload or restart Bob so it reads `.bob/mcp.json`.
5. Confirm that the `repoonboard` server and
   `analyze_github_repository` tool appear in the MCP panel.
6. Ask Bob:

```text
Analyze https://github.com/octocat/Hello-World for a new contributor.
Use the RepoOnboard MCP tool and explain the setup steps and first contribution.
```

The tool is marked in `alwaysAllow` so Bob can call it without a separate
approval prompt. Remove that entry if manual approval is preferred.

## MCP protocol smoke test without Bob

This checks the same STDIO protocol Bob uses:

```bash
printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"smoke-test","version":"1.0"}}}' \
  '{"jsonrpc":"2.0","method":"notifications/initialized","params":{}}' \
  '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}' \
  '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"analyze_github_repository","arguments":{"github_url":"https://github.com/octocat/Hello-World"}}}' \
  | ./backend/bin/repoonboard-mcp
```

This is a protocol test, not a Bob-host test. The final Bob-side check must
be run from IBM Bob after it reloads `.bob/mcp.json`.

## Architecture

```text
React later
   │
   │ POST /api/analyze
   ▼
C analysis service
   ├── GitHub URL validation
   ├── GitHub API via libcurl
   ├── bounded repository snapshot
   └── deterministic repository analysis

IBM Bob
   │ MCP STDIO / JSON-RPC
   ▼
repoonboard-mcp
   └── same C analysis service
```

The HTTP API and MCP tool call the same `analysis_service.c` implementation,
so the dashboard and IBM Bob cannot drift into separate analysis behavior.
