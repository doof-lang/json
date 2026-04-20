export import function parseJsonValue(text: string): Result<JsonValue, string>
  from "./native_json.hpp" as doof_json::parse

export import function formatJsonValue(value: JsonValue): string
  from "doof_runtime.hpp" as doof::to_string