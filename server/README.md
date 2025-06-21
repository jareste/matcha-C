# API Documentation

## Server Features

* User registration
* User login
* Session validation
* User logout
* Real-time connections (WebSocket and Socket.IO) — note: connection endpoints exist but no handlers implemented yet

## Endpoints

### POST `/api/register`

* **Authentication:** None
* **Request Body (application/json):**

  ```json
  {
    "username": "string",
    "email": "string",
    "password": "string",
    "first_name": "string",
    "last_name": "string"
  }
  ```
* **Responses:**

  * **201 Created**

    ```json
    {"success":true, "message":"Registration successful"}
    ```
  * **400 Bad Request**

    * Invalid JSON: `{"error":"Invalid JSON"}`
    * Missing fields: `{"error":"<Field> is required"}`
    * Username already exists: `{"error":"Username already exists"}`
    * Email already exists: `{"error":"Email already exists"}`
    * Generic bad request: `{"error":"Bad Request"}`
  * **500 Internal Server Error**

    ```json
    {"error":"Failed to register user"}
    ```

### POST `/api/login`

* **Authentication:** None
* **Request Body (application/json):**

  ```json
  {
    "email": "string",
    "password": "string"
  }
  ```
* **Responses:**

  * **200 OK**

    * Sets cookie: `Set-Cookie: token=<JWT>; HttpOnly; Path=/; SameSite=Strict`

    ```json
    {"success":true, "message":"Login successful"}
    ```
  * **400 Bad Request**

    * Invalid JSON: `{"error":"Invalid JSON"}`
    * Missing fields: `{"error":"Email is required"}` or `{"error":"Password is required"}`
    * Email not found: `{"error":"Email not found"}`
    * Invalid credentials: `{"error":"Invalid credentials"}`
  * **500 Internal Server Error**

    ```json
    {"error":"Failed to generate token"}
    ```

### GET `/api/validate`

* **Authentication:** Required (token cookie)
* **Responses:**

  * **200 OK**

    ```json
    {"success":true, "message":"User is valid", "username":"<username>"}
    ```
  * **405 Method Not Allowed**

    ```json
    {"error":"Method Not Allowed"}
    ```

### POST `/api/logout`

* **Authentication:** Required (token cookie)
* **Responses:**

  * **200 OK**

    * Sets cookie: `Set-Cookie: token=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT`
    * Response body: *(empty)*
  * **405 Method Not Allowed**

    ```json
    {"error":"Method Not Allowed"}
    ```

## WebSocket & Socket.IO Connections

* **Endpoints available:** `/ws` and `/socket.io`
* **Status:** no request handlers have been implemented yet, so just connection can be tested.

## Notes

* **Password Storage:** Currently the password is stored directly in `password_hash`. Implement proper hashing before production.
