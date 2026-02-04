# 谁是卧底多模型推理能力测试框架

## 功能概述
- 随机分配 6 名模型身份（1 卧底 + 5 平民，可扩展为 2 卧底）
- 词汇库按照科技/文化/生活 40/30/30 比例抽取，维持 60%-80% 相似度
- 支持描述（30-60 秒）、分析、投票三个阶段的 5 轮流程控制
- 实时评估逻辑一致性、社交智能、策略思维、元认知
- 提供 Flask Web 控制台用于人工主持和状态查看
- 一键触发统一 HTTP 模型 API，自动完成描述/分析/投票

## 快速开始
```bash
cp .env.example .env  # 配置 MODEL_API_BASE_URL / MODEL_API_KEY / MODEL_API_CONFIG
pip install -r requirements.txt
python app.py --port 5000
```
访问 http://localhost:5000 查看状态，或调用 API 接口。

## 模型 API 接入
在 `.env` 中配置：
```
MODEL_API_BASE_URL=https://your-default-endpoint
MODEL_API_KEY=sk-xxx
MODEL_API_TIMEOUT=45
MODEL_API_CONFIG=model_providers.json
```
服务端会针对描述/分析/投票分别发送 `{"model", "action", "payload"}` 结构的 POST 请求，模型返回 `{"content": "...", "metadata": {"vote_target": "m2"}}` 即可。

### 多厂商/多模型路由
`MODEL_API_CONFIG` 指向的 JSON 需包含：
```json
{
  "providers": {
    "openai": {"base_url": "https://api.openai.com/...", "api_key": "env:OPENAI_API_KEY"},
    "aliyun": {"base_url": "https://dashscope.aliyuncs.com/...", "api_key": "env:ALIYUN_API_KEY"}
  },
  "models": {
    "Qwen3": {"provider": "aliyun", "remote_model": "qwen-plus"},
    "GPT4": {"provider": "openai", "remote_model": "gpt-4o"}
  }
}
```
`providers` 描述不同厂商的 base_url/API Key/超时/默认请求体；`models` 映射测试用模型名到厂商及远端模型名。默认 `.env` 中的 base_url 与 key 仍会作为后备方案。

## API 概览
- `GET /api/state`：获取最新状态快照
- `POST /start_game`：重新开始游戏，支持自定义模型与卧底数量
- `POST /submit_description`：提交描述（model_id, description, duration）
- `POST /submit_analysis`：提交分析（model_id, analysis）
- `POST /submit_vote`：提交投票（model_id, vote_target）
- `GET /evaluate`：返回四维评估分数
- `POST /auto_round`：调用统一 HTTP 模型 API 自动完成一轮流程

## 词库管理
参考 `werewolf_test.py` 中的 `WORD_PAIRS`，每个词条包含：
- `words`: (平民词, 卧底词)
- `category`: `tech` / `culture` / `life`
- `difficulty`: `life` / `professional` / `cross`
- `similarity`: 介于 0.6 与 0.8
- `tags`: 可用于扩展过滤

## 测试脚本示例
```python
import os
from model_api import ModelAPIClient
from werewolf_test import WerewolfTest

os.environ["MODEL_API_BASE_URL"] = "https://api.example.com/v1/werewolf"
os.environ["MODEL_API_KEY"] = "sk-xxx"

test = WerewolfTest(["Qwen3", "GPT4", "Claude3", "Llama3", "ModelX", "ModelY"])
test.start_game()
client = ModelAPIClient()

for _ in range(5):
    if not test.game_state.get("active"):
        break
    test.auto_play_round(client)

print(test.evaluate())
```
