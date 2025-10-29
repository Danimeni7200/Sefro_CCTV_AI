# 📘 Project Best Practices

## 1. Project Purpose
A hybrid-tier License Plate Recognition (LPR) system. The Edge tier (C++ streaming client, Python AI FastAPI, Electron desktop) captures frames from IP cameras, preprocesses and performs LPR inference, exposes health/metrics, and optionally syncs anonymized results to the Cloud tier. The Cloud tier (FastAPI API + Next.js dashboard) provides authentication, ingest/analytics endpoints, and a Farsi RTL dashboard for monitoring.

## 2. Project Structure
- Monorepo layout under packages/
  - packages/local_cpp: C++ edge streaming client
    - src/: pipeline, stream reader, preprocessor, inference client, health server, logging, config
    - CMakeLists*.txt: build configs (full/simple)
    - config.json(.example): runtime config (stream, AI host, queues, preprocess)
  - packages/ai_service: Python FastAPI service for LPR (/infer, /results/latest, /healthz)
    - main.py, requirements.txt, config.env.example
  - packages/cloud_api: Python FastAPI backend (JWT auth, ingest, analytics, websocket)
    - main.py, requirements.txt, config.env.example, SQLite db (dev/demo)
  - packages/cloud_web: Next.js (TypeScript) dashboard with RTL Farsi UI
    - pages/, styles/, next.config.js, package.json, tsconfig.json
  - packages/electron_app: Electron desktop app (preload bridge, renderer UI)
    - main.js, preload.js, renderer.html, package.json, config.json.example
  - packages/shared/schemas: JSON schemas (e.g., lpr_result.schema.json) used across tiers
- Root files
  - README.md, EXTENSIBILITY.md, .gitignore, config.json (not used; prefer package-level configs)
- Entry points and configs
  - C++: src/main.cpp (full client), src/simple_main.cpp (test client)
  - AI: uvicorn main:app
  - Cloud API: uvicorn main:app
  - Cloud Web: Next.js dev/build scripts
  - Electron: npm start
  - Env/config via *.env.example and package-level config.json files. Do not commit real secrets.

## 3. Test Strategy
- Current status: No formal test suites checked in. Adopt the following approach per tier:
- Unit tests
  - C++ (local_cpp):
    - ring_buffer: concurrency and backpressure
    - preprocessor: deterministic transforms, quality scoring
    - inference_client: JSON parsing and error handling with stubbed responses
    - logger/config: rotation, hot-reload flags
    - Framework: GoogleTest; integrate via CMake add_subdirectory/tests
  - Python (ai_service & cloud_api):
    - FastAPI endpoints with TestClient
    - Pydantic models validation, schema compatibility with shared/schemas
    - Token issuance/verification, rate-limiting helpers
    - Mock external calls (YOLO/EasyOCR, DB, HTTP to Cloud) with unittest.mock/pytest-mock
    - Framework: pytest + coverage; factory fixtures for app and DB (cloud_api uses in-memory sqlite for tests)
  - Next.js (cloud_web):
    - React components and helpers with Jest/Vitest + React Testing Library
    - API client functions: mock fetch; verify auth header and error handling
  - Electron (electron_app):
    - Preload API contract tests; renderer logic tests with Jest + jsdom
- Integration tests
  - Edge flow: simulate camera → C++ pipeline → AI /infer (use sample frames) → verify result schema and latency bounds
  - Cloud flow: login → JWT → analytics endpoints; ingest LPR payloads
  - Cross-schema: validate AI output JSON against shared/schemas/lpr_result.schema.json
  - Tools: pytest for cross-service; optional docker-compose for local orchestration
- E2E tests
  - Web dashboard (cloud_web): Playwright tests for login, charts, and RTL layout
  - Electron: Playwright (Electron mode) basic smoke (load, set host, poll, render rows)
- Coverage expectations
  - Unit: 80%+ for Python and core C++ libs; critical paths prioritized
  - Integration/E2E: scenario coverage of happy path and key failures (RTSP down, AI down)
- Mocking guidelines
  - Prefer interface boundaries: HTTP calls mocked at client layer
  - For C++, abstract HTTP and time sources for deterministic tests
  - For Python, isolate external model imports behind lazy getters; inject fakes in tests

## 4. Code Style
- Cross-cutting
  - Follow shared schemas; keep bbox format consistent across tiers (prefer [x, y, w, h] or [x1, y1, x2, y2] consistently and document in schema)
  - Use structured, machine-parseable logs (JSON lines) with timestamps and camera_id
  - Keep long-running or blocking work off request threads; use worker threads/queues
  - Validate inputs at service boundaries; fail fast with informative errors
- C++ (local_cpp)
  - Files: lower_snake_case for filenames; header/impl pairs (.hpp/.cpp)
  - Types: UpperCamelCase; functions/variables: lower_snake_case; constants/macros: ALL_CAPS
  - RAII: prefer smart pointers; avoid raw new/delete
  - Concurrency: guard shared state; avoid data races; clearly define ownership
  - Error handling: return expected/optional for recoverable cases; reserve exceptions for exceptional conditions; never throw across threads
  - Performance: avoid copies of cv::Mat; use move semantics; bound queues for backpressure
  - Config: read once, watch for changes with atomic flags; apply safely
- Python (FastAPI services)
  - Typing: use type hints everywhere; Pydantic models for request/response
  - Async: declare async def endpoints; avoid CPU-bound work on the loop; use thread/process pools if needed
  - Errors: raise HTTPException with explicit status_code and detail; centralized handlers for common errors
  - Serialization: prefer orjson/ujson; ensure datetime is timezone-aware (UTC ISO 8601)
  - Configuration: load from env with pydantic-settings or os.getenv; provide *.env.example; no secrets in VCS
  - Dependencies: lazy-load heavy models (YOLO/EasyOCR); cache singletons thread-safely
- Next.js (cloud_web)
  - Components: PascalCase; files under pages follow Next conventions; other components under components/
  - State: React hooks; avoid global mutable state; keep JWT in memory + localStorage with care
  - Network: centralized API client; always send Authorization: Bearer <token>
  - i18n/RTL: keep lang="fa" dir="rtl"; verify layout in tests
  - Styling: prefer CSS modules or utility classes for scalability; avoid global leaks
- Electron (electron_app)
  - Security: enable contextIsolation, sandbox, and a minimal preload bridge; never expose nodeIntegration
  - IPC: validate inputs; handle errors; avoid long-running operations on the main thread
  - Persistence: store host and user prefs safely; do not store tokens in plain files

## 5. Common Patterns
- Pipeline pattern on the Edge: stream_reader → preprocessor → inference_client → logger/metrics
- Lock-free ring buffers for stage decoupling and backpressure
- Health/metrics HTTP server for observability (/healthz, /status, /metrics, and /discover)
- Schema-first JSON contracts (shared/schemas/lpr_result.schema.json)
- Lazy initialization of heavy AI models and optional fallbacks (dummy detector)
- Config-by-example: *.env.example and config.json.example per package
- JWT auth and bearer-protected routes in Cloud API; CORS configured per environment

## 6. Do's and Don'ts
- Do
  - Keep schemas in sync across Edge/Cloud; validate outputs in CI
  - Structure logs as JSON with stable keys; rotate logs in production
  - Apply exponential backoff for RTSP reconnects and HTTP retries
  - Treat external dependencies as unreliable; handle timeouts and partial failures
  - Gate optional features behind env flags (e.g., CLOUD_SYNC_ENABLED)
  - Write deterministic, race-free code; add time abstractions for testability
  - Document any new endpoints and configuration keys
- Don't
  - Don’t block FastAPI event loop with CPU-bound tasks; avoid synchronous heavy I/O in handlers
  - Don’t change bbox formats casually; update schema and all consumers when necessary
  - Don’t expose secrets or tokens in code or logs; don’t commit real .env files or DBs
  - Don’t rely on direct require in Electron renderer; use a secure preload
  - Don’t allow unbounded queues or uncontrolled thread creation

## 7. Tools & Dependencies
- C++ (local_cpp)
  - OpenCV (video capture and image ops), libcurl (HTTP), nlohmann_json (JSON)
  - Build: CMake; outputs in build_*/ directories
- Python (ai_service)
  - FastAPI, Uvicorn, Pillow, numpy, orjson; optional ultralytics (YOLO), easyocr
  - Env: config.env.example; run with uvicorn main:app
- Python (cloud_api)
  - FastAPI, SQLModel/SQLite, PyJWT, Passlib, CORS middleware
  - JWT issuance via OAuth2PasswordBearer; demo login flow currently password::token
- Next.js (cloud_web)
  - React/TypeScript, ECharts, CSS; RTL setup via _document.tsx
- Electron (electron_app)
  - Electron main/preload; renderer HTML/JS; secure defaults enabled
- Setup quickstart (dev)
  - AI service: cd packages/ai_service && pip install -r requirements.txt && uvicorn main:app --host 0.0.0.0 --port 8000 --reload
  - C++ client: configure packages/local_cpp/config.json; build with CMake; run build_full/Release/local_cpp_client.exe
  - Cloud API: cd packages/cloud_api && pip install -r requirements.txt && uvicorn main:app --host 0.0.0.0 --port 9000 --reload
  - Cloud Web: cd packages/cloud_web && npm i && npm run dev
  - Electron: cd packages/electron_app && npm i && npm start

## 8. Other Notes
- Bounding box convention must be standardized and enforced via shared schema; update all producers/consumers together
- Consider replacing password::token with a JSON login payload (username, password, api_token) across cloud_web and cloud_api
- Large model deps (torch/ultralytics) are optional; guard imports and provide fallbacks; document hardware requirements
- Health server includes RTSP discovery (/discover) returning candidate URLs; optionally implement active probing
- Avoid using the root config.json; prefer package-level configs and documented env vars
- When adding features, follow EXTENSIBILITY.md guidelines and keep interfaces stable across tiers
