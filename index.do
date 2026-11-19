export import isolated function parseJsonValue(text: string): Result<SerialValue, string>
  from "./native_json.hpp" as doof_json::parse

export import isolated function formatJsonValue(value: SerialValue): string
  from "./native_json.hpp" as doof_json::format

export function parseJsonObject(text: string): Result<SerialObject, string> {
  try result := parseJsonValue(text)
  object := result as SerialObject else {
    return Failure("Parsed value is not a JSON object")
  }
  return Success(object)
}
