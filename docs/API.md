# std/json Guide

`std/json` parses JSON text into Doof `JsonValue` values and formats
`JsonValue` back to compact JSON. It is the lowest-level JSON helper; higher
level typed serialization is handled by the Doof runtime metadata system.

## Quick Start

```doof
import { formatJsonValue, parseJsonObject, parseJsonValue } from "std/json"

value := try! parseJsonValue("[1,true,{\"name\":\"Ada\"}]")
object := try! parseJsonObject("{\"ok\":true}")
text := formatJsonValue({ ok: true, count: 3 })
```

## Parsing

`parseJsonValue(text)` accepts any JSON document: null, booleans, numbers,
strings, arrays, and objects. Syntax failures return `Failure<string>` with a
readable message that includes line and column information.

Numeric tokens are mapped to Doof numeric values:

- 32-bit integer tokens become `int`
- larger integral tokens become `long`
- fractional or exponent tokens become `double`

`parseJsonObject(text)` is a convenience wrapper for APIs that require a JSON
object at the top level. It returns `Failure("Parsed value is not a JSON object")`
when parsing succeeds but the top-level value is not an object.

## Formatting

`formatJsonValue(value)` emits compact JSON with no extra whitespace. Array order
is preserved. Object member order follows the insertion order of the underlying
`JsonObject`.

Control characters in strings are escaped using JSON escapes.

## API

### `parseJsonValue`

```doof
export import function parseJsonValue(text: string): Result<JsonValue, string>
```

Parse any JSON value.

Defined in [index.do](../index.do).

### `parseJsonObject`

```doof
export function parseJsonObject(text: string): Result<JsonObject, string>
```

Parse JSON and require the top-level value to be an object.

Defined in [index.do](../index.do).

### `formatJsonValue`

```doof
export import function formatJsonValue(value: JsonValue): string
```

Serialize a `JsonValue` to compact JSON.

Defined in [index.do](../index.do).
