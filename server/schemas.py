"""Pydantic schemas for API requests and responses."""
from __future__ import annotations

from datetime import datetime
from typing import List, Optional

from pydantic import BaseModel, constr


class RegisterRequest(BaseModel):
    username: constr(min_length=3, max_length=64)
    password: constr(min_length=6, max_length=128)


class UserView(BaseModel):
    id: int
    username: str
    created_at: datetime


class LoginRequest(BaseModel):
    username: constr(min_length=3, max_length=64)
    password: constr(min_length=6, max_length=128)


class TokenResponse(BaseModel):
    access_token: str
    token_type: str = "bearer"


class FriendRequest(BaseModel):
    friend_username: constr(min_length=3, max_length=64)


class MessageCreateRequest(BaseModel):
    recipient_username: constr(min_length=3, max_length=64)
    content: constr(min_length=1, max_length=2048)


class MessageView(BaseModel):
    id: int
    sender: str
    recipient: str
    content: str
    created_at: datetime


class MessagesResponse(BaseModel):
    messages: List[MessageView]
    next_after: Optional[int] = None
