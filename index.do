export import isolated function parseJsonValue(text: string): Result<JsonValue, string>
  from "./native_json.hpp" as doof_json::parse

export import isolated function formatJsonValue(value: JsonValue): string
  from "./native_json.hpp" as doof_json::format

export function parseJsonObject(text: string): Result<JsonObject, string> {
  try result := parseJsonValue(text)
  case result {
    o: JsonObject -> return Success(o)
    _ -> return Failure("Parsed value is not a JSON object")
  }
}
