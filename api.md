# Matcha API

Contract between the C backend and the React frontend. Agree on changes here before implementing them.

## Conventions

- **Base URL:** `http://localhost:8080`
- **Format:** JSON in and out. Errors are always `{"error": "..."}`.
- **Auth:** `POST /api/login` sets the `token` cookie (`HttpOnly; SameSite=Strict`). Send requests with `credentials: "include"`.
- **No cookie** on an auth route → `403 {"error": "Forbidden"}`. **Invalid token** → connection closed, no response.
- **Unknown route** → `404 {"error": "Not Found"}`.
- **Dates:** not unified yet. **Pagination:** none.

## Summary

| Method | Path | Auth |
|---|---|---|
| POST | `/api/register` | — |
| POST | `/api/login` | — |
| POST | `/api/verify-email` | — |
| POST | `/api/forgot-password` | — |
| POST | `/api/reset-password` | — |
| GET | `/api/validate` | ✔ |
| POST | `/api/logout` | ✔ |
| GET | `/api/profile/get` | ✔ |
| POST | `/api/profile/update` | ✔ |
| POST | `/api/tags/insert` | ✔ |
| POST | `/api/pics/insert` | ✔ |
| GET | `/api/pics/get` | ✔ |
| GET | `/api/chat` | ✔ |
| GET | `/api/chat/messages` | ✔ |
| WS | `/socket.io/` | ✔ |
| SIO | `send` event | ✔ |
| WS | `/ws` | — |

---

## Auth

### POST `/api/register`

```json
{"username": "", "email": "", "password": "", "first_name": "", "last_name": ""}
```

- **201** `{"success": true, "message": "Registration successful"}`
- **400** `Invalid JSON` · `<Field> is required` · `Username already exists` · `Email already exists`
- **405** `Method Not Allowed`
- **500** `Failed to register user`

### POST `/api/login`

```json
{"email": "", "password": ""}
```

- **200** `{"success": true, "message": "Login successful"}` + `Set-Cookie: token=...`
- **400** `Invalid JSON` · `Email is required` · `Password is required` · `Email not found` · `Invalid credentials`
- **405** `Method Not Allowed`
- **500** `Failed to generate token`

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
- **400** `Invalid JSON` · `Email is required`
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

- **200** `{"success": true, "message": "User is valid", "username": "<username>"}`
- **405** `Method Not Allowed`

### POST `/api/logout`

- **200** empty body, cookie cleared
- **405** `Method Not Allowed`

---

## Profile

### GET `/api/profile/get?id=<user_id>`

`id` optional, defaults to the logged-in user.

- **200**

  ```json
  {
    "id": 1, "username": "", "first_name": "", "last_name": "", "email": "",
    "bio": "", "location": "secretito", "fame_rating": 0, "last_online": "never",
    "email_verified": false, "tags": ["music"], "pictures": ["/tmp/matCha/uploads/a.jpg"]
  }
  ```

- **404** `User not found`

### POST `/api/profile/update`

All fields optional, others ignored.

```json
{"first_name": "", "last_name": "", "email": "", "bio": ""}
```

- **200** `{"success": true, "message": "Profile updated successfully"}`
- **400** `No data provided` · `Invalid JSON`
- **404** `User not found`
- **500** `Failed to update profile`

---

## Tags

### POST `/api/tags/insert`

```json
{"tags": [1, 4, 7]}
```

Tag IDs. Invalid ones are silently skipped.

- **200** `{"message": "Tags successfully added"}`
- **400** `Invalid JSON body` · `Invalid or missing 'tags' in request body`

---

## Pictures

### POST `/api/pics/insert`

`multipart/form-data` with one file.

- **200** `{"message": "Picture uploaded successfully"}`
- **400** `Expected multipart/form-data` · `Missing boundary`
- **500** `Failed to save picture`

### GET `/api/pics/get?id=<picture_id>`

- **200** raw image (`Content-Type: image/jpeg`)
- **400** `Missing 'id' parameter` · `Invalid 'id' parameter`
- **404** `Picture not found`
- **500** `File missing on disk`

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

- **400** `Bad Request - Missing Query Parameters` · `Missing 'id' parameter`
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

- Invalid token → no response (should be `401`).
- `send` recipient is hard-coded to user 2.
- No way to get a picture ID (profile returns file paths, upload returns nothing).
- `/api/pics/get` without a query string crashes.
- `/api/profile/get?id=` exposes other users' email.
- Method not checked outside auth routes.

## To agree on

- `GET /api/me` (or keep `/api/validate`)
- `/api/auth/*` naming
- 401 vs 403 vs 400
- `/api/verify-email` contract (frontend relies on 400 = invalid, 410 = expired)
- `/api/forgot-password` + `/api/reset-password` contract (frontend pages exist, backend pending: issue #12 cannot be verified end to end until then)
- CSRF, date format (ISO 8601?), pagination
