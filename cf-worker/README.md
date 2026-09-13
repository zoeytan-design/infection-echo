# Infection Echo — API Proxy (Cloudflare Worker)

Holds the OpenAI and ElevenLabs API keys server-side so they never appear in
`index.html` or any file the browser downloads. The front end calls this
worker instead of calling OpenAI/ElevenLabs directly, authenticating with a
Firebase anonymous-auth ID token — no password, no re-typed key, and nothing
a visitor can steal by viewing page source.

## One-time setup

### 1. Enable Anonymous auth in Firebase
Firebase Console → your project (`encho-20101`) → **Authentication** →
**Sign-in method** → enable **Anonymous**. This lets `index.html` sign every
visitor in silently (no UI, no prompt) so it can get a token to call this
worker.

### 2. Deploy the worker
From this folder:

```bash
npm install
npx wrangler login          # opens a browser to log into your Cloudflare account
npx wrangler secret put OPENAI_API_KEY        # paste your sk-... key when prompted
npx wrangler secret put ELEVENLABS_API_KEY    # paste your sk_... key when prompted
npx wrangler deploy
```

The last command prints your worker's URL, e.g.:

```
https://infection-echo-proxy.<your-subdomain>.workers.dev
```

Copy that URL — it goes into `index.html` as `WORKER_BASE` (see the main
README / ask Claude to wire it in).

### 3. (Optional) Restrict CORS
`wrangler.toml` sets `ALLOWED_ORIGIN = "*"` by default. Once you know the
exact URL the site is served from (e.g. your GitHub Pages URL), change it to
that exact origin and redeploy (`npx wrangler deploy`). This isn't the main
security boundary (the Firebase token check is), but it's a good extra
layer.

## Updating a key later
If a key leaks or needs rotating, just re-run the relevant `secret put`
command and redeploy — nothing in `index.html` or git needs to change.

## Endpoints

| Method | Path | Forwards to |
|---|---|---|
| POST | `/openai/v1/responses` | `https://api.openai.com/v1/responses` |
| GET | `/eleven/v1/voices` | `https://api.elevenlabs.io/v1/voices` |
| POST | `/eleven/v1/text-to-speech/:voiceId/stream` | `https://api.elevenlabs.io/v1/text-to-speech/:voiceId/stream` |
| GET | `/health` | (local — no upstream call, no auth required) |

All routes except `/health` require `Authorization: Bearer <Firebase ID token>`.
