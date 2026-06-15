from __future__ import annotations

from enum import Enum
from functools import wraps
from typing import Callable, Iterable

from telegram import Update
from telegram.ext import ContextTypes


class Role(str, Enum):
    ADMIN = "ADMIN"
    OPERATOR = "OPERATOR"
    VIEWER = "VIEWER"


def get_user_role(user_id: int | None, settings) -> Role | None:
    if user_id is None:
        return None
    if user_id in settings.admin_user_ids:
        return Role.ADMIN
    if user_id in settings.operator_user_ids:
        return Role.OPERATOR
    if user_id in settings.viewer_user_ids:
        return Role.VIEWER
    return None


def is_allowed_chat(update: Update, settings) -> bool:
    chat = update.effective_chat
    if chat is None:
        return False
    if chat.type == "private":
        return settings.allowed_chat_id is not None and chat.id == settings.allowed_chat_id
    return settings.allowed_group_id is not None and chat.id == settings.allowed_group_id


def require_roles(*roles: Role):
    def decorator(func: Callable):
        @wraps(func)
        async def wrapper(update: Update, context: ContextTypes.DEFAULT_TYPE, *args, **kwargs):
            settings = context.application.bot_data["settings"]
            if not is_allowed_chat(update, settings):
                await update.effective_message.reply_text("Không có quyền sử dụng bot này.")
                return
            role = get_user_role(update.effective_user.id if update.effective_user else None, settings)
            if role not in set(roles):
                await update.effective_message.reply_text("Không có quyền sử dụng bot này.")
                return
            context.user_data["role"] = role
            return await func(update, context, *args, **kwargs)
        return wrapper
    return decorator


ADMIN_ONLY = (Role.ADMIN,)
ADMIN_OPERATOR = (Role.ADMIN, Role.OPERATOR)
ALL_AUTHORIZED = (Role.ADMIN, Role.OPERATOR, Role.VIEWER)
