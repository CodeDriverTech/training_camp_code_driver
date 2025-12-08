import os
import subprocess
import sys
from pathlib import Path

from dotenv import load_dotenv


def ensure_env_var(name: str) -> str:
    """读取必须存在的环境变量，若为空则报错提示。"""
    value = os.getenv(name)
    if not value:
        raise RuntimeError(f"缺少必要的环境变量：{name}")
    return value


def main() -> None:
    load_dotenv()

    # 允许通过 CLAUDE_CLI_PATH 覆盖 CLI 命令（默认直接调用 claude）
    cli_path = os.getenv("CLAUDE_CLI_PATH", "claude")
    prompt = os.getenv("CLAUDE_CLI_PROMPT", "Hello from Claude CLI")
    model = os.getenv("CLAUDE_CLI_MODEL", "claude-3-5-sonnet-20241022")

    api_key = ensure_env_var("ANTHROPIC_API_KEY")
    base_url = os.getenv("ANTHROPIC_API_URL")

    cmd = [cli_path, "--print"]
    if model:
        cmd.extend(["--model", model])

    env = os.environ.copy()
    env["ANTHROPIC_API_KEY"] = api_key
    if base_url:
        env["ANTHROPIC_API_URL"] = base_url

    print("准备执行命令：", " ".join(cmd))
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False,
            input=prompt,
            env=env,
        )
    except FileNotFoundError:
        raise RuntimeError(
            f"无法找到 Claude CLI 可执行文件（尝试路径：{cli_path}）。"
            "请确认已通过 `npm install -g @anthropic-ai/claude-code` 安装，并将其加入 PATH。"
        )

    print("退出码：", result.returncode)
    if result.stdout:
        print("----- CLI STDOUT -----")
        print(result.stdout)
    if result.stderr:
        print("----- CLI STDERR -----", file=sys.stderr)
        print(result.stderr, file=sys.stderr)

    if result.returncode != 0:
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
