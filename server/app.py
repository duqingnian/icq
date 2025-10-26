"""FastAPI application providing the chat backend."""
from __future__ import annotations

import os
from typing import List, Optional

from fastapi import Depends, FastAPI, HTTPException, Query, status
from sqlalchemy import and_, or_, select
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session

from . import auth, models, schemas
from .database import Base, engine
from .dependencies import get_current_user, get_db

app = FastAPI(title="ICQ Minimal Chat Server")


@app.on_event("startup")
def create_tables() -> None:
    Base.metadata.create_all(bind=engine)


@app.post("/register", response_model=schemas.UserView, status_code=status.HTTP_201_CREATED)
def register_user(payload: schemas.RegisterRequest, db: Session = Depends(get_db)):
    existing = db.query(models.User).filter(models.User.username == payload.username).one_or_none()
    if existing:
        raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="Username already exists")

    user = models.User(username=payload.username, password_hash=auth.get_password_hash(payload.password))
    db.add(user)
    db.flush()
    return schemas.UserView(id=user.id, username=user.username, created_at=user.created_at)


@app.post("/login", response_model=schemas.TokenResponse)
def login(payload: schemas.LoginRequest, db: Session = Depends(get_db)):
    user = db.query(models.User).filter(models.User.username == payload.username).one_or_none()
    if not user or not auth.verify_password(payload.password, user.password_hash):
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid credentials")

    token = auth.create_access_token(subject=user.username)
    return schemas.TokenResponse(access_token=token)


@app.post("/friends", status_code=status.HTTP_204_NO_CONTENT)
def add_friend(
    payload: schemas.FriendRequest,
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    if payload.friend_username == current_user.username:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail="Cannot add yourself as friend")

    friend = db.query(models.User).filter(models.User.username == payload.friend_username).one_or_none()
    if not friend:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Friend not found")

    for pair in ((current_user.id, friend.id), (friend.id, current_user.id)):
        friendship = models.Friendship(user_id=pair[0], friend_id=pair[1])
        db.add(friendship)
        try:
            db.flush()
        except IntegrityError:
            db.rollback()
    return None


@app.get("/friends", response_model=List[str])
def list_friends(
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    stmt = select(models.User.username).join(models.Friendship, models.Friendship.friend_id == models.User.id).where(
        models.Friendship.user_id == current_user.id
    )
    return [row[0] for row in db.execute(stmt)]


@app.post("/messages", status_code=status.HTTP_204_NO_CONTENT)
def send_message(
    payload: schemas.MessageCreateRequest,
    current_user: models.User = Depends(get_current_user),
    db: Session = Depends(get_db),
):
    recipient = db.query(models.User).filter(models.User.username == payload.recipient_username).one_or_none()
    if not recipient:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Recipient not found")

    friendship = db.query(models.Friendship).filter(
        models.Friendship.user_id == current_user.id,
        models.Friendship.friend_id == recipient.id,
    ).one_or_none()
    if not friendship:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Recipient is not your friend")

    message = models.Message(sender_id=current_user.id, recipient_id=recipient.id, content=payload.content)
    db.add(message)
    db.flush()
    return None


@app.get("/messages/{friend_username}", response_model=schemas.MessagesResponse)
def get_messages(
    friend_username: str,
    current_user: models.User = Depends(get_current_user),
    after: Optional[int] = Query(default=None, description="Return messages with id greater than this value"),
    limit: int = Query(default=50, ge=1, le=200),
    db: Session = Depends(get_db),
):
    friend = db.query(models.User).filter(models.User.username == friend_username).one_or_none()
    if not friend:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Friend not found")

    friendship = db.query(models.Friendship).filter(
        models.Friendship.user_id == current_user.id,
        models.Friendship.friend_id == friend.id,
    ).one_or_none()
    if not friendship:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Friendship not established")

    conditions = [
        or_(
            and_(models.Message.sender_id == current_user.id, models.Message.recipient_id == friend.id),
            and_(models.Message.sender_id == friend.id, models.Message.recipient_id == current_user.id),
        )
    ]
    if after is not None:
        conditions.append(models.Message.id > after)

    stmt = (
        select(models.Message)
        .where(*conditions)
        .order_by(models.Message.id)
        .limit(limit + 1)
    )

    rows = db.execute(stmt).scalars().all()
    next_after = None
    if len(rows) > limit:
        next_after = rows[-1].id
        rows = rows[:-1]

    messages = [
        schemas.MessageView(
            id=row.id,
            sender=row.sender.username,
            recipient=row.recipient.username,
            content=row.content,
            created_at=row.created_at,
        )
        for row in rows
    ]
    return schemas.MessagesResponse(messages=messages, next_after=next_after)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(
        "server.app:app",
        host=os.getenv("SERVER_HOST", "127.0.0.1"),
        port=int(os.getenv("SERVER_PORT", "8000")),
    )
