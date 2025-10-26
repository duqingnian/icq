# Chat Server Setup

This repository now includes a minimal FastAPI-based backend under `server/` that satisfies the
registration, login, friend management, and text messaging requirements. The code is intended to run
on Debian or other Linux environments with Python 3.10+.

## Python Environment

1. Create and activate a virtual environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```
2. Install the backend dependencies:
   ```bash
   pip install -r server/requirements.txt
   ```

Set the following environment variables before running any scripts or the server:

* `MYSQL_DSN` – SQLAlchemy connection string, e.g. `mysql+pymysql://icq:icq_password@localhost:3306/icq_chat`.
* `JWT_SECRET_KEY` – secret string used to sign login tokens (choose a strong random value).
* Optionally `SERVER_HOST` / `SERVER_PORT` – used by the development launcher in `server/app.py`.

## Database Schema

Create the MySQL database and tables with the statements below. Adjust database and user names to
match your environment.

```sql
CREATE DATABASE IF NOT EXISTS icq_chat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE icq_chat;

CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(64) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS friendships (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT uq_friend_pair UNIQUE (user_id, friend_id),
    CONSTRAINT fk_friendships_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_friendships_friend FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id INT NOT NULL,
    recipient_id INT NOT NULL,
    content VARCHAR(2048) NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_messages_sender FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_messages_recipient FOREIGN KEY (recipient_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

## Running the Server

To expose the API on your Debian host (45.58.179.79), run:

```bash
export MYSQL_DSN="mysql+pymysql://icq:icq_password@45.58.179.79:3306/icq_chat"
export JWT_SECRET_KEY="replace-with-strong-secret"
uvicorn server.app:app --host 45.58.179.79 --port 8000
```

Alternatively, you can run `python -m server.app` which uses the `SERVER_HOST` and `SERVER_PORT`
environment variables (defaulting to `127.0.0.1:8000`). Setting `SERVER_HOST=45.58.179.79` ensures
the service binds directly to that address without editing source code. If you do want to hardcode
another host or port, edit the `uvicorn.run` call at the bottom of `server/app.py`.

## Available API Endpoints

* `POST /register` – create a new user (used internally by the CLI script as well).
* `POST /login` – authenticate with username and password. Returns a bearer token.
* `POST /friends` – add a friend relationship. Requires a bearer token.
* `GET /friends` – list usernames of existing friends. Requires a bearer token.
* `POST /messages` – send a text message to an existing friend. Requires a bearer token.
* `GET /messages/{friend_username}` – retrieve the conversation history, with optional `after` query
  parameter for pagination.

All authenticated requests expect an `Authorization: Bearer <token>` header with the value returned
by the login endpoint.

## Registering Users from the Command Line

Use the dedicated helper script to create accounts without hitting the HTTP endpoint:

```bash
python -m server.user_register alice --password "SuperSecret123"
```

If `--password` is omitted, the script will interactively prompt for it.
