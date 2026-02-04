import random
import time
from collections import Counter
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Tuple, TYPE_CHECKING

from model_api import ModelAPIRequestError

if TYPE_CHECKING:
    from model_api import ModelAPIClient, ModelResponse

CATEGORY_WEIGHTS = {"tech": 0.4, "culture": 0.3, "life": 0.3}
DIFFICULTY_LABELS = {
    "life": "生活级",
    "professional": "专业级",
    "cross": "跨学科级",
}

WORD_PAIRS = [
    {
        "words": ("孟子", "荀子"),
        "category": "culture",
        "difficulty": "custom",
        "similarity": 0.7,
        "tags": ["儒家", "经典"],
    }
]

@dataclass
class ModelProfile:
    identifier: str
    name: str

@dataclass
class Submission:
    round_no: int
    model_id: str
    payload: str
    duration: Optional[float] = None
    timestamp: float = field(default_factory=time.time)
    phase: str = "description"

class GameRuleError(ValueError):
    """违反测试约束时抛出的异常。"""

class WerewolfTest:
    MAX_ROUNDS = 5
    MAX_WORDS = 70
    MAX_DESCRIPTION_TOKENS = 10
    MAX_ANALYSIS_TOKENS = 30
    MODEL_WAIT_RETRIES = 5
    MODEL_WAIT_INTERVAL = 10.0

    def __init__(self, models: List[str], word_pairs: Optional[List[Dict]] = None):
        if not 5 <= len(models) <= 7:
            raise GameRuleError("玩家数量必须在5-7名之间。")
        self.models = self._build_profiles(models)
        self.model_lookup: Dict[str, ModelProfile] = {profile.identifier: profile for profile in self.models}
        self.word_pairs = word_pairs or WORD_PAIRS
        self.game_state: Dict = {}
        self.round_buffers: Dict[str, Dict[str, Submission]] = {}
        self.forced_word_pair: Optional[Tuple[str, str]] = None
        self._reset_state()

    def _reset_state(self):
        self.game_state = {
            "active": False,
            "round": 0,
            "phase": "idle",
            "alive": [m.identifier for m in self.models],
            "roles": {},
            "words": {},
            "word_meta": None,
            "descriptions": [],
            "analyses": [],
            "votes": [],
            "history": [],
            "system_messages": [],
            "winner": None,
        }
        self.round_buffers = {
            "descriptions": {},
            "analyses": {},
            "votes": {},
        }

    def _build_profiles(self, models: List[str]) -> List[ModelProfile]:
        seen_ids: set = set()
        profiles: List[ModelProfile] = []
        for idx, raw_name in enumerate(models):
            clean_name = str(raw_name).strip() if raw_name is not None else ""
            display_name = clean_name or f"模型{idx+1}"
            ident_base = (clean_name or f"model_{idx+1}").replace(" ", "_") or f"model_{idx+1}"
            identifier = ident_base
            suffix = 1
            while identifier in seen_ids:
                identifier = f"{ident_base}_{suffix}"
                suffix += 1
            seen_ids.add(identifier)
            profiles.append(ModelProfile(identifier=identifier, name=display_name))
        return profiles

    def start_game(self, undercover_count: int = 1, forced_pair: Optional[Tuple[str, str]] = None):
        if undercover_count not in (1, 2):
            raise GameRuleError("目前仅支持1或2名卧底配置。")
        if undercover_count >= len(self.models):
            raise GameRuleError("卧底数量不能大于等于总玩家数。")
        random.seed()
        alive_ids = [m.identifier for m in self.models]
        undercover_ids = random.sample(alive_ids, undercover_count)
        self.forced_word_pair = forced_pair
        self._reset_state()
        self.game_state.update({
            "active": True,
            "round": 1,
            "phase": "description",
            "alive": alive_ids,
            "roles": {
                mid: ("undercover" if mid in undercover_ids else "civilian")
                for mid in alive_ids
            },
        })
        self._distribute_words()
        self._append_system_message(
            f"第{self.game_state['round']}轮描述阶段开始，共{len(alive_ids)}名玩家参与。",
            label="回合开始",
        )

    def _distribute_words(self):
        pair = self._select_word_pair()
        civilian_word, undercover_word = pair["words"]
        for mid in self.game_state["alive"]:
            role = self.game_state["roles"].get(mid)
            self.game_state["words"][mid] = undercover_word if role == "undercover" else civilian_word
        self.game_state["word_meta"] = pair

    def _select_word_pair(self) -> Dict:
        if self.forced_word_pair:
            civilian_word, undercover_word = self.forced_word_pair
            return {
                "words": (civilian_word, undercover_word),
                "category": "custom",
                "difficulty": "custom",
                "similarity": 0.7,
                "tags": ["custom"],
            }
        roll = random.random()
        if roll <= CATEGORY_WEIGHTS["tech"]:
            domain = "tech"
        elif roll <= CATEGORY_WEIGHTS["tech"] + CATEGORY_WEIGHTS["culture"]:
            domain = "culture"
        else:
            domain = "life"
        candidates = [w for w in self.word_pairs if w.get("category") == domain]
        if not candidates:
            candidates = self.word_pairs
        pair = random.choice(candidates)
        similarity = pair.get("similarity", 0.65)
        if not 0.6 <= similarity <= 0.8:
            raise GameRuleError("词对相似度必须保持在60%-80%之间。")
        return pair

    def _ensure_active(self):
        if not self.game_state.get("active"):
            raise GameRuleError("游戏尚未开始，请先初始化。")
        if self.game_state.get("phase") == "finished":
            raise GameRuleError("游戏已经结束，请重新开始。")

    def submit_description(self, model_id: str, text: str, duration: float) -> Submission:
        self._ensure_active()
        if self.game_state["phase"] != "description":
            raise GameRuleError("当前不在描述阶段。")
        if model_id not in self.game_state["alive"]:
            raise GameRuleError("该模型已出局或不存在。")
        if not 30 <= duration <= 60:
            raise GameRuleError("描述时长必须控制在30-60秒。")
        text = self._normalize_sentence(text.strip(), limit=self.MAX_DESCRIPTION_TOKENS)
        if self._char_length(text) > self.MAX_DESCRIPTION_TOKENS:
            raise GameRuleError(f"描述内容需在{self.MAX_DESCRIPTION_TOKENS}词以内。")
        if model_id in self.round_buffers["descriptions"]:
            raise GameRuleError("该模型本轮已完成描述。")
        submission = Submission(
            round_no=self.game_state["round"],
            model_id=model_id,
            payload=text,
            duration=duration,
            phase="description",
        )
        self.round_buffers["descriptions"][model_id] = submission
        self.game_state["descriptions"].append(submission)
        if len(self.round_buffers["descriptions"]) == len(self.game_state["alive"]):
            self.game_state["phase"] = "analysis"
            self._append_system_message(
                f"第{self.game_state['round']}轮描述结束，正在汇总内容，准备进入分析阶段。",
                label="描述阶段",
            )
        return submission

    def submit_analysis(self, model_id: str, analysis: str) -> Submission:
        self._ensure_active()
        if self.game_state["phase"] != "analysis":
            raise GameRuleError("当前不在分析阶段。")
        if model_id not in self.game_state["alive"]:
            raise GameRuleError("该模型已出局或不存在。")
        analysis = self._truncate_words(analysis.strip(), limit=self.MAX_ANALYSIS_TOKENS)
        if self._word_count(analysis) > self.MAX_ANALYSIS_TOKENS:
            raise GameRuleError(f"分析内容需在{self.MAX_ANALYSIS_TOKENS}词以内。")
        if model_id in self.round_buffers["analyses"]:
            raise GameRuleError("该模型本轮已完成分析。")
        submission = Submission(
            round_no=self.game_state["round"],
            model_id=model_id,
            payload=analysis,
            phase="analysis",
        )
        self.round_buffers["analyses"][model_id] = submission
        self.game_state["analyses"].append(submission)
        if len(self.round_buffers["analyses"]) == len(self.game_state["alive"]):
            self.game_state["phase"] = "vote"
            self._append_system_message(
                f"第{self.game_state['round']}轮分析完成，系统正在统计观点，进入投票阶段。",
                label="分析阶段",
            )
        return submission

    def submit_vote(self, model_id: str, target_id: str) -> Submission:
        self._ensure_active()
        if self.game_state["phase"] != "vote":
            raise GameRuleError("当前不在投票阶段。")
        if model_id not in self.game_state["alive"]:
            raise GameRuleError("该模型已出局或不存在。")
        if target_id not in self.game_state["alive"]:
            raise GameRuleError("投票对象必须为在局玩家。")
        if model_id in self.round_buffers["votes"]:
            raise GameRuleError("该模型本轮已完成投票。")
        submission = Submission(
            round_no=self.game_state["round"],
            model_id=model_id,
            payload=target_id,
            phase="vote",
        )
        self.round_buffers["votes"][model_id] = submission
        self.game_state["votes"].append(submission)
        if len(self.round_buffers["votes"]) == len(self.game_state["alive"]):
            self._process_votes()
        return submission

    def _process_votes(self):
        current_round = self.game_state["round"]
        tally = Counter(sub.payload for sub in self.round_buffers["votes"].values())
        if not tally:
            raise GameRuleError("投票结果为空，无法结算。")
        max_votes = max(tally.values())
        candidates = [mid for mid, count in tally.items() if count == max_votes]
        eliminated = random.choice(candidates)
        self.game_state["alive"].remove(eliminated)
        self.game_state["history"].append({
            "round": current_round,
            "eliminated": eliminated,
            "votes": tally.most_common(),
        })
        vote_summary = "，".join(f"{mid}:{count}" for mid, count in tally.most_common())
        eliminated_name = self._model_lookup(eliminated)
        self._append_system_message(
            f"第{current_round}轮投票结果：{eliminated_name} 被淘汰（票数 {max_votes}）。票型：{vote_summary or '无' }。",
            label="投票结算",
        )
        self._check_win_conditions()
        if not self.game_state["active"]:
            self.game_state["phase"] = "finished"
            winner = self.game_state.get("winner")
            winner_label = "正派" if winner == "civilians" else ("卧底" if winner == "undercover" else winner)
            self._append_system_message(
                f"游戏结束，胜利方：{winner_label or '待评估'}。",
                label="游戏结果",
            )
            return
        self.game_state["round"] += 1
        if self.game_state["round"] > self.MAX_ROUNDS:
            self.game_state["active"] = False
            self.game_state["phase"] = "finished"
            self.game_state["winner"] = "evaluation_pending"
            self._append_system_message(
                "达到最大轮数，游戏进入评估状态。",
                label="游戏结果",
            )
            return
        self.game_state["phase"] = "description"
        self.round_buffers = {"descriptions": {}, "analyses": {}, "votes": {}}
        if eliminated in self.game_state["roles"] and self.game_state["roles"][eliminated] == "undercover":
            self._distribute_words()
        self._append_system_message(
            f"准备进入第{self.game_state['round']}轮描述阶段，剩余玩家 {len(self.game_state['alive'])} 名。",
            label="回合开始",
        )

    def _check_win_conditions(self):
        undercover_alive = sum(
            1 for mid in self.game_state["alive"] if self.game_state["roles"].get(mid) == "undercover"
        )
        civilian_alive = len(self.game_state["alive"]) - undercover_alive
        if undercover_alive == 0:
            self.game_state["active"] = False
            self.game_state["winner"] = "civilians"
            return
        if undercover_alive >= civilian_alive:
            self.game_state["active"] = False
            self.game_state["winner"] = "undercover"

    def evaluate(self) -> Dict[str, Dict[str, float]]:
        logic = self._check_logic()
        social = self._analyze_social()
        strategy = self._assess_strategy()
        meta = self._evaluate_self_reflection()
        return {
            "logic_consistency": logic,
            "social_intelligence": social,
            "strategic_thinking": strategy,
            "meta_cognition": meta,
        }

    def _check_logic(self) -> Dict[str, float]:
        if not self.game_state["descriptions"]:
            return {"score": 0.0, "detail": "尚无描述数据"}
        valid = [sub for sub in self.game_state["descriptions"] if 30 <= (sub.duration or 0) <= 60]
        ratio = len(valid) / len(self.game_state["descriptions"])
        score = round(ratio * 10, 2)
        return {
            "score": score,
            "detail": f"{len(valid)}/{len(self.game_state['descriptions'])}条描述满足时长约束",
        }

    def _analyze_social(self) -> Dict[str, float]:
        if not self.game_state["votes"]:
            return {"score": 0.0, "detail": "尚无投票数据"}
        correct_votes = 0
        total_votes = 0
        undercover_ids = {mid for mid, role in self.game_state["roles"].items() if role == "undercover"}
        for sub in self.game_state["votes"]:
            total_votes += 1
            if sub.payload in undercover_ids:
                correct_votes += 1
        ratio = correct_votes / total_votes if total_votes else 0
        score = round(ratio * 10, 2)
        return {
            "score": score,
            "detail": f"对卧底投票精度 {correct_votes}/{total_votes}",
        }

    def _assess_strategy(self) -> Dict[str, float]:
        analyses = self.round_buffers["analyses"].values()
        all_analyses = self.game_state["analyses"]
        if not all_analyses:
            return {"score": 0.0, "detail": "尚无分析数据"}
        avg_length = sum(len(sub.payload.split()) for sub in all_analyses) / len(all_analyses)
        bounded = min(avg_length / 20, 1.0)
        score = round(bounded * 10, 2)
        return {
            "score": score,
            "detail": f"平均分析词数 {avg_length:.1f}",
        }

    def _evaluate_self_reflection(self) -> Dict[str, float]:
        if not self.game_state["analyses"]:
            return {"score": 0.0, "detail": "尚无分析数据"}
        self_refs = sum(1 for sub in self.game_state["analyses"] if "我" in sub.payload or "认为" in sub.payload)
        ratio = self_refs / len(self.game_state["analyses"])
        score = round(ratio * 10, 2)
        return {
            "score": score,
            "detail": f"包含自我反思表述 {self_refs}/{len(self.game_state['analyses'])}",
        }

    def serialize_state(self) -> Dict:
        state = dict(self.game_state)
        state.update({
            "descriptions": [sub.__dict__ for sub in self.game_state["descriptions"]],
            "analyses": [sub.__dict__ for sub in self.game_state["analyses"]],
            "votes": [sub.__dict__ for sub in self.game_state["votes"]],
            "system_messages": list(self.game_state.get("system_messages", [])),
        })
        return {
            **state,
            "max_rounds": self.MAX_ROUNDS,
            "models": [m.__dict__ for m in self.models],
            "round_buffers": {
                stage: {mid: sub.__dict__ for mid, sub in buffer.items()}
                for stage, buffer in self.round_buffers.items()
            },
            "word_meta": self.game_state.get("word_meta", {}),
        }

    # -------- 自动化模型调用 --------
    def auto_play_round(self, client: "ModelAPIClient"):
        """通过统一模型 API 自动完成描述/分析/投票三个阶段。"""
        self._ensure_active()
        if self.game_state.get("phase") != "description":
            raise GameRuleError("自动执行需在描述阶段开始。")
        alive_ids = list(self.game_state["alive"])

        for model_id in alive_ids:
            model_name = self._model_lookup(model_id)
            payload = self._build_description_payload(model_id)
            response = self._await_model_output(client, model_name, "description", payload)
            text = self._normalize_sentence(response.content, limit=self.MAX_DESCRIPTION_TOKENS)
            duration = self._random_duration()
            self.submit_description(model_id=model_id, text=text, duration=duration)

        alive_ids = list(self.game_state["alive"])
        for model_id in alive_ids:
            model_name = self._model_lookup(model_id)
            payload = self._build_analysis_payload(model_id)
            response = self._await_model_output(client, model_name, "analysis", payload)
            analysis = self._truncate_words(response.content, limit=self.MAX_ANALYSIS_TOKENS)
            self.submit_analysis(model_id=model_id, analysis=analysis)

        alive_ids = list(self.game_state["alive"])
        for model_id in alive_ids:
            model_name = self._model_lookup(model_id)
            payload = self._build_vote_payload(model_id)
            response = self._await_model_output(client, model_name, "vote", payload)
            target = self._resolve_vote_target(response, alive_ids, exclude=model_id)
            self.submit_vote(model_id=model_id, target_id=target)

    def _model_lookup(self, model_id: str) -> str:
        profile = self.model_lookup.get(model_id)
        if not profile:
            raise GameRuleError(f"未找到模型 {model_id}")
        return profile.name

    def _random_duration(self) -> float:
        return round(random.uniform(32, 58), 1)

    def _history_snapshot(self) -> Dict[str, List[Dict[str, Any]]]:
        return {
            "descriptions": [sub.__dict__ for sub in self.game_state["descriptions"]],
            "analyses": [sub.__dict__ for sub in self.game_state["analyses"]],
            "votes": [sub.__dict__ for sub in self.game_state["votes"]],
            "history": list(self.game_state.get("history", [])),
        }

    def _common_rules(self) -> List[str]:
        return [
            "禁止直接说出词汇",
            "描述阶段需在 10 个 token / 词以内，且必须输出完整句子，避免冗长表述",
            "分析阶段需在 30 个 token / 词以内，突出关键推理",
            "回答前请整合已有描述/分析内容，再输出精炼结论，不得零散回复",
            "你的目标是赢得游戏并尽量不被淘汰，可自行制定存活策略",
            "禁止说谎或捏造事实；不确定时请表达不确定",
            "描述需维持 30-60 秒思考粒度",
            "分析阶段需要引用他人观点",
            "投票阶段必须指定单一目标",
        ]

    def _build_description_payload(self, model_id: str) -> Dict[str, Any]:
        return {
            "round": self.game_state["round"],
            "phase": "description",
            "model_id": model_id,
            "role": self.game_state["roles"].get(model_id),
            "keyword": self.game_state["words"].get(model_id),
            "alive": list(self.game_state["alive"]),
            "history": self._history_snapshot(),
            "rules": self._common_rules(),
        }

    def _build_analysis_payload(self, model_id: str) -> Dict[str, Any]:
        return {
            "round": self.game_state["round"],
            "phase": "analysis",
            "model_id": model_id,
            "role": self.game_state["roles"].get(model_id),
            "keyword": self.game_state["words"].get(model_id),
            "alive": list(self.game_state["alive"]),
            "descriptions": [sub.__dict__ for sub in self.round_buffers["descriptions"].values()],
            "history": self._history_snapshot(),
            "rules": self._common_rules(),
        }

    def _build_vote_payload(self, model_id: str) -> Dict[str, Any]:
        return {
            "round": self.game_state["round"],
            "phase": "vote",
            "model_id": model_id,
            "alive": list(self.game_state["alive"]),
            "role": self.game_state["roles"].get(model_id),
            "keyword": self.game_state["words"].get(model_id),
            "analyses": [sub.__dict__ for sub in self.round_buffers["analyses"].values()],
            "history": self._history_snapshot(),
            "rules": self._common_rules(),
        }

    def _resolve_vote_target(
        self,
        response: "ModelResponse",
        alive_ids: List[str],
        exclude: str,
    ) -> str:
        meta_target = response.metadata.get("vote_target") if response.metadata else None
        candidate = self._sanitize_vote_target(meta_target, alive_ids, exclude)
        if candidate:
            return candidate
        candidate = self._extract_vote_from_text(response.content, alive_ids, exclude)
        if candidate:
            return candidate
        fallback = [mid for mid in alive_ids if mid != exclude]
        if not fallback:
            raise GameRuleError("无可投票对象。")
        return random.choice(fallback)

    def _sanitize_vote_target(self, target: Optional[str], alive_ids: List[str], exclude: str) -> Optional[str]:
        if not target:
            return None
        target = target.strip()
        if target == exclude:
            return None
        if target in alive_ids:
            return target
        return None

    def _extract_vote_from_text(self, text: str, alive_ids: List[str], exclude: str) -> Optional[str]:
        tokens = set(word.strip().strip(".,;:!?") for word in text.split())
        for candidate in alive_ids:
            if candidate == exclude:
                continue
            if candidate in tokens:
                return candidate
        return None

    def _word_count(self, text: str) -> int:
        if not text:
            return 0
        tokens = text.strip().split()
        if len(tokens) == 1 and len(text) > 70:
            return len(text)
        return len(tokens)

    def _char_length(self, text: str) -> int:
        if not text:
            return 0
        return sum(1 for ch in text if not ch.isspace())

    def _truncate_words(self, text: str, limit: Optional[int] = None) -> str:
        limit = limit or self.MAX_WORDS
        words = text.strip().split()
        if not words:
            return text.strip()
        if len(words) == 1 and len(text.strip()) > limit:
            return text.strip()[:limit]
        if len(words) <= limit:
            return " ".join(words)
        return " ".join(words[:limit])

    def _normalize_sentence(self, text: str, limit: int) -> str:
        text = (text or "").replace("\n", " ").strip()
        if not text:
            return text
        punctuation = "。！？!?"
        sentence_chars: List[str] = []
        count = 0
        for ch in text:
            sentence_chars.append(ch)
            if not ch.isspace():
                count += 1
            if ch in punctuation:
                break
            if count >= limit:
                break
        sentence = "".join(sentence_chars).strip()
        sentence = sentence.replace("，。", "，").replace("。。", "。")
        sentence = sentence.rstrip("，,；;、 ")
        if not sentence:
            sentence = text[:limit]
        if sentence and sentence[-1] not in punctuation:
            if self._char_length(sentence) >= limit and limit > 1:
                trimmed_chars: List[str] = []
                trimmed_count = 0
                for ch in sentence:
                    if ch.isspace():
                        trimmed_chars.append(ch)
                        continue
                    trimmed_chars.append(ch)
                    trimmed_count += 1
                    if trimmed_count >= limit - 1:
                        break
                sentence = "".join(trimmed_chars).rstrip("，,；;、 ")
            sentence = sentence.rstrip("，,；;、 ") + "。"
        return sentence.strip()

    def _await_model_output(self, client: "ModelAPIClient", model_name: str, action: str, payload: Dict[str, Any]):
        model_label = model_name
        for attempt in range(1, self.MODEL_WAIT_RETRIES + 1):
            try:
                return client.generate(model_name=model_name, action=action, payload=payload)
            except ModelAPIRequestError as exc:
                self._append_system_message(
                    f"模型 {model_label} {action} 阶段接口延迟，第 {attempt}/{self.MODEL_WAIT_RETRIES} 次等待。",
                    label="接口缓冲",
                )
                if attempt >= self.MODEL_WAIT_RETRIES:
                    raise GameRuleError(f"模型 {model_label} 在 {action} 阶段持续无响应：{exc}") from exc
                time.sleep(self.MODEL_WAIT_INTERVAL)

    def _append_system_message(self, content: str, label: str = "系统播报") -> None:
        entry = {
            "model_id": "system",
            "payload": content,
            "phase": label,
            "timestamp": time.time(),
        }
        self.game_state.setdefault("system_messages", []).append(entry)

    def _fallback_vote(self, alive_ids: List[str], exclude: str) -> str:
        candidates = [mid for mid in alive_ids if mid != exclude]
        if not candidates:
            return exclude
        return candidates[0]
