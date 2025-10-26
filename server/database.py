"""Database session management for the chat server."""
from __future__ import annotations

import os
from contextlib import contextmanager
from typing import Generator

from sqlalchemy import create_engine
from sqlalchemy.orm import Session, declarative_base, sessionmaker

DATABASE_URL_ENV = "MYSQL_DSN"
DEFAULT_DB_URL = "mysql+pymysql://icq:icq_password@localhost:3306/icq_chat"


def _build_engine():
    database_url = os.getenv(DATABASE_URL_ENV, DEFAULT_DB_URL)
    return create_engine(database_url, pool_pre_ping=True, future=True)


def _build_session_factory(bind_engine):
    return sessionmaker(bind=bind_engine, autoflush=False, autocommit=False, future=True)


engine = _build_engine()
SessionLocal = _build_session_factory(engine)
Base = declarative_base()


@contextmanager
def session_scope() -> Generator[Session, None, None]:
    """Provide a transactional scope around a series of operations."""
    session: Session = SessionLocal()
    try:
        yield session
        session.commit()
    except Exception:
        session.rollback()
        raise
    finally:
        session.close()
