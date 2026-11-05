# Matcha API

Contract between the C backend and the React frontend. Agree on changes here before implementing them.

## Conventions

- **Base URL:** `http://localhost:8080` (`make`), or `https://localhost:8443` (`make tls`, self-signed).
- **Format:** JSON in and out. Errors are always `{"error": "..."}`.
- **Auth:** `POST /api/login` sets the `token` cookie (`HttpOnly; SameSite=Strict`). Send requests with `credentials: "include"`.
- **No cookie** on an auth route → `403 {"error": "Forbidden"}`. **Invalid or expired token** → `401 {"error": "Invalid or expired session"}`. On any route but login/logout, the frontend treats both as "session lost": it clears its auth state and sends the user to login.
- **Unknown route** → `404 {"error": "Not Found"}`.
- **Dates:** not unified yet. **Pagination:** none.

### Central preconditions

Every route declares what it needs at registration, and the router enforces it
before the handler runs — so these answers are identical on every endpoint and
are not repeated per route below. They are checked in this order:

| Condition | Response |
|---|---|
| Unknown route | **404** `Not Found` |
| `OPTIONS` (CORS preflight) | **204**, no body |
| Auth route, no `token` cookie | **403** `Forbidden` |
| Auth route, bad or expired token | **401** `Invalid or expired session` |
| Method not allowed on the route | **405** `Method Not Allowed` |
| Route needs a query string, none given | **400** `Missing query parameters` |
| Route needs a body, none given | **400** `Request body is required` |
| Route needs JSON, body does not parse | **400** `Invalid JSON` |
| Body over 8 MB | **413** `Request body too large` |

CORS: every response echoes the request's `Origin` in
`Access-Control-Allow-Origin` and adds `Access-Control-Allow-Credentials: true`,
including the router-level `400` / `404` / `405` / `500`. A request **without**
an `Origin` header is not cross-origin, so it gets no CORS header at all.
Preflight (`OPTIONS`) answers `204` with the same echoed origin plus
`Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS` and
`Access-Control-Allow-Headers: Content-Type`.

The origin is echoed, not matched against an allowlist, so any site can read an
authenticated response if the browser attaches the cookie — `SameSite=Strict`
is what currently prevents that. See To agree on.

The session cookie is `token=<jwt>; HttpOnly; Path=/; SameSite=Strict`, plus
`Secure` when the listener is TLS. It is left off in plain-HTTP mode on
purpose: a `Secure` cookie is dropped by the browser over `http://`, which
would break the plain dev listener.

## Summary

| Method | Path | Auth | Needs |
|---|---|---|---|
| POST | `/api/register` | — | JSON |
| POST | `/api/login` | — | JSON |
| POST | `/api/verify-email` | — | JSON · *not implemented* |
| POST | `/api/forgot-password` | — | JSON · *not implemented* |
| POST | `/api/reset-password` | — | JSON · *not implemented* |
| GET | `/api/validate` | ✔ | — |
| POST | `/api/logout` | ✔ | — |
| GET | `/api/profile/get` | ✔ | — |
| POST · PATCH | `/api/profile/update` | ✔ | JSON |
| GET | `/api/tags/get` | ✔ | — |
| POST | `/api/tags/insert` | ✔ | JSON |
| POST | `/api/tags/delete` | ✔ | JSON |
| POST | `/api/pics/insert` | ✔ | multipart |
| GET | `/api/pics/get` | ✔ | query |
| POST | `/api/pics/set_profile` | ✔ | JSON |
| POST | `/api/pics/delete` | ✔ | JSON |
| GET | `/api/chat` | ✔ | — |
| GET | `/api/chat/messages` | ✔ | query |
| WS | `/socket.io/` | ✔ | — |
| SIO | `send` event | ✔ | — |
| WS | `/ws` | — | — |

---

## Auth

### POST `/api/register`

```json
{"username": "", "email": "", "password": "", "first_name": "", "last_name": ""}
```

- **201** `{"success": true, "message": "Registration successful"}`
- **400** `<field> is required` (lowercase field name) · `Username already exists` · `Email already exists`
- **500** `Failed to register user`

### POST `/api/login`

```json
{"email": "", "password": ""}
```

- **200** `{"success": true, "message": "Login successful"}` + `Set-Cookie: token=...`
- **400** `Email is required` · `Password is required` · `Invalid credentials`
- **500** `Failed to generate token`

A wrong password and an unregistered email both answer `Invalid credentials`, so
the endpoint cannot be used to test whether an account exists.

### POST `/api/verify-email` (proposed, not implemented yet)

Called by the frontend page `/verify-email?token=<token>`, which is the link sent in the registration email. Tokens are single-use.

```json
{"token": ""}
```

- **200** `{"success": true, "message": "Email verified"}`
- **400** `Token is required` · `Invalid token` (unknown or already used)
- **410** `Token expired`
- **500** `Failed to verify email`

### POST `/api/forgot-password` (proposed, not implemented yet)

Called by the frontend page `/forgot-password`. Sends an email containing the link `/reset-password?token=<token>`. Tokens are single-use and expire.

```json
{"email": ""}
```

- **200** `{"success": true, "message": "If the email exists, a reset link was sent"}` — **same answer whether or not the email is registered**, to avoid revealing account existence. The frontend also hides `Email not found` if the backend ever returns it.
- **400** `Email is required`
- **500** `Failed to send email`

### POST `/api/reset-password` (proposed, not implemented yet)

Called by the frontend page `/reset-password?token=<token>`. Replaces the password and invalidates the token (and ideally existing sessions).

```json
{"token": "", "password": ""}
```

- **200** `{"success": true, "message": "Password updated"}`
- **400** `Token is required` · `Invalid token` (unknown or already used) · `Password is required` · `Password too weak` (any message containing "password" is shown on the field)
- **410** `Token expired`
- **500** `Failed to reset password`

### GET `/api/validate`

Tells the frontend whether a session is active and who owns it. Called once when the app loads and right after login.

- **200** `{"success": true, "message": "User is valid", "username": "<username>"}`

### POST `/api/logout`

- **200** `{"success": true, "message": "Logged out"}` + `Set-Cookie: token=deleted; ... Expires=Thu, 01 Jan 1970 00:00:00 GMT`

---

## Profile

### GET `/api/profile/get?id=<user_id>`

`id` optional, defaults to the logged-in user.

- **200**

  ```json
  {
    "id": 1, "username": "", "first_name": "", "last_name": "", "email": "",
    "bio": "", "location": "secretito", "fame_rating": 0, "last_online": "never",
    "email_verified": false,
    "tags": [{"id": 1, "name": "Music"}],
    "pictures": [{"id": 12, "is_profile": true}, {"id": 13, "is_profile": false}]
  }
  ```

- **404** `User not found` (also for a non-numeric `id`)

`pictures` gives ids, not file paths — fetch the bytes from
`/api/pics/get?id=<id>`. Exactly one entry has `is_profile: true` whenever the
user has any pictures. `location` is still a hard-coded placeholder.

`tags` gives `{id, name}` objects. It used to be an array of bare names, which
could neither be posted back to `/api/tags/insert` (it takes ids) nor matched
against `/api/tags/get`.

### POST · PATCH `/api/profile/update`

All fields optional, others ignored. An empty object is accepted as a no-op.

```json
{"first_name": "", "last_name": "", "email": "", "bio": ""}
```

- **200** `{"success": true, "message": "Profile updated successfully"}`
- **404** `User not found`
- **500** `Failed to update profile`

---

## Tags

### GET `/api/tags/get`

The whole tag catalogue, which is what `/api/tags/insert` ids refer to.

- **200**

  ```json
  {"tags": [{"id": 1, "name": "Music"}, {"id": 2, "name": "Movies"}]}
  ```

- **500** `Failed to retrieve tags`

The rows come from `db_ttag_insert_default_tags()`, which runs on every start
and is idempotent because `name` is unique — 21 tags: Music, Movies, Sports,
Travel, Food, Art, Technology, Gaming, Fitness, Books, Photography, Dancing,
Cooking, Hiking, Fashion, Writing, Yoga, Meditation, Crafts, Volunteering,
Gardening. `id` comes from a sequence, so a client has to read it here rather
than assume it: on a database recreated from scratch (`make fclean`) the names
are the same but the ids need not be. `name` is at most 32 characters. There is
no route to create a tag, and `user_tags` rows are constrained to real tags and
real users (`ON DELETE CASCADE`), so a deleted user or tag takes its mappings
with it.

### POST `/api/tags/insert`

```json
{"tags": [1, 4, 7]}
```

Tag ids, as listed by `/api/tags/get`. The request describes the tags the
caller wants, so it is idempotent: an id the user already has is counted in
`already_present` instead of failing, and repeats inside one array count once.

- **200**

  ```json
  {"success": true, "added": 2, "already_present": 1,
   "tags": [{"id": 1, "name": "Music"}, {"id": 4, "name": "Travel"}]}
  ```

  `tags` is the caller's whole set after the write, in the same shape as
  `/api/profile/get`, so a tag editor needs no follow-up request.

- **400** `tags is required` (missing, or not an array) ·
  `tags must not be empty` · `tags must contain positive integer ids` ·
  `tags takes at most 32 ids` · `Unknown tag id <N>`
- **500** `Failed to add tags` · `Failed to retrieve tags`

Every id is validated against the catalogue before anything is written, and the
first bad one is named in the error. `400` rather than `404` for an unknown id:
the ids arrive as a list inside the body, so the route itself is found and only
the body is wrong — unlike `/api/pics/*`, where the id *is* the resource being
addressed.

### POST `/api/tags/delete`

```json
{"tags": [1, 4]}
```

Same body, same validation and the same `400` set as `/api/tags/insert`.
Removing a tag the caller does not have is a no-op, counted in `not_present`.

- **200**

  ```json
  {"success": true, "removed": 1, "not_present": 1,
   "tags": [{"id": 4, "name": "Travel"}]}
  ```

- **500** `Failed to remove tags` · `Failed to retrieve tags`

It only ever touches the caller's own rows: deleting a tag another user has
answers `200` with `removed: 0`.

---

## Pictures

Up to **5 pictures** per user, one of them the profile picture.

Server-side rules, all in `api/pics/pics.c`:

- The uploaded **filename is discarded**. The server generates the stored name
  (`u<uid>_<epoch>_<16 hex>.<ext>`) and only that basename is persisted, so a
  client can never influence where the file lands.
- The **format is taken from the magic bytes**, not from the filename or the
  part's `Content-Type`. PNG, JPEG, GIF and WebP only.
- **5 MB per picture**, **5 pictures per user**.
- The **first upload automatically becomes the profile picture**
- Another user's picture answers **404**, never 403, so ids are not enumerable.

### POST `/api/pics/insert`

`multipart/form-data` with one file part. Any part carrying a `filename` is
taken as the file; additional file parts are ignored. Other fields are ignored.

- **201** `{"id": 12, "is_profile": true}`
- **400** `Expected multipart/form-data` · `Missing or malformed boundary` · `Malformed multipart body` · `No file part in the request`
- **409** `At most 5 pictures; delete one first`
- **413** `Picture larger than 5242880 bytes`
- **415** `Only PNG, JPEG, GIF and WebP images are accepted`
- **500** `Storage unavailable` · `Failed to store the picture` · `Failed to record the picture` · `Out of memory`

The quota is checked before the body is parsed, so an over-quota upload is
rejected without reading the file.

### GET `/api/pics/get?id=<picture_id>`

Any logged-in user may fetch any picture; they are profile content.

- **200** raw image bytes, with the real `Content-Type`
  (`image/png` · `image/jpeg` · `image/gif` · `image/webp`) and
  `X-Content-Type-Options: nosniff`
- **400** `Missing 'id' parameter` · `Invalid 'id' parameter`
- **404** `Picture not found` (unknown id, or the file is gone from disk)
- **500** `Stored picture is unusable` · `Failed to read the picture` · `Out of memory`

### POST `/api/pics/set_profile`

```json
{"id": 12}
```

Moves the profile flag onto that picture and clears it from the caller's others.
Must be one of the caller's own pictures.

- **200** `{"message": "Profile picture updated"}`
- **400** `Expected a positive integer 'id'`
- **404** `Picture not found` (unknown, or owned by someone else)
- **500** `Failed to set the profile picture` · `Failed to read the existing pictures`

### POST `/api/pics/delete`

```json
{"id": 12}
```

Removes the row and the file. If the deleted picture was the profile picture and
others remain, the oldest remaining one is promoted.

- **200** `{"message": "Picture deleted"}`
- **400** `Expected a positive integer 'id'`
- **404** `Picture not found` (unknown, or owned by someone else)
- **500** `Failed to delete the picture`

---

## Chat

### GET `/api/chat`

Stub.

- **200** `{"message": "Hello from chat!"}`

### GET `/api/chat/messages?id=<user_id>`

Messages sent by me to that user.

- **200**

  ```json
  {"messages": [{"id": 1, "sender_id": 1, "recipient_id": 2, "content": "hi", "is_read": false, "timestamp": "Wed Oct 21 14:03:12 2026"}]}
  ```

- **400** `Missing 'id' parameter`
- **404** `Requested chat not found`

---

## Real-time

### `/socket.io/`

Socket.IO v4, WebSocket transport only. Auth via cookie.

```js
io("http://localhost:8080", { transports: ["websocket"], withCredentials: true });
```

### Event `send` (client → server)

```js
socket.emit("send", "hello");
```

No ack, nothing pushed to the recipient.

### `/ws`

Raw WebSocket. Connects, no handlers.

---

## Known issues

- Missing cookie on an auth route answers `403 Forbidden` (should be `401`). Until then the frontend also treats exactly `403 {"error": "Forbidden"}` as "session lost".
- Unknown-route `404` always sends `Access-Control-Allow-Origin: http://localhost:8000` instead of the request's origin, so a browser on another origin sees a network error, not the 404.
- `send` recipient is hard-coded to user 2.
- No way to get a picture ID (profile returns file paths, upload returns nothing).
- `/api/pics/get` without a query string crashes.
- `/api/profile/get?id=` exposes other users' email.
- Method not checked outside auth routes.

## To agree on

- `/api/auth/*` naming
- Whether a missing cookie should be `401` rather than `403` (a bad token is already `401`)
- `/api/verify-email` contract (frontend relies on 400 = invalid, 410 = expired)
- `/api/forgot-password` + `/api/reset-password` contract (frontend pages exist, backend pending: issue #12 cannot be verified end to end until then)
- Whether the frontend downscales pictures before upload (the server stores and serves whatever it is given, at full size)
- Whether `Access-Control-Allow-Origin` should be an allowlist (the configured
  frontend origin) rather than an echo of whatever `Origin` arrives
- CSRF, date format (ISO 8601?), pagination
