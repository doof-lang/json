export import isolated function parseJsonValue(text: string): Result<SerialValue, string>
  from "./native_json.hpp" as doof_json::parse

export import isolated function formatJsonValue(value: SerialValue): string
  from "./native_json.hpp" as doof_json::format

export function parseJsonObject(text: string): Result<SerialObject, string> {
  try result := parseJsonValue(text)
  case result {
    o: SerialObject -> return Success(o)
    _ -> return Failure("Parsed value is not a JSON object")
  }
}
