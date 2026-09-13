// Infection Echo — API key proxy
//
// Holds OPENAI_API_KEY / ELEVENLABS_API_KEY as Cloudflare secrets (never
// shipped to the browser). Every request must carry a valid Firebase
// Authentication ID token (anonymous sign-in is enough) in the
// `Authorization: Bearer <token>` header — that's the gate that stops
// strangers from spending your API credits. CORS is left open by default
// since the real access control is the token check, not the browser's
// same-origin policy (a non-browser client can ignore CORS entirely).

import { createRemoteJWKSet, jwtVerify } from 'jose';

const FIREBASE_JWKS_URL =
  'https://www.googleapis.com/service_accounts/v1/jwk/securetoken@system.gserviceaccount.com';

let jwks = null;
function getJWKS() {
  if (!jwks) jwks = createRemoteJWKSet(new URL(FIREBASE_JWKS_URL));
  return jwks;
}

function corsHeaders(origin) {
  return {
    'Access-Control-Allow-Origin': origin || '*',
    'Access-Control-Allow-Methods': 'GET,POST,OPTIONS',
    'Access-Control-Allow-Headers': 'Authorization, Content-Type',
    'Access-Control-Max-Age': '86400',
  };
}

function jsonResponse(obj, status, origin) {
  return new Response(JSON.stringify(obj), {
    status: status || 200,
    headers: { 'Content-Type': 'application/json', ...corsHeaders(origin) },
  });
}

async function verifyFirebaseToken(request, env) {
  const authHeader = request.headers.get('Authorization') || '';
  const match = authHeader.match(/^Bearer\s+(.+)$/i);
  if (!match) throw new Error('missing bearer token');
  const token = match[1];

  const { payload } = await jwtVerify(token, getJWKS(), {
    issuer: 'https://securetoken.google.com/' + env.FIREBASE_PROJECT_ID,
    audience: env.FIREBASE_PROJECT_ID,
  });

  if (!payload.sub) throw new Error('token has no subject');
  return payload;
}

export default {
  async fetch(request, env, _ctx) {
    const url = new URL(request.url);
    const origin = env.ALLOWED_ORIGIN || '*';

    if (request.method === 'OPTIONS') {
      return new Response(null, { headers: corsHeaders(origin) });
    }

    if (url.pathname === '/health') {
      return jsonResponse({ ok: true }, 200, origin);
    }

    try {
      await verifyFirebaseToken(request, env);
    } catch (e) {
      return jsonResponse({ error: 'unauthorized', detail: e.message }, 401, origin);
    }

    try {
      // ── OpenAI Responses API ──────────────────────────────────────
      if (url.pathname === '/openai/v1/responses' && request.method === 'POST') {
        const body = await request.text();
        const upstream = await fetch('https://api.openai.com/v1/responses', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            Authorization: 'Bearer ' + env.OPENAI_API_KEY,
          },
          body,
        });
        const text = await upstream.text();
        return new Response(text, {
          status: upstream.status,
          headers: { 'Content-Type': 'application/json', ...corsHeaders(origin) },
        });
      }

      // ── ElevenLabs: list voices ───────────────────────────────────
      if (url.pathname === '/eleven/v1/voices' && request.method === 'GET') {
        const upstream = await fetch('https://api.elevenlabs.io/v1/voices', {
          headers: { 'xi-api-key': env.ELEVENLABS_API_KEY },
        });
        const text = await upstream.text();
        return new Response(text, {
          status: upstream.status,
          headers: { 'Content-Type': 'application/json', ...corsHeaders(origin) },
        });
      }

      // ── ElevenLabs: text-to-speech (streamed) ─────────────────────
      const ttsMatch = url.pathname.match(/^\/eleven\/v1\/text-to-speech\/([^/]+)\/stream$/);
      if (ttsMatch && request.method === 'POST') {
        const voiceId = decodeURIComponent(ttsMatch[1]);
        const body = await request.text();
        const upstream = await fetch(
          'https://api.elevenlabs.io/v1/text-to-speech/' + voiceId + '/stream',
          {
            method: 'POST',
            headers: {
              'xi-api-key': env.ELEVENLABS_API_KEY,
              'Content-Type': 'application/json',
            },
            body,
          }
        );
        const headers = new Headers(corsHeaders(origin));
        headers.set('Content-Type', upstream.headers.get('Content-Type') || 'audio/mpeg');
        return new Response(upstream.body, { status: upstream.status, headers });
      }

      return jsonResponse({ error: 'not_found' }, 404, origin);
    } catch (e) {
      return jsonResponse({ error: 'proxy_error', detail: e.message }, 502, origin);
    }
  },
};
