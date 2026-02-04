import os
import time
from typing import Any, Dict, List, Optional

from flask import Flask, jsonify, render_template, request

from model_api import ModelAPIClient, ModelAPIConfigError, ModelAPIRequestError
from werewolf_test import GameRuleError, WerewolfTest

app = Flask(__name__)

DEFAULT_MODELS = [
    "qwen3-max",
    "deepseek-r1-distill-qwen-3",
    "qwq-plus",
    "qwen2.5-vl-32b-instruct",
    "deepseek-v3.1",
]
test_game: Optional[WerewolfTest] = None
model_client: Optional[ModelAPIClient] = None
run_history: List[Dict[str, Any]] = []


def _parse_models(raw_models) -> List[str]:
    if raw_models is None:
        return DEFAULT_MODELS
    if isinstance(raw_models, list):
        parsed = [str(item).strip() for item in raw_models]
    else:
        tokens = str(raw_models).replace("\r", "\n").split("\n")
        parsed = [token.strip() for token in tokens]
    parsed = [name for name in parsed if name]
    return parsed or DEFAULT_MODELS


def _ensure_game(auto_bootstrap: bool = True) -> WerewolfTest:
    global test_game
    if test_game is None and auto_bootstrap:
        test_game = WerewolfTest(DEFAULT_MODELS)
        test_game.start_game()
    if test_game is None:
        raise GameRuleError("游戏尚未初始化。")
    return test_game


def _get_model_client() -> ModelAPIClient:
    global model_client
    if model_client is None:
        model_client = ModelAPIClient()
    return model_client


@app.route("/")
def index():
    game = _ensure_game()
    return render_template("game.html", state=game.serialize_state())


@app.route("/api/state", methods=["GET"])
def api_state():
    game = _ensure_game()
    return jsonify(game.serialize_state())


@app.route("/start_auto_game", methods=["POST"])
def start_auto_game():
    global test_game
    data = request.get_json(silent=True) or {}
    models = _parse_models(data.get("models"))
    try:
        test_game = WerewolfTest(models)
        test_game.start_game(undercover_count=1, forced_pair=("孟子", "荀子"))
        _run_full_game(test_game)
        evaluation = test_game.evaluate()
        record = {
            "run_id": f"run-{int(time.time() * 1000)}",
            "ts": time.time(),
            "models": list(models),
            "winner": test_game.game_state.get("winner"),
            "evaluation": evaluation,
        }
        run_history.append(record)
        return jsonify({"status": "completed", "state": test_game.serialize_state(), "evaluation": evaluation, "record": record})
    except (GameRuleError, ModelAPIConfigError, ModelAPIRequestError) as exc:
        return jsonify({"status": "error", "message": str(exc)}), 400


@app.route("/reset_game", methods=["POST"])
def reset_game():
    global test_game
    data = request.get_json(silent=True) or {}
    models = _parse_models(data.get("models"))
    try:
        test_game = WerewolfTest(models)
        test_game.start_game(undercover_count=1, forced_pair=("孟子", "荀子"))
        return jsonify({"status": "ok", "state": test_game.serialize_state()})
    except GameRuleError as exc:
        return jsonify({"status": "error", "message": str(exc)}), 400


@app.route("/api/run_history", methods=["GET"])
def api_run_history():
    return jsonify({"status": "ok", "runs": list(run_history)})


@app.route("/api/clear_history", methods=["POST"])
def clear_history():
    run_history.clear()
    return jsonify({"status": "ok"})


@app.route("/run_samples", methods=["POST"])
def run_samples():
    global test_game
    data = request.get_json(silent=True) or {}
    models = _parse_models(data.get("models"))
    samples = int(data.get("samples") or 3)
    samples = max(1, min(samples, 20))
    records: List[Dict[str, Any]] = []
    try:
        for _ in range(samples):
            game = WerewolfTest(models)
            game.start_game(undercover_count=1, forced_pair=("孟子", "荀子"))
            _run_full_game(game)
            evaluation = game.evaluate()
            record = {
                "run_id": f"run-{int(time.time() * 1000)}",
                "ts": time.time(),
                "models": list(models),
                "winner": game.game_state.get("winner"),
                "evaluation": evaluation,
            }
            run_history.append(record)
            records.append(record)
        test_game = WerewolfTest(models)
        test_game.start_game(undercover_count=1, forced_pair=("孟子", "荀子"))
        return jsonify({"status": "ok", "records": records, "run_count": len(records)})
    except (GameRuleError, ModelAPIConfigError, ModelAPIRequestError) as exc:
        return jsonify({"status": "error", "message": str(exc)}), 400


@app.route("/generate_final_report", methods=["POST"])
def generate_final_report():
    data = request.get_json(silent=True) or {}
    model_name = str(data.get("model") or "openai")
    try:
        client = _get_model_client()
        runs = list(run_history)
        if not runs:
            return jsonify({"status": "error", "message": "暂无采样结果，请先运行至少一局。"}), 400
        payload = {
            "task": "你是裁判。请根据多次游戏采样结果，给出各评估维度的权重并加权汇总，输出最终报告。",
            "runs": runs,
            "required_output": {
                "weights": "给出 logic_consistency/social_intelligence/strategic_thinking/meta_cognition 四项权重(和为1)",
                "summary": "用中文给出总体结论与关键观察",
                "recommendations": "给出改进建议",
            },
        }
        response = client.generate(model_name=model_name, action="final_report", payload=payload)
        return jsonify({"status": "ok", "report": response.content, "run_count": len(runs)})
    except (ModelAPIConfigError, ModelAPIRequestError) as exc:
        return jsonify({"status": "model_api_error", "message": str(exc)}), 500


@app.route("/generate_report", methods=["POST"])
def generate_report():
    game = _ensure_game()
    try:
        client = _get_model_client()
        evaluation = game.evaluate()
        payload = {
            "evaluation": evaluation,
            "history": game.game_state.get("history", []),
            "descriptions": [sub.__dict__ for sub in game.game_state["descriptions"]],
            "analyses": [sub.__dict__ for sub in game.game_state["analyses"]],
            "votes": [sub.__dict__ for sub in game.game_state["votes"]],
            "winner": game.game_state.get("winner"),
        }
        response = client.generate(model_name="gemini", action="report", payload=payload)
        return jsonify({"status": "ok", "report": response.content})
    except (ModelAPIConfigError, ModelAPIRequestError) as exc:
        return jsonify({"status": "model_api_error", "message": str(exc)}), 500


def _run_full_game(game: WerewolfTest):
    client = _get_model_client()
    safety = 0
    while game.game_state.get("active") and safety <= WerewolfTest.MAX_ROUNDS + 2:
        if game.game_state.get("phase") != "description":
            break
        game.auto_play_round(client)
        safety += 1


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port, debug=True)
