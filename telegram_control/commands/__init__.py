"""Command registration."""

from telegram.ext import Application, CommandHandler, MessageHandler, filters

from telegram_control.commands.ai_devops_cmd import build_now_cmd, code_cmd, git_push_now_cmd, git_status_now_cmd, task_diff_cmd, task_log_cmd, tasks_cmd
from telegram_control.commands.approve_cmd import approve_cmd, reject_cmd
from telegram_control.commands.build_cmd import build_cmd
from telegram_control.commands.camera_cmd import cam10_cmd, photo_cmd
from telegram_control.commands.deploy_cmd import deploy_cmd
from telegram_control.commands.git_cmd import gitstatus_cmd
from telegram_control.commands.help_cmd import help_cmd
from telegram_control.commands.logs_cmd import logs_cmd
from telegram_control.commands.project_cmd import run_project_cmd, stop_project_cmd
from telegram_control.commands.result_cmd import ask_result_cmd, result_cmd
from telegram_control.commands.start_cmd import start_cmd
from telegram_control.commands.status_cmd import status_cmd
from telegram_control.commands.task_cmd import task_cmd


def register_commands(app: Application) -> None:
    app.add_handler(CommandHandler("start", start_cmd))
    app.add_handler(CommandHandler("help", help_cmd))
    app.add_handler(CommandHandler("cam10", cam10_cmd))
    app.add_handler(CommandHandler("photo", photo_cmd))
    app.add_handler(CommandHandler("status", status_cmd))
    app.add_handler(CommandHandler("gitstatus", gitstatus_cmd))
    app.add_handler(CommandHandler("task", task_cmd))
    app.add_handler(CommandHandler("code", code_cmd))
    app.add_handler(CommandHandler("tasks", tasks_cmd))
    app.add_handler(CommandHandler("build", build_cmd))
    app.add_handler(CommandHandler("flash", build_cmd))
    app.add_handler(CommandHandler("deploy", deploy_cmd))
    app.add_handler(CommandHandler("result", result_cmd))
    app.add_handler(CommandHandler("logs", logs_cmd))
    app.add_handler(CommandHandler("log", task_log_cmd))
    app.add_handler(CommandHandler("diff", task_diff_cmd))
    app.add_handler(CommandHandler("ask_result", ask_result_cmd))
    app.add_handler(CommandHandler("run_project", run_project_cmd))
    app.add_handler(CommandHandler("stop_project", stop_project_cmd))
    app.add_handler(CommandHandler("approve", approve_cmd))
    app.add_handler(CommandHandler("reject", reject_cmd))
    app.add_handler(CommandHandler("git_status", git_status_now_cmd))
    app.add_handler(CommandHandler("git_push", git_push_now_cmd))
    app.add_handler(CommandHandler("build_now", build_now_cmd))
    app.add_handler(MessageHandler(filters.Regex(r"^/git-status(?:\s|$)"), git_status_now_cmd))
    app.add_handler(MessageHandler(filters.Regex(r"^/git-push(?:\s|$)"), git_push_now_cmd))
