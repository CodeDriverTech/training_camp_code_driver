import json
import os
import random
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Optional

import requests
from dotenv import load_dotenv

BASE_DIR = Path(__file__).resolve().parent
load_dotenv(dotenv_path=BASE_DIR / ".env", override=False)


class ModelAPIConfigError(RuntimeError):
    """在 .env 未正确配置模型 API 时抛出。"""


class ModelAPIRequestError(RuntimeError):
    """统一的模型推理请求异常。"""


@dataclass
class ModelResponse:
    content: str
    metadata: Optional[Dict[str, Any]] = None


class ModelAPIClient:
    """面向 WerewolfTest 的统一调用封装，支持多厂商/多模型与本地 mock。"""

    def __init__(
        self,
        base_url: Optional[str] = None,
        api_key: Optional[str] = None,
        timeout: Optional[float] = None,
        config_path: Optional[str] = None,
    ):
        self.base_url = base_url or os.environ.get("MODEL_API_BASE_URL")
        self.api_key = api_key or os.environ.get("MODEL_API_KEY")
        self.timeout = timeout or float(os.environ.get("MODEL_API_TIMEOUT", 45))
        self.max_retries = int(os.environ.get("MODEL_API_RETRIES", 2))
        self.retry_backoff_factor = float(os.environ.get("MODEL_API_BACKOFF_FACTOR", 1.5))
        self.retry_backoff_cap = float(os.environ.get("MODEL_API_BACKOFF_CAP", 6))
        self.circuit_threshold = int(os.environ.get("MODEL_API_CIRCUIT_THRESHOLD", 3))
        self.circuit_cooldown = float(os.environ.get("MODEL_API_CIRCUIT_COOLDOWN", 30))
        self._route_failures: Dict[str, Dict[str, float]] = {}
        raw_config_path = config_path or os.environ.get("MODEL_API_CONFIG")
        if raw_config_path:
            self.config_path = str((BASE_DIR / raw_config_path).resolve()) if not os.path.isabs(raw_config_path) else raw_config_path
        else:
            self.config_path = None
        self.providers: Dict[str, Dict[str, Any]] = {}
        self.model_routes: Dict[str, Dict[str, Any]] = {}
        if self.config_path:
            self._load_config(self.config_path)
        if not self.base_url and not self.providers:
            raise ModelAPIConfigError("请配置 MODEL_API_BASE_URL 或提供 MODEL_API_CONFIG 文件")

    def _load_config(self, path: str):
        config_file = Path(path)
        if not config_file.exists():
            raise ModelAPIConfigError(f"未找到模型配置文件: {config_file}")
        try:
            data = json.loads(config_file.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            raise ModelAPIConfigError(f"解析 {config_file} 失败: {exc}") from exc
        self.providers = data.get("providers", {})
        self.model_routes = data.get("models", {})

    def _resolve_env_value(self, raw_value: Optional[str]) -> Optional[str]:
        if not raw_value:
            return None
        if isinstance(raw_value, str) and raw_value.startswith("env:"):
            env_key = raw_value.split(":", 1)[1]
            return os.environ.get(env_key)
        return raw_value

    def _resolve_api_key(self, raw_key: Optional[str]) -> Optional[str]:
        return self._resolve_env_value(raw_key)

    def _resolve_route(self, model_name: str) -> Dict[str, Any]:
        route_cfg = self.model_routes.get(model_name, {})
        provider_name = route_cfg.get("provider")
        provider_cfg = self.providers.get(provider_name, {}) if provider_name else {}
        mode = provider_cfg.get("mode", "http")
        if mode == "mock":
            return {
                "mode": "mock",
                "mock_profile": provider_cfg.get("mock_profile", {}),
                "model_alias": route_cfg.get("remote_model") or model_name,
            }

        base_url_cfg = provider_cfg.get("base_url")
        base_url = self._resolve_env_value(base_url_cfg) or self.base_url
        api_key = self._resolve_api_key(provider_cfg.get("api_key")) or self.api_key
        if not base_url or not api_key:
            raise ModelAPIConfigError(f"模型 {model_name} 缺少可用的 base_url 或 api_key 配置")
        timeout = provider_cfg.get("timeout", self.timeout)
        headers = dict(provider_cfg.get("headers", {}))
        auth_header = provider_cfg.get("auth_header", "Authorization")
        auth_scheme = provider_cfg.get("auth_scheme", "Bearer")
        headers.setdefault(auth_header, f"{auth_scheme} {api_key}".strip())
        remote_model = route_cfg.get("remote_model") or provider_cfg.get("default_model") or model_name
        extra_body = {**provider_cfg.get("body", {}), **route_cfg.get("body", {})}
        request_format = provider_cfg.get("request_format", "custom")
        return {
            "mode": "http",
            "base_url": base_url,
            "timeout": timeout,
            "headers": headers,
            "remote_model": remote_model,
            "extra_body": extra_body,
            "request_format": request_format,
        }

    def _build_openai_chat_messages(self, action: str, payload: Dict[str, Any]) -> list:
        user_content = json.dumps({"action": action, "payload": payload}, ensure_ascii=False)
        return [
            {
                "role": "system",
                "content": "你正在参与一个‘谁是卧底’推理游戏。你的目标是赢得游戏并尽量不被淘汰，可自行制定存活策略。禁止说谎或捏造事实；不确定时请表达不确定。请严格按要求输出中文，不要泄露关键词。",
            },
            {"role": "user", "content": user_content},
        ]

    def _parse_openai_chat_content(self, data: Dict[str, Any]) -> str:
        choices = data.get("choices") or []
        if choices and isinstance(choices, list):
            first = choices[0] or {}
            msg = first.get("message") or {}
            content = msg.get("content")
            if content:
                return content
        return ""

    def generate(self, model_name: str, action: str, payload: Dict[str, Any]) -> ModelResponse:
        route = self._resolve_route(model_name)
        route_key = self._route_key(model_name, route)
        if self._is_circuit_open(route_key):
            info = self._route_failures.get(route_key, {})
            blocked_until = info.get("blocked_until", 0)
            cooldown_left = max(0, int(blocked_until - time.time()))
            raise ModelAPIRequestError(
                f"模型 {model_name} 通道处于熔断状态，请在 {cooldown_left} 秒后重试"
            )
        if route.get("mode") == "mock":
            return self._mock_response(model_name, action, payload, route)

        request_format = route.get("request_format", "custom")
        if request_format == "openai_chat":
            body = {
                "model": route["remote_model"],
                "messages": self._build_openai_chat_messages(action, payload),
                **route["extra_body"],
            }
        else:
            body = {
                "model": route["remote_model"],
                "action": action,
                "payload": payload,
                **route["extra_body"],
            }
        headers = {"Content-Type": "application/json", **route["headers"]}
        last_error: Optional[Exception] = None
        for attempt in range(self.max_retries + 1):
            try:
                resp = requests.post(route["base_url"], json=body, headers=headers, timeout=route["timeout"])
            except requests.RequestException as exc:
                last_error = exc
                if attempt == self.max_retries:
                    self._record_failure(route_key)
                    raise ModelAPIRequestError(f"模型 API 请求失败: {exc}") from exc
                self._record_failure(route_key)
                self._sleep_with_backoff(attempt)
                continue

            if resp.status_code >= 500 and attempt < self.max_retries:
                last_error = ModelAPIRequestError(f"模型 API 返回错误码 {resp.status_code}: {resp.text}")
                self._record_failure(route_key)
                self._sleep_with_backoff(attempt)
                continue
            if resp.status_code >= 400:
                self._record_failure(route_key)
                raise ModelAPIRequestError(f"模型 API 返回错误码 {resp.status_code}: {resp.text}")

            try:
                data = resp.json()
                self._reset_failure(route_key)
                break
            except ValueError as exc:
                last_error = exc
                if attempt == self.max_retries:
                    self._record_failure(route_key)
                    preview = resp.text[:200].strip()
                    raise ModelAPIRequestError(f"模型 API 返回非 JSON 内容: {preview}") from exc
                self._record_failure(route_key)
                self._sleep_with_backoff(attempt)
        else:
            if last_error:
                raise ModelAPIRequestError(str(last_error))
            raise ModelAPIRequestError("模型 API 在重试后仍无响应")

        if request_format == "openai_chat":
            content = self._parse_openai_chat_content(data)
        else:
            content = data.get("content") or data.get("text") or ""
        metadata = data.get("metadata") or {}
        if not content:
            raise ModelAPIRequestError("模型 API 未返回 content 字段")
        return ModelResponse(content=content, metadata=metadata)

    def _route_key(self, model_name: str, route: Dict[str, Any]) -> str:
        base = route.get("base_url", "")
        remote = route.get("remote_model", model_name)
        return f"{base}::{remote}"

    def _is_circuit_open(self, route_key: str) -> bool:
        info = self._route_failures.get(route_key)
        if not info:
            return False
        blocked_until = info.get("blocked_until")
        if blocked_until and blocked_until > time.time():
            return True
        if blocked_until and blocked_until <= time.time():
            self._route_failures.pop(route_key, None)
        return False

    def _record_failure(self, route_key: str) -> None:
        if not self.circuit_threshold:
            return
        info = self._route_failures.setdefault(route_key, {"fail_count": 0, "blocked_until": None})
        info["fail_count"] = info.get("fail_count", 0) + 1
        if info["fail_count"] >= self.circuit_threshold:
            info["blocked_until"] = time.time() + self.circuit_cooldown

    def _reset_failure(self, route_key: str) -> None:
        if route_key in self._route_failures:
            self._route_failures.pop(route_key, None)

    def _sleep_with_backoff(self, attempt: int) -> None:
        base = min(self.retry_backoff_factor * (attempt + 1), self.retry_backoff_cap)
        jitter = random.uniform(0, 0.5)
        time.sleep(base + jitter)

    def _mock_response(self, model_name: str, action: str, payload: Dict[str, Any], route: Dict[str, Any]) -> ModelResponse:
        rnd = random.Random(hash((model_name, action, payload.get("round"))))
        if action == "description":
            keyword = payload.get("keyword", "未知线索")
            role = payload.get("role", "civilian")
            tone = "含蓄" if role == "undercover" else "直观"
            text = f"[{model_name}] 用{tone}方式描述：这让我想到{keyword}的特征，与大家的线索可以相互验证。"
            return ModelResponse(content=text)
        if action == "analysis":
            descs = payload.get("descriptions", [])
            sample = descs[-1]["payload"] if descs else "暂无"
            text = f"[{model_name}] 认为刚刚的描述透露出『{sample[:20]}...』，怀疑最模糊的线索。"
            return ModelResponse(content=text)
        if action == "vote":
            alive = payload.get("alive", [])
            self_id = payload.get("model_id")
            candidates = [mid for mid in alive if mid != self_id]
            vote = candidates[rnd.randrange(len(candidates))] if candidates else self_id
            text = f"[{model_name}] 综合目前推理，倾向投 {vote}。"
            return ModelResponse(content=text, metadata={"vote_target": vote})
        return ModelResponse(content=f"[{model_name}] 暂无输出")
