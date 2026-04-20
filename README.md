# std/json

JSON parsing and formatting for `JsonValue`. Parsing returns a `Result<JsonValue, string>` with readable error messages that include line and column information.

## Usage

```doof
import { formatJsonValue, parseJsonValue } from "std/json"

payload := try parseJsonValue("{\"name\":\"Ada\",\"active\":true}")
text := formatJsonValue([
  "ok",
  { only: 1 },
  false,
])

// payload is a JsonValue
// text == "[\"ok\",{\"only\":1},false]"
```

## Exports

### `parseJsonValue(text: string): Result<JsonValue, string>`

Parse a JSON document into a `JsonValue`.

- Accepts JSON nulls, booleans, numbers, strings, arrays, and objects.
- Integer tokens within 32-bit range become `int`; larger integral tokens become `long`; fractional or exponent tokens become `double`.
- String failures and syntax failures report a message with `line` and `column` information.

```doof
value := try parseJsonValue("[1,true,{\"only\":2}]")
```

---

### `formatJsonValue(value: JsonValue): string`

Serialize a `JsonValue` to compact JSON.

- Emits no extra whitespace.
- Escapes JSON control characters in strings.
- Array element order is preserved.
- Object member order should be treated as unspecified.

```doof
payload: JsonValue := ["line\nbreak", { only: "value" }]
text := formatJsonValue(payload)
// "[\"line\\nbreak\",{\"only\":\"value\"}]"
```