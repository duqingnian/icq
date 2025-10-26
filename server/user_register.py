"""Command line helper for registering users via username and password."""

from __future__ import annotations

import argparse
import getpass
import sys

from sqlalchemy.exc import IntegrityError

from . import auth, models
from .database import session_scope


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Register a new chat user")
    parser.add_argument("username", help="Unique username")
    parser.add_argument(
        "--password",
        help="Password for the new account. If omitted, you will be prompted securely.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    password = args.password or getpass.getpass("Password: ")
    if len(password) < 6:
        print("Password must be at least 6 characters long", file=sys.stderr)
        return 1

    with session_scope() as session:
        user = models.User(username=args.username, password_hash=auth.get_password_hash(password))
        session.add(user)
        try:
            session.flush()
        except IntegrityError:
            print("Username already exists", file=sys.stderr)
            return 1

    print(f"User '{args.username}' created successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
