import { formatJsonValue, parseJsonValue } from "./index"

function requireParsed(text: string): JsonValue {
  let value: JsonValue = null
  let found = false

  case parseJsonValue(text) {
    s: Success => {
      value = s.value
      found = true
    }
    f: Failure => {
      assert(false, "expected JSON parse success: ${f.error}")
    }
  }

  assert(found, "expected parsed JSON helper to produce a value")
  return value
}

function requireParseFailure(text: string): string {
  let message = ""
  let found = false

  case parseJsonValue(text) {
    s: Success => {
      assert(false, "expected JSON parse failure")
    }
    f: Failure => {
      message = f.error
      found = true
    }
  }

  assert(found, "expected JSON parse failure helper to produce a message")
  return message
}

export function testParsePrimitivesAndEscapes(): void {
  assert(formatJsonValue(requireParsed("null")) == "null", "expected null to round-trip")
  assert(formatJsonValue(requireParsed("true")) == "true", "expected true to round-trip")
  assert(formatJsonValue(requireParsed("false")) == "false", "expected false to round-trip")
  assert(
    formatJsonValue(requireParsed("\"line\\nfeed\"")) == "\"line\\nfeed\"",
    "expected escaped newlines to round-trip"
  )
  assert(
    formatJsonValue(requireParsed("\"\\u0041\"")) == "\"A\"",
    "expected unicode escapes to decode to UTF-8 text"
  )
}

export function testParseNumbersAcrossIntegerRanges(): void {
  assert(formatJsonValue(requireParsed("42")) == "42", "expected small integers to round-trip")
  assert(
    formatJsonValue(requireParsed("2147483648")) == "2147483648",
    "expected integers past int range to round-trip"
  )
  assert(
    formatJsonValue(requireParsed("-2147483649")) == "-2147483649",
    "expected negative long values to round-trip"
  )
  assert(formatJsonValue(requireParsed("1.25")) == "1.25", "expected decimal numbers to round-trip")
  assert(formatJsonValue(requireParsed("1e1")) == "10", "expected exponent numbers to parse")
}

export function testParseArraysAndObjectsRoundTrip(): void {
  assert(formatJsonValue(requireParsed("[]")) == "[]", "expected empty arrays to round-trip")
  assert(formatJsonValue(requireParsed("{}")) == "{}", "expected empty objects to round-trip")

  parsed := requireParsed("[null,true,1,\"ok\",{\"only\":2},[]]")
  assert(
    formatJsonValue(parsed) == "[null,true,1,\"ok\",{\"only\":2},[]]",
    "expected nested arrays and single-key objects to round-trip"
  )
}

export function testFormatConstructedValues(): void {
  payload: JsonValue := [
    "line\nbreak",
    { only: "value" },
    2147483648L,
    false,
    null,
  ]

  assert(
    formatJsonValue(payload) == "[\"line\\nbreak\",{\"only\":\"value\"},2147483648,false,null]",
    "expected constructed JsonValue payloads to format as compact JSON"
  )
}

export function testParseRejectsInvalidNumbers(): void {
  leadingZero := requireParseFailure("01")
  assert(
    leadingZero.contains("Leading zeros are not allowed in JSON numbers"),
    "expected leading zero diagnostics"
  )

  badExponent := requireParseFailure("1e")
  assert(badExponent.contains("Invalid JSON number exponent"), "expected exponent diagnostics")

  outOfRange := requireParseFailure("1e309")
  assert(outOfRange.contains("JSON number out of range"), "expected range diagnostics")
}

export function testParseReportsLineAndColumn(): void {
  assert(
    requireParseFailure("{\n  \"a\" 1\n}") == "Expected ':' after object key at line 2, column 7",
    "expected object syntax errors to report line and column"
  )
}

export function testParseRejectsBrokenUnicodeEscapes(): void {
  incomplete := requireParseFailure("\"\\u12")
  assert(incomplete.contains("Incomplete unicode escape"), "expected incomplete unicode diagnostics")

  missingLow := requireParseFailure("\"\\uD83D\"")
  assert(missingLow.contains("Expected unicode low surrogate"), "expected missing low surrogate diagnostics")

  unexpectedLow := requireParseFailure("\"\\uDE00\"")
  assert(unexpectedLow.contains("Unexpected unicode low surrogate"), "expected unexpected low surrogate diagnostics")
}

export function testParseRejectsTrailingCharacters(): void {
  message := requireParseFailure("true false")
  assert(message.contains("Unexpected trailing characters"), "expected trailing character diagnostics")
}